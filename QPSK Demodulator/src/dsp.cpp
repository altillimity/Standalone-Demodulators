#include "utils.h"
#include "dsp.h"
#ifdef _WIN32
#include <io.h>
#endif

void QPSKDemodulatorDSP::initDSP(
    std::function<void(int)> pProgressCallback,
    std::function<void(void)> pDoneCallback,
    std::function<void(int8_t, int8_t, int)> pConstellationCallback)
{
    // Buffers
    buffer = new std::complex<float>[BUFFER_SIZE * 10];
    buffer2 = new std::complex<float>[BUFFER_SIZE * 10];
    buffer3 = new std::complex<float>[BUFFER_SIZE * 10];
    agc_buffer = new std::complex<float>[BUFFER_SIZE * 10];
    agc_buffer2 = new std::complex<float>[BUFFER_SIZE * 10];
    filter_buffer = new std::complex<float>[BUFFER_SIZE * 10];
    filter_buffer2 = new std::complex<float>[BUFFER_SIZE * 10];
    filter_buffer3 = new std::complex<float>[BUFFER_SIZE * 10];
    pll_buffer = new std::complex<float>[BUFFER_SIZE * 10];
    recovery_buffer = new std::complex<float>[BUFFER_SIZE * 13];
    recovered_buffer = new std::complex<float>[BUFFER_SIZE * 10];
    buffer_i16 = new int16_t[BUFFER_SIZE * 2];
    buffer_i8 = new int8_t[BUFFER_SIZE * 2];
    buffer_u8 = new uint8_t[BUFFER_SIZE * 2];

    // Init FIFOs
    dc_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    pre_agc_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    post_agc_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    pre_pll_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    post_pll_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    post_reco_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);

    progressCallback=pProgressCallback;
    doneCallback=pDoneCallback;
    constellationCallback=pConstellationCallback;
}

void QPSKDemodulatorDSP::destroyDSP()
{
    delete[] buffer;
    delete[] buffer2;
    delete[] buffer3;
    delete[] agc_buffer;
    delete[] agc_buffer2;
    delete[] filter_buffer;
    delete[] filter_buffer2;
    delete[] filter_buffer3;
    delete[] pll_buffer;
    delete[] recovery_buffer;
    delete[] recovered_buffer;
    delete[] buffer_i16;
    delete[] buffer_i8;
    delete[] buffer_u8;

    delete dc_pipe, pre_agc_pipe, post_agc_pipe, pre_pll_pipe, post_pll_pipe, post_reco_pipe;
}

void QPSKDemodulatorDSP::startDSP(
    std::string inputFilePath,
    std::string outputFilePath,
    bool optionF32,
    bool optionI16,
    bool optionI8,
    bool optionW8,
    int sampleRateEntry,
    int symbolRateEntry,
    float rrcAlphaEntry,
    float rrcTapsEntry,
    float loopBwEntry,
    bool hardSymbolsOption,
    bool dcBlockOption,
    bool aquaModeEntry,
    bool hrptModeEntry)
{
    data_in_filesize = 0;
    if (inputFilePath.compare("-") != 0)
    {
        data_in_filesize = getFilesize(inputFilePath);
        freopen(inputFilePath.c_str(), "rb", stdin); // redirects standard input
    }
    else
    {
#ifdef _WIN32
        _setmode(_fileno(stdin), O_BINARY);
#endif
    }
    if (outputFilePath.compare("-") != 0)
    {
        freopen(outputFilePath.c_str(), "wb", stdout); // redirects standard output
    }
    else
    {
#ifdef _WIN32
        _setmode(_fileno(stdout), O_BINARY);
#endif
    }

    f32 = optionF32;
    i16 = optionI16;
    i8 = optionI8;
    w8 = optionW8;

    samplerate = sampleRateEntry;
    symbolrate = symbolRateEntry;
    rrc_alpha = rrcAlphaEntry;
    rrc_taps = rrcTapsEntry;
    loop_bw = loopBwEntry;

    hard_symbs = hardSymbolsOption;
    dc_block = dcBlockOption;
    aquaMode = aquaModeEntry;
    hrptMode = hrptModeEntry;

    // Init our blocks
    agc = new libdsp::AgcCC(aquaMode ? 0.1f : hrptMode ? 0.00001f : 0.0001f, 1.0f, 1.0f, 65536); // Aqua requires a different mode...
    rrc_fir_filter = new libdsp::FIRFilterCCF(1, libdsp::firgen::root_raised_cosine(1, samplerate, symbolrate, rrc_alpha, rrc_taps));
    costas_loop = new libdsp::CostasLoop(loop_bw, 4);
    clock_recovery = aquaMode ? new libdsp::ClockRecoveryMMCC(samplerate / symbolrate, 0.8f, 0.9f, 0.037f, 0.0001f) : new libdsp::ClockRecoveryMMCC(samplerate / symbolrate, pow(8.7e-3, 2) / 4.0, 0.5f, 8.7e-3, 0.005f); // The one used for Aqua is different than normal QPSK
    dc_blocker = new libdsp::DCBlocker(1024, true);
    delay_one_imag = new DelayOneImag();

    // Start everything
    fileThread = new std::thread(&QPSKDemodulatorDSP::fileThreadFunction, this);
    if (dc_block)
        dcBlockThread = new std::thread(&QPSKDemodulatorDSP::dcBlockThreadFunction, this);
    agcThread = new std::thread(&QPSKDemodulatorDSP::agcThreadFunction, this);
    rrcThread = new std::thread(&QPSKDemodulatorDSP::rrcThreadFunction, this);
    pllThread = new std::thread(&QPSKDemodulatorDSP::pllThreadFunction, this);
    clockrecoveryThread = new std::thread(&QPSKDemodulatorDSP::clockrecoveryThreadFunction, this);
    finalWriteThread = new std::thread(&QPSKDemodulatorDSP::finalWriteThreadFunction, this);
}

void QPSKDemodulatorDSP::fileThreadFunction()
{
    while (!std::cin.eof())
    {
        
        // Get baseband, possibly convert to F32
        if (f32)
        {
            std::cin.read((char *)buffer, BUFFER_SIZE * sizeof(std::complex<float>));
        }
        else if (i16)
        {
            std::cin.read((char *)buffer_i16, BUFFER_SIZE * sizeof(int16_t) * 2);
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                using namespace std::complex_literals;
                buffer[i] = (float)buffer_i16[i * 2] + (float)buffer_i16[i * 2 + 1] * 1if;
            }
        }
        else if (i8)
        {
            std::cin.read((char *)buffer_i8, BUFFER_SIZE * sizeof(int8_t) * 2);
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                using namespace std::complex_literals;
                buffer[i] = (float)buffer_i8[i * 2] + (float)buffer_i8[i * 2 + 1] * 1if;
            }
        }
        else if (w8)
        {
            std::cin.read((char *)buffer_u8, BUFFER_SIZE * sizeof(uint8_t) * 2);
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                float imag = (buffer_u8[i * 2] - 127) * 0.004f;
                float real = (buffer_u8[i * 2 + 1] - 127) * 0.004f;
                using namespace std::complex_literals;
                buffer[i] = real + imag * 1if;
            }
        }

        if (dc_block)
            dc_pipe->push(buffer, BUFFER_SIZE);
        else
            pre_agc_pipe->push(buffer, BUFFER_SIZE);
    }
    //this is the only reliable way i found to stop the program
    usleep(1000000); //let the other threads finish?
    if (doneCallback)
        doneCallback(); //you better have a done callback
}

void QPSKDemodulatorDSP::agcThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = pre_agc_pipe->pop(agc_buffer, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        /// AGC
        agc->work(agc_buffer, gotten, agc_buffer);

        post_agc_pipe->push(agc_buffer, gotten);
    }
}

void QPSKDemodulatorDSP::rrcThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = post_agc_pipe->pop(agc_buffer2, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        // Root-raised-cosine filtering
        int out = rrc_fir_filter->work(agc_buffer2, gotten, filter_buffer);

        pre_pll_pipe->push(filter_buffer, out);
    }
}

void QPSKDemodulatorDSP::pllThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = pre_pll_pipe->pop(filter_buffer2, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        // Costas loop, frequency offset recovery
        costas_loop->work(filter_buffer2, gotten, filter_buffer2);

        if (aquaMode)
            delay_one_imag->work(filter_buffer2, gotten, filter_buffer3);

        post_pll_pipe->push(aquaMode ? filter_buffer3 : filter_buffer2, gotten);
    }
}

void QPSKDemodulatorDSP::clockrecoveryThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = post_pll_pipe->pop(pll_buffer, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        // Clock recovery
        int recovered_size = clock_recovery->work(pll_buffer, gotten, recovery_buffer);

        post_reco_pipe->push(recovery_buffer, recovered_size);
    }
}

void QPSKDemodulatorDSP::dcBlockThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = dc_pipe->pop(buffer2, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        // Block DC
        dc_blocker->work(buffer2, gotten, buffer3);

        pre_agc_pipe->push(buffer3, gotten);
    }
}

void QPSKDemodulatorDSP::finalWriteThreadFunction()
{
    int dat_size;
    while (true)
    {
        dat_size = post_reco_pipe->pop(recovered_buffer, BUFFER_SIZE);

        if (dat_size <= 0)
            continue;

        for (int i = 0; i < dat_size; i++)
        {
            int8_t symb_real = clamp(recovered_buffer[i].real() * 100);
            int8_t symb_imag = clamp(recovered_buffer[i].imag() * 100);

            if (constellationCallback && (i < 1024))
            {
                constellationCallback(symb_real, symb_imag, i);
            }

            if (!hard_symbs)
            {
                std::cout.put(symb_imag);
                std::cout.put(symb_real);
            }
        }

        if (hard_symbs)
            std::cout.write((char *)recovered_buffer, dat_size * sizeof(std::complex<float>));

        int percent = abs(round(((float)std::cin.tellg() / (float)data_in_filesize) * 1000.0f) / 10.0f);

        if (progressCallback)
            progressCallback(percent);
    }
}
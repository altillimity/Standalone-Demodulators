#include "window.h"

#include "utils.h"

void QPSKDemodulatorApp::initDSP()
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
}

void QPSKDemodulatorApp::destroyDSP()
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

void QPSKDemodulatorApp::startDSP()
{
    // Read startup variables
    data_in_filesize = getFilesize(inputFilePath);
    data_in = std::ifstream(inputFilePath, std::ios::binary);
    data_out = std::ofstream(outputFilePath, std::ios::binary);

    f32 = optionF32->GetValue();
    i16 = optionI16->GetValue();
    i8 = optionI8->GetValue();
    w8 = optionW8->GetValue();

    samplerate = std::stof((std::string)samplerateEntry->GetValue());
    symbolrate = std::stof((std::string)symbolrateEntry->GetValue());
    rrc_alpha = std::stof((std::string)rrcAlphaEntry->GetValue());
    rrc_taps = std::stof((std::string)rrcTapsEntry->GetValue());
    loop_bw = std::stof((std::string)loopBwEntry->GetValue());

    hard_symbs = hardSymbolsOption->IsChecked();
    dc_block = dcBlockOption->IsChecked();

    // Init our blocks
    agc = new libdsp::AgcCC(aquaMode ? 0.1f : hrptMode ? 0.00001f : 0.0001f, 1.0f, 1.0f, 65536); // Aqua requires a different mode...
    rrc_fir_filter = new libdsp::FIRFilterCCF(1, libdsp::firgen::root_raised_cosine(1, samplerate, symbolrate, rrc_alpha, rrc_taps));
    costas_loop = new libdsp::CostasLoop(loop_bw, 4);
    clock_recovery = aquaMode ? new libdsp::ClockRecoveryMMCC(samplerate / symbolrate, 0.8f, 0.9f, 0.037f, 0.0001f) : new libdsp::ClockRecoveryMMCC(samplerate / symbolrate, pow(8.7e-3, 2) / 4.0, 0.5f, 8.7e-3, 0.005f); // The one used for Aqua is different than normal QPSK
    dc_blocker = new libdsp::DCBlocker(1024, true);
    delay_one_imag = new DelayOneImag();

    // Start everything
    fileThread = new std::thread(&QPSKDemodulatorApp::fileThreadFunction, this);
    if (dc_block)
        dcBlockThread = new std::thread(&QPSKDemodulatorApp::dcBlockThreadFunction, this);
    agcThread = new std::thread(&QPSKDemodulatorApp::agcThreadFunction, this);
    rrcThread = new std::thread(&QPSKDemodulatorApp::rrcThreadFunction, this);
    pllThread = new std::thread(&QPSKDemodulatorApp::pllThreadFunction, this);
    clockrecoveryThread = new std::thread(&QPSKDemodulatorApp::clockrecoveryThreadFunction, this);
    finalWriteThread = new std::thread(&QPSKDemodulatorApp::finalWriteThreadFunction, this);
}

void QPSKDemodulatorApp::fileThreadFunction()
{
    while (!data_in.eof())
    {
        // Get baseband, possibly convert to F32
        if (f32)
        {
            data_in.read((char *)buffer, BUFFER_SIZE * sizeof(std::complex<float>));
        }
        else if (i16)
        {
            data_in.read((char *)buffer_i16, BUFFER_SIZE * sizeof(int16_t) * 2);
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                using namespace std::complex_literals;
                buffer[i] = (float)buffer_i16[i * 2] + (float)buffer_i16[i * 2 + 1] * 1if;
            }
        }
        else if (i8)
        {
            data_in.read((char *)buffer_i8, BUFFER_SIZE * sizeof(int8_t) * 2);
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                using namespace std::complex_literals;
                buffer[i] = (float)buffer_i8[i * 2] + (float)buffer_i8[i * 2 + 1] * 1if;
            }
        }
        else if (w8)
        {
            data_in.read((char *)buffer_u8, BUFFER_SIZE * sizeof(uint8_t) * 2);
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
}

void QPSKDemodulatorApp::agcThreadFunction()
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

void QPSKDemodulatorApp::rrcThreadFunction()
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

void QPSKDemodulatorApp::pllThreadFunction()
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

void QPSKDemodulatorApp::clockrecoveryThreadFunction()
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

void QPSKDemodulatorApp::dcBlockThreadFunction()
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

void QPSKDemodulatorApp::finalWriteThreadFunction()
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

            if (i < 1024)
            {
                drawPane->constellation_buffer[i * 2 + 1] = symb_real;
                drawPane->constellation_buffer[i * 2] = symb_imag;
            }

            if (!hard_symbs)
            {
                data_out.put(symb_imag);
                data_out.put(symb_real);
            }
        }

        if (hard_symbs)
            data_out.write((char *)recovered_buffer, dat_size * sizeof(std::complex<float>));

        int percent = abs(round(((float)data_in.tellg() / (float)data_in_filesize) * 1000.0f) / 10.0f);

        wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter([=]() {
            progressbar->SetValue(percent);
            progressLabel->SetLabelText(std::string("Progress : " + std::to_string(percent) + "%"));
            drawPane->Refresh();
        });
    }

    data_in.close();
    data_out.close();

    wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter([=]() {
        startButton->Enable();
    });
}
#include "window.h"
#include <functional>
#include "utils.h"
#include "dsp.h"

void CBPSKDemodulatorDsp::initDSP(
    std::function<void(int, int, int)> pProgressCallback,
    std::function<void(void)> pDoneCallback,
    std::function<void(int8_t, int8_t, int)> pConstellationCallback)
{
    // Buffers
    buffer = new std::complex<float>[BUFFER_SIZE * 10];
    agc_buffer = new std::complex<float>[BUFFER_SIZE * 10];
    front_rrc_buffer = new std::complex<float>[BUFFER_SIZE * 10];
    front_rrc_buffer2 = new std::complex<float>[BUFFER_SIZE * 10];
    pll_buffer = new std::complex<float>[BUFFER_SIZE * 10];
    pll_buffer2 = new float[BUFFER_SIZE * 10];
    rrc_buffer = new float[BUFFER_SIZE * 10];
    rrc_buffer2 = new float[BUFFER_SIZE * 10];
    recovery_buffer = new float[BUFFER_SIZE * 10];
    recovery_buffer2 = new float[BUFFER_SIZE * 10];
    recovered_buffer = new float[BUFFER_SIZE * 10];
    noise_buffer = new float[BUFFER_SIZE * 10];
    bitsBuffer = new uint8_t[BUFFER_SIZE * 10];
    buffer_i16 = new int16_t[BUFFER_SIZE * 2];
    buffer_i8 = new int8_t[BUFFER_SIZE * 2];
    buffer_u8 = new uint8_t[BUFFER_SIZE * 2];

    // Init FIFOs
    file_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    agc_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    front_rrc_pipe = new libdsp::Pipe<std::complex<float>>(BUFFER_SIZE);
    pll_pipe = new libdsp::Pipe<float>(BUFFER_SIZE);
    rrc_pipe = new libdsp::Pipe<float>(BUFFER_SIZE);
    recovery_pipe = new libdsp::Pipe<float>(BUFFER_SIZE);

    // Callbacks
    progressCallback = pProgressCallback;
    doneCallback = pDoneCallback;
    constellationCallback = pConstellationCallback;
}

void CBPSKDemodulatorDsp::destroyDSP()
{
    delete[] buffer;
    delete[] agc_buffer;
    delete[] front_rrc_buffer;
    delete[] front_rrc_buffer2;
    delete[] pll_buffer;
    delete[] pll_buffer2;
    delete[] rrc_buffer;
    delete[] rrc_buffer2;
    delete[] recovery_buffer;
    delete[] recovery_buffer2;
    delete[] recovered_buffer;
    delete[] noise_buffer;
    delete[] bitsBuffer;
    delete[] buffer_i16;
    delete[] buffer_i8;
    delete[] buffer_u8;

    delete file_pipe, agc_pipe, front_rrc_pipe, pll_pipe, pll_pipe, recovery_pipe;
}

void CBPSKDemodulatorDsp::startDSP(std::string inputFilePath,
                                   std::string outputFilePath,
                                   bool optionF32,
                                   bool optionI16,
                                   bool optionI8,
                                   bool optionW8,
                                   float samplerate,
                                   float symbolrate,
                                   float rrc_alpha,
                                   float rrc_taps,
                                   bool pNoaa_deframer,
                                   bool pRrc_filter)
{
    // Read startup variables
    data_in_filesize = getFilesize(inputFilePath);
    data_in = std::ifstream(inputFilePath, std::ios::binary);
    data_out = std::ofstream(outputFilePath, std::ios::binary);
    noaa_deframer = pNoaa_deframer;
    rrc_filter = pRrc_filter;

    // Init our blocks
    agc = new libdsp::AgcCC(meteorMode ? 0.0038e-3f : 0.002e-3f, 1.0f, 0.5f / 32768.0f, 65536);
    front_rrc_fir_filter = new libdsp::FIRFilterCCF(1, libdsp::firgen::root_raised_cosine(1, samplerate, symbolrate * 2.2, rrc_alpha, rrc_taps));
    carrier_pll = new libdsp::BPSKCarrierPLL(meteorMode ? 0.030f : 0.01f, meteorMode ? (powf(0.030f, 2) / 4.0f) : (powf(0.01f, 2) / 4.0f), meteorMode ? 0.5f : ((3 * M_PI * 100e3f) / samplerate));
    rrc_fir_filter = new libdsp::FIRFilterFFF(1, libdsp::firgen::root_raised_cosine(1, samplerate / 2, symbolrate, 0.5, 31));
    movingAverage = new libdsp::MovingAverageFF(round((samplerate / symbolrate) / 2.0), 1.0 / round((samplerate / symbolrate) / 2.0), BUFFER_SIZE, 1);
    clock_recovery = new libdsp::ClockRecoveryMMFF((samplerate / symbolrate) / 2.0f, meteorMode ? (powf(40e-3, 2) / 4.0f) : (powf(0.01, 2) / 4.0f), meteorMode ? 1.0f : 0.5f, meteorMode ? 40e-3 : 0.01f, meteorMode ? 0.01 : 100e-6f);
    noaa_deframer_blk = new NOAADeframer;
    noise_source = new libdsp::NoiseSource(libdsp::NS_GAUSSIAN, 0.2f, 0);

    // Start everything
    fileThread = new std::thread(&CBPSKDemodulatorDsp::fileThreadFunction, this);
    agcThread = new std::thread(&CBPSKDemodulatorDsp::agcThreadFunction, this);
    if (rrc_filter)
        frontRrcThread = new std::thread(&CBPSKDemodulatorDsp::frontRrcThreadFunction, this);
    rrcThread = new std::thread(&CBPSKDemodulatorDsp::rrcThreadFunction, this);
    pllThread = new std::thread(&CBPSKDemodulatorDsp::pllThreadFunction, this);
    clockrecoveryThread = new std::thread(&CBPSKDemodulatorDsp::clockrecoveryThreadFunction, this);
    finalWriteThread = new std::thread(&CBPSKDemodulatorDsp::finalWriteThreadFunction, this);
}

void CBPSKDemodulatorDsp::fileThreadFunction()
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

        file_pipe->push(buffer, BUFFER_SIZE);
    }
}

void CBPSKDemodulatorDsp::agcThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = file_pipe->pop(agc_buffer, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        /// AGC
        agc->work(agc_buffer, gotten, agc_buffer);

        agc_pipe->push(agc_buffer, gotten);
    }
}

void CBPSKDemodulatorDsp::frontRrcThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = agc_pipe->pop(front_rrc_buffer, 4000);

        if (gotten <= 0)
            continue;

        // Root-raised-cosine filtering
        int out = front_rrc_fir_filter->work(front_rrc_buffer, gotten, front_rrc_buffer2);

        front_rrc_pipe->push(front_rrc_buffer2, out);
    }
}

void CBPSKDemodulatorDsp::pllThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = rrc_filter ? front_rrc_pipe->pop(pll_buffer, BUFFER_SIZE) : agc_pipe->pop(pll_buffer, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        // Carrier PLL
        carrier_pll->work(pll_buffer, gotten, pll_buffer2);

        pll_pipe->push(pll_buffer2, gotten);
    }
}

void CBPSKDemodulatorDsp::rrcThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = pll_pipe->pop(rrc_buffer, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        int out;
        // Root-raised-cosine filtering or Moving AVG
        if (noaaMode)
            out = rrc_fir_filter->work(rrc_buffer, gotten, rrc_buffer2);
        else
            out = movingAverage->work(rrc_buffer, gotten, rrc_buffer2);

        rrc_pipe->push(rrc_buffer2, out);
    }
}

void CBPSKDemodulatorDsp::clockrecoveryThreadFunction()
{
    int gotten;
    while (true)
    {
        gotten = rrc_pipe->pop(recovery_buffer, BUFFER_SIZE);

        if (gotten <= 0)
            continue;

        // Clock recovery
        int recovered_size = clock_recovery->work(recovery_buffer, gotten, recovery_buffer2);

        recovery_pipe->push(recovery_buffer2, recovered_size);
    }
}

uint8_t byteToWrite;
int inByteToWrite = 0;

std::vector<uint8_t> getBytes(uint8_t *bits, int length)
{
    std::vector<uint8_t> bytesToRet;
    for (int ii = 0; ii < length; ii++)
    {
        byteToWrite = (byteToWrite << 1) | bits[ii];
        inByteToWrite++;

        if (inByteToWrite == 8)
        {
            bytesToRet.push_back(byteToWrite);
            inByteToWrite = 0;
        }
    }

    return bytesToRet;
}

void volk_32f_binary_slicer_8i_generic(int8_t *cVector, const float *aVector, unsigned int num_points)
{
    int8_t *cPtr = cVector;
    const float *aPtr = aVector;
    unsigned int number = 0;

    for (number = 0; number < num_points; number++)
    {
        if (*aPtr++ >= 0)
        {
            *cPtr++ = 1;
        }
        else
        {
            *cPtr++ = 0;
        }
    }
}

void CBPSKDemodulatorDsp::finalWriteThreadFunction()
{
    int dat_size, frame_count = 0;
    while (true)
    {
        dat_size = recovery_pipe->pop(recovered_buffer, BUFFER_SIZE);

        if (dat_size <= 0)
            continue;

        noise_source->work(noise_buffer, 1024);

        for (int i = 0; i < dat_size; i++)
        {
            int8_t symb_real = clamp(recovered_buffer[i] * 90);

            if (constellationCallback && (i < 1024))
            {
                constellationCallback(noise_buffer[i] * 80, symb_real, i);
            }
        }

        volk_32f_binary_slicer_8i_generic((int8_t *)bitsBuffer, recovered_buffer, dat_size);

        if (noaa_deframer)
        {
            std::vector<uint16_t> frames = noaa_deframer_blk->work(bitsBuffer, dat_size);

            // Count frames
            frame_count += frames.size();

            // Write to file
            if (frames.size() > 0)
                data_out.write((char *)&frames[0], frames.size() * sizeof(uint16_t));
        }
        else
        {
            std::vector<uint8_t> bytes = getBytes(bitsBuffer, dat_size);
            if (bytes.size() > 0)
                data_out.write((char *)&bytes[0], bytes.size());
        }

        if (progressCallback)
            progressCallback(data_in.tellg(), data_in_filesize, frame_count);
    }

    data_in.close();
    data_out.close();

    if (doneCallback)
        doneCallback();
}
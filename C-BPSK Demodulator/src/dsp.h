
#pragma once
#include <functional>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "constellation.h"
#include <fstream>
#include <thread>

#include <dsp/agc.h>
#include <dsp/fir_filter.h>
#include <dsp/fir_gen.h>
#include <dsp/carrier_pll_psk.h>
#include <dsp/clock_recovery_mm.h>
#include <dsp/moving_average.h>
#include <dsp/pipe.h>
#include <dsp/noise_source.h>
#include "dsp.h"
#include "dsp/noaa_deframer.h"
#include "utils.h"

class CBPSKDemodulatorDsp
{
public:
    void initDSP(
        std::function<void(size_t, size_t, size_t)> pProgressCallback,
        std::function<void(void)> pDoneCallback,
        std::function<void(int8_t, int8_t, int)> pConstellationCallback);
    void startDSP(std::string inputFilePath,
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
                  bool pRrc_filter);
    void destroyDSP();

private:
    void fileThreadFunction();
    void agcThreadFunction();
    void frontRrcThreadFunction();
    void pllThreadFunction();
    void rrcThreadFunction();
    void clockrecoveryThreadFunction();
    void finalWriteThreadFunction();
    std::function<void(int, int, int)> progressCallback;
    std::function<void(void)> doneCallback;
    std::function<void(int8_t, int8_t, int)> constellationCallback;

    bool noaa_deframer = false;
    bool rrc_filter = false;
    bool noaaMode = false, meteorMode = false;

private:
    std::thread *fileThread;
    std::thread *agcThread;
    std::thread *frontRrcThread;
    std::thread *pllThread;
    std::thread *rrcThread;
    std::thread *clockrecoveryThread;
    std::thread *finalWriteThread;
    bool f32, i16, i8, w8;
    float samplerate, symbolrate, rrc_alpha, rrc_taps;
    size_t data_in_filesize;
    std::ifstream data_in;
    std::ofstream data_out;

private:
    // All buffers we use along the way
    std::complex<float> *buffer;
    std::complex<float> *agc_buffer;
    std::complex<float> *front_rrc_buffer;
    std::complex<float> *front_rrc_buffer2;
    std::complex<float> *pll_buffer;
    float *pll_buffer2;
    float *rrc_buffer;
    float *rrc_buffer2;
    float *recovery_buffer;
    float *recovery_buffer2;
    float *recovered_buffer;
    float *noise_buffer;
    uint8_t *bitsBuffer;
    // Int16 buffer
    int16_t *buffer_i16;
    // Int8 buffer
    int8_t *buffer_i8;
    // Uint8 buffer
    uint8_t *buffer_u8;

private:
    // All blocks used along the way
    libdsp::AgcCC *agc;
    libdsp::FIRFilterCCF *front_rrc_fir_filter;
    libdsp::BPSKCarrierPLL *carrier_pll;
    libdsp::FIRFilterFFF *rrc_fir_filter;
    libdsp::MovingAverageFF *movingAverage;
    libdsp::ClockRecoveryMMFF *clock_recovery;
    libdsp::NoiseSource *noise_source;
    NOAADeframer *noaa_deframer_blk;

private:
    // All FIFOs we use along the way
    libdsp::Pipe<std::complex<float>> *file_pipe;
    libdsp::Pipe<std::complex<float>> *agc_pipe;
    libdsp::Pipe<std::complex<float>> *front_rrc_pipe;
    libdsp::Pipe<float> *pll_pipe;
    libdsp::Pipe<float> *rrc_pipe;
    libdsp::Pipe<float> *recovery_pipe;
};
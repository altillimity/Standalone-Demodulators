#pragma once

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
#include "dsp/noaa_deframer.h"

#define BUFFER_SIZE 4096

class CBPSKDemodulatorApp : public wxApp
{
public:
    virtual bool OnInit();
    ConstellationViewer *drawPane;
    wxGauge *progressbar;
    wxButton *selectInputButton, *selectOutputButton;
    wxStaticText *progressLabel;
    wxStaticText *inputLabel, *outputLabel;
    wxButton *startButton;

    wxStaticText *basebandTypeLabel;
    wxRadioButton *optionF32, *optionI16, *optionI8, *optionW8;

    wxStaticText *samplerateLabel, *symbolrateLabel, *rrcAlphaLabel, *rrcTapsLabel;
    wxTextCtrl *samplerateEntry, *symbolrateEntry, *rrcAlphaEntry, *rrcTapsEntry;

    wxCheckBox *noaaDeframerOption, *frontRRCOption;

    wxStaticText *presetsLabel;
    wxButton *presetNOAAHRPT, *presetMeteorHRPT;

private:
    std::string inputFilePath, outputFilePath;
    size_t data_in_filesize;
    std::ifstream data_in;
    std::ofstream data_out;
    bool noaaMode = false;
    bool meteorMode = false;
    bool f32, i16, i8, w8;
    float samplerate, symbolrate, rrc_alpha, rrc_taps;
    bool noaa_deframer, rrc_filter;

private:
    void initDSP();
    void startDSP();
    void destroyDSP();

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

private:
    void fileThreadFunction();
    void agcThreadFunction();
    void frontRrcThreadFunction();
    void pllThreadFunction();
    void rrcThreadFunction();
    void clockrecoveryThreadFunction();
    void finalWriteThreadFunction();

private:
    std::thread *fileThread;
    std::thread *agcThread;
    std::thread *frontRrcThread;
    std::thread *pllThread;
    std::thread *rrcThread;
    std::thread *clockrecoveryThread;
    std::thread *finalWriteThread;
};

class CBPSKDemodulatorFrame : public wxFrame
{
public:
    CBPSKDemodulatorFrame();
};
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
#include <dsp/costas_loop.h>
#include <dsp/clock_recovery_mm.h>
#include <dsp/dc_blocker.h>
#include <dsp/pipe.h>
#include "dsp/delay_one_imag.h"

#define BUFFER_SIZE 4096

class QPSKDemodulatorApp : public wxApp
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

    wxStaticText *samplerateLabel, *symbolrateLabel, *rrcAlphaLabel, *rrcTapsLabel, *loopBwLabel;
    wxTextCtrl *samplerateEntry, *symbolrateEntry, *rrcAlphaEntry, *rrcTapsEntry, *loopBwEntry;

    wxCheckBox *hardSymbolsOption, *dcBlockOption;

    wxStaticText *presetsLabel;
    wxButton *presetFY3BHRPT, *presetMetOpHRPT, *presetFy3BMPT, *presetFY3D, *presetHRD, *presetM2LRPT, *presetAqua;

private:
    std::string inputFilePath, outputFilePath;
    size_t data_in_filesize;
    std::ifstream data_in;
    std::ofstream data_out;
    bool aquaMode = false;
    bool hrptMode = false;
    bool f32, i16, i8, w8;
    float samplerate, symbolrate, rrc_alpha, rrc_taps, loop_bw;
    bool hard_symbs;
    bool dc_block;

private:
    void initDSP();
    void startDSP();
    void destroyDSP();

private:
    // All buffers we use along the way
    std::complex<float> *buffer;
    std::complex<float> *buffer2;
    std::complex<float> *buffer3;
    std::complex<float> *agc_buffer;
    std::complex<float> *agc_buffer2;
    std::complex<float> *filter_buffer;
    std::complex<float> *filter_buffer2;
    std::complex<float> *filter_buffer3;
    std::complex<float> *pll_buffer;
    std::complex<float> *recovery_buffer;
    std::complex<float> *recovered_buffer;
    // Int16 buffer
    int16_t *buffer_i16;
    // Int8 buffer
    int8_t *buffer_i8;
    // Uint8 buffer
    uint8_t *buffer_u8;

private:
    // All blocks used along the way
    libdsp::AgcCC *agc;
    libdsp::FIRFilterCCF *rrc_fir_filter;
    libdsp::CostasLoop *costas_loop;
    libdsp::ClockRecoveryMMCC *clock_recovery;
    libdsp::DCBlocker *dc_blocker;
    DelayOneImag *delay_one_imag;

private:
    // All FIFOs we use along the way
    libdsp::Pipe<std::complex<float>> *dc_pipe;
    libdsp::Pipe<std::complex<float>> *pre_agc_pipe;
    libdsp::Pipe<std::complex<float>> *post_agc_pipe;
    libdsp::Pipe<std::complex<float>> *pre_pll_pipe;
    libdsp::Pipe<std::complex<float>> *post_pll_pipe;
    libdsp::Pipe<std::complex<float>> *post_reco_pipe;

private:
    void fileThreadFunction();
    void agcThreadFunction();
    void rrcThreadFunction();
    void pllThreadFunction();
    void clockrecoveryThreadFunction();
    void dcBlockThreadFunction();
    void finalWriteThreadFunction();

private:
    std::thread *fileThread;
    std::thread *agcThread;
    std::thread *rrcThread;
    std::thread *pllThread;
    std::thread *clockrecoveryThread;
    std::thread *dcBlockThread;
    std::thread *finalWriteThread;
};

class QPSKDemodulatorFrame : public wxFrame
{
public:
    QPSKDemodulatorFrame();
};
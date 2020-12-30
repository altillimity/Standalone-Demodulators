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
    CBPSKDemodulatorDsp *dsp;
    std::string inputFilePath, outputFilePath;
    void updateProgress(size_t current, size_t total, size_t frame_count);
    void done();
    void updateConstellation(int8_t a, int8_t b, int i);
    bool noaaMode, meteorMode;
};

class CBPSKDemodulatorFrame : public wxFrame
{
public:
    CBPSKDemodulatorFrame();
};

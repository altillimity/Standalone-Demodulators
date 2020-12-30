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
#include "dsp.h"

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
    QPSKDemodulatorDSP *dsp;

private:
    bool aquaMode;
    bool hrptMode;
    std::string inputFilePath, outputFilePath;

    void updateProgress(int percent);
    void done();
    void updateConstellation(int8_t symb_real, int8_t symb_imag, int i);
};
class QPSKDemodulatorFrame : public wxFrame
{
public:
    QPSKDemodulatorFrame();
};
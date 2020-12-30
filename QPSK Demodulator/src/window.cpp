#include "window.h"

QPSKDemodulatorFrame::QPSKDemodulatorFrame() : wxFrame(NULL, wxID_ANY, "QPSK Demodulator (by Aang23)")
{
}

bool QPSKDemodulatorApp::OnInit()
{
    initDSP();

    QPSKDemodulatorFrame *frame = new QPSKDemodulatorFrame();
#ifdef __linux__
    frame->SetSize(800, 428);
#elif _WIN32
    frame->SetSize(820, 450);
#elif __APPLE__
    frame->SetSize(820, 450);
#endif

    // Constellation viewer
    drawPane = new ConstellationViewer((wxFrame *)frame);
    drawPane->SetSize(400, 400);

    // Progress stuff
    progressbar = new wxGauge((wxFrame *)frame, 0, 100, wxPoint(400, 390), wxSize(400, 10));
    progressLabel = new wxStaticText((wxFrame *)frame, 0, "Progress : 0%", wxPoint(402, 370));

    // Input / Output file stuff
    selectInputButton = new wxButton((wxFrame *)frame, 1, _T("Input File"), wxPoint(412, 295), wxDefaultSize, 0);
    selectOutputButton = new wxButton((wxFrame *)frame, 2, _T("Output File"), wxPoint(412, 330), wxDefaultSize, 0);
    inputLabel = new wxStaticText((wxFrame *)frame, 0, "No input file selected", wxPoint(502, 305));
    outputLabel = new wxStaticText((wxFrame *)frame, 0, "No output file selected", wxPoint(502, 340));

    // Symbolrate / Symbolrate entries
    samplerateLabel = new wxStaticText((wxFrame *)frame, 0, "Samplerate (Hz) :", wxPoint(412, 15));
    samplerateEntry = new wxTextCtrl((wxFrame *)frame, 0, "6000000", wxPoint(532, 10));
    symbolrateLabel = new wxStaticText((wxFrame *)frame, 0, "Symbolrate (Hz) :", wxPoint(412, 45));
    symbolrateEntry = new wxTextCtrl((wxFrame *)frame, 0, "2800000", wxPoint(532, 40));
    rrcAlphaLabel = new wxStaticText((wxFrame *)frame, 0, "RRC Alpha :", wxPoint(412, 75));
    rrcAlphaEntry = new wxTextCtrl((wxFrame *)frame, 0, "0.5", wxPoint(532, 70));
    rrcTapsLabel = new wxStaticText((wxFrame *)frame, 0, "RRC Taps :", wxPoint(412, 105));
    rrcTapsEntry = new wxTextCtrl((wxFrame *)frame, 0, "361", wxPoint(532, 100));
    loopBwLabel = new wxStaticText((wxFrame *)frame, 0, "Costas BW :", wxPoint(412, 135));
    loopBwEntry = new wxTextCtrl((wxFrame *)frame, 0, "0.005", wxPoint(532, 130));

    // Hard symbol option
    hardSymbolsOption = new wxCheckBox((wxFrame *)frame, 0, "Hard symbols", wxPoint(412, 160));
    dcBlockOption = new wxCheckBox((wxFrame *)frame, 0, "DC Blocker", wxPoint(542, 160));

    // Baseband type radio buttons
    basebandTypeLabel = new wxStaticText((wxFrame *)frame, 0, "Baseband type :", wxPoint(412, 190));
    wxFont font = basebandTypeLabel->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    basebandTypeLabel->SetFont(font);
    optionF32 = new wxRadioButton((wxFrame *)frame, 0, "32-bits float", wxPoint(412, 220));
    optionI16 = new wxRadioButton((wxFrame *)frame, 0, "16-bits int", wxPoint(412, 250));
    optionI8 = new wxRadioButton((wxFrame *)frame, 0, "8-bits int", wxPoint(532, 220));
    optionW8 = new wxRadioButton((wxFrame *)frame, 0, "8-bits wav", wxPoint(532, 250));

    // Presets
    presetsLabel = new wxStaticText((wxFrame *)frame, 0, "Presets", wxPoint(680, 7));
    font = presetsLabel->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    presetsLabel->SetFont(font);
    presetM2LRPT = new wxButton((wxFrame *)frame, 10, _T("M2 LRPT"), wxPoint(670, 30), wxDefaultSize, 0);
    presetMetOpHRPT = new wxButton((wxFrame *)frame, 11, _T("MetOp AHRPT"), wxPoint(670, 65), wxDefaultSize, 0);
    presetFY3BHRPT = new wxButton((wxFrame *)frame, 12, _T("FY3B AHRPT"), wxPoint(670, 100), wxDefaultSize, 0);
    presetFY3BHRPT = new wxButton((wxFrame *)frame, 17, _T("FY3C AHRPT"), wxPoint(670, 135), wxDefaultSize, 0);
    presetFy3BMPT = new wxButton((wxFrame *)frame, 13, _T("FY3B MPT"), wxPoint(670, 170), wxDefaultSize, 0);
    presetFY3D = new wxButton((wxFrame *)frame, 14, _T("FY3D AHRPT"), wxPoint(670, 205), wxDefaultSize, 0);
    presetHRD = new wxButton((wxFrame *)frame, 15, _T("NPP/JPSS HRD"), wxPoint(670, 240), wxDefaultSize, 0);
    presetAqua = new wxButton((wxFrame *)frame, 16, _T("Aqua DB"), wxPoint(670, 275), wxDefaultSize, 0);

    // Start button
    startButton = new wxButton((wxFrame *)frame, 6, _T("Start"), wxPoint(710, 355), wxDefaultSize, 0);

    // Bind buttons
    Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
        if (event.GetId() == 1)
        {
            wxFileDialog saveFileDialog((wxFrame *)frame, _("Select baseband file"), "", "", "Baseband|*", wxFD_OPEN);
            if (saveFileDialog.ShowModal() == wxID_CANCEL)
                return;

            inputFilePath = saveFileDialog.GetPath().ToStdString();
            inputLabel->SetLabelText(inputFilePath);
        }
        else if (event.GetId() == 2)
        {
            wxFileDialog saveFileDialog((wxFrame *)frame, _("Select output symbols file"), "", "", "Hard / Soft symbols|*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (saveFileDialog.ShowModal() == wxID_CANCEL)
                return;

            outputFilePath = saveFileDialog.GetPath().ToStdString();
            outputLabel->SetLabelText(outputFilePath);
        }
        else if (event.GetId() == 10)
        {
            samplerateEntry->SetValue("140000");
            symbolrateEntry->SetValue("72000");
            rrcAlphaEntry->SetValue("0.5");
            rrcTapsEntry->SetValue("511");
            loopBwEntry->SetValue("0.005");
            hardSymbolsOption->SetValue(false);
            optionI16->SetValue(true);
            dcBlockOption->SetValue(false);
            aquaMode = false;
            hrptMode = true;
        }
        else if (event.GetId() == 11)
        {
            samplerateEntry->SetValue("6000000");
            symbolrateEntry->SetValue("2333333");
            rrcAlphaEntry->SetValue("0.5");
            rrcTapsEntry->SetValue("31");
            loopBwEntry->SetValue("0.002");
            hardSymbolsOption->SetValue(false);
            optionI16->SetValue(true);
            dcBlockOption->SetValue(false);
            aquaMode = false;
            hrptMode = true;
        }
        else if (event.GetId() == 12)
        {
            samplerateEntry->SetValue("6000000");
            symbolrateEntry->SetValue("2800000");
            rrcAlphaEntry->SetValue("0.5");
            rrcTapsEntry->SetValue("31");
            loopBwEntry->SetValue("0.002");
            hardSymbolsOption->SetValue(false);
            optionI16->SetValue(true);
            dcBlockOption->SetValue(false);
            aquaMode = false;
            hrptMode = true;
        }
        else if (event.GetId() == 17)
        {
            samplerateEntry->SetValue("6000000");
            symbolrateEntry->SetValue("2600000");
            rrcAlphaEntry->SetValue("0.5");
            rrcTapsEntry->SetValue("31");
            loopBwEntry->SetValue("0.002");
            hardSymbolsOption->SetValue(false);
            optionI16->SetValue(true);
            dcBlockOption->SetValue(false);
            aquaMode = false;
            hrptMode = true;
        }
        else if (event.GetId() == 13)
        {
            samplerateEntry->SetValue("30000000");
            symbolrateEntry->SetValue("18700000");
            rrcAlphaEntry->SetValue("0.35");
            rrcTapsEntry->SetValue("31");
            loopBwEntry->SetValue("0.0063");
            hardSymbolsOption->SetValue(false);
            optionW8->SetValue(true);
            dcBlockOption->SetValue(false);
            aquaMode = false;
            hrptMode = false;
        }
        else if (event.GetId() == 14)
        {
            samplerateEntry->SetValue("45000000");
            symbolrateEntry->SetValue("30000000");
            rrcAlphaEntry->SetValue("0.35");
            rrcTapsEntry->SetValue("31");
            loopBwEntry->SetValue("0.0063");
            hardSymbolsOption->SetValue(false);
            optionW8->SetValue(true);
            dcBlockOption->SetValue(false);
            aquaMode = false;
            hrptMode = false;
        }
        else if (event.GetId() == 15)
        {
            samplerateEntry->SetValue("25000000");
            symbolrateEntry->SetValue("15000000");
            rrcAlphaEntry->SetValue("0.7");
            rrcTapsEntry->SetValue("31");
            loopBwEntry->SetValue("0.001");
            hardSymbolsOption->SetValue(false);
            optionW8->SetValue(true);
            dcBlockOption->SetValue(false);
            aquaMode = false;
            hrptMode = false;
        }
        else if (event.GetId() == 16)
        {
            samplerateEntry->SetValue("15000000");
            symbolrateEntry->SetValue("7500000");
            rrcAlphaEntry->SetValue("0.5");
            rrcTapsEntry->SetValue("31");
            loopBwEntry->SetValue("0.006");
            hardSymbolsOption->SetValue(false);
            optionW8->SetValue(true);
            dcBlockOption->SetValue(true);
            aquaMode = true;
            hrptMode = false;
        }
        else if (event.GetId() == 6)
        {
            startDSP();
            startButton->Disable();
        }
    });

    //frame->SetSizer(sizer);
    //frame->SetAutoLayout(true);

    frame->Show(true);

    return true;
}

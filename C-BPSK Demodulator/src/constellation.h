#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class ConstellationViewer : public wxPanel
{

public:
    ConstellationViewer(wxFrame *parent);

    void paintEvent(wxPaintEvent &evt);
    void render(wxDC &dc);
    void OnEraseBackGround(wxEraseEvent &event){};

    DECLARE_EVENT_TABLE()

public:
    int8_t *constellation_buffer;
};
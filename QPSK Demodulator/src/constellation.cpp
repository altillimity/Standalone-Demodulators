#include "constellation.h"

#include <thread>

BEGIN_EVENT_TABLE(ConstellationViewer, wxPanel)
EVT_PAINT(ConstellationViewer::paintEvent)
EVT_ERASE_BACKGROUND(ConstellationViewer::OnEraseBackGround)
END_EVENT_TABLE()

ConstellationViewer::ConstellationViewer(wxFrame *parent) : wxPanel(parent)
{
    constellation_buffer = new int8_t[1024 * 2];
}

void ConstellationViewer::render(wxDC &dc)
{
    dc.SetPen(wxPen(wxColor(255, 255, 255), 2));
    dc.SetBrush(*wxBLACK_BRUSH);
    dc.DrawRectangle(0, 0, 400, 400);


    dc.SetPen(wxPen(wxColor(100, 100, 100), 1));
    dc.DrawLine(200, 0, 200, 400);
    dc.DrawLine(0, 200, 400, 200);

    dc.SetBrush(*wxGREEN_BRUSH);
    dc.SetPen(*wxGREEN_PEN);
    for (int i = 0; i < 1024; i++)
    {
        int pos1 = (((float)constellation_buffer[i * 2] + (255 / 2)) / 255.5f) * 400;     //rand() % 400 + 1;
        int pos2 = (((float)constellation_buffer[i * 2 + 1] + (255 / 2)) / 255.5f) * 400; //rand() % 400 + 1;
        dc.DrawCircle(wxPoint(pos2, pos1), 2);
    }
}

void ConstellationViewer::paintEvent(wxPaintEvent &evt)
{
    wxPaintDC dc(this);
    render(dc);
}
/////////////////////////////////////////////////////////////////////////////
// Name:        contrib/samples/ogl/ogledit/ogledit.h
// Purpose:     OGL sample
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGLSAMPLE_OGLEDIT_H_
#define _OGLSAMPLE_OGLEDIT_H_

#include "wx/docview.h"
#include "wx/ogl/ogl.h"

// Define a new application
class MyFrame;
class EditorToolPalette;
class MyApp: public wxApp
{
  public:
    MyFrame *frame;
    wxDocManager* myDocManager;

    MyApp(void);
    bool OnInit(void);
    int OnExit(void);

    // Palette stuff
    EditorToolPalette *CreatePalette(wxFrame *parent);
};

DECLARE_APP(MyApp)

// Define a new frame
class MyCanvas;
class MyFrame: public wxDocParentFrame
{
    DECLARE_CLASS(MyFrame)
public:
    wxMenu *editMenu;

    MyCanvas *canvas;
    EditorToolPalette *palette;

    MyFrame(wxDocManager *manager, wxFrame *parent, const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE);
    virtual ~MyFrame();

    void OnSize(wxSizeEvent& event);
    void OnAbout(wxCommandEvent& event);

DECLARE_EVENT_TABLE()
};

extern MyFrame *GetMainFrame(void);

// Menu/undo/redo commands

enum
{
    OGLEDIT_ADD_SHAPE = wxID_HIGHEST,
    OGLEDIT_ADD_LINE,
    OGLEDIT_EDIT_LABEL,
    OGLEDIT_CHANGE_BACKGROUND_COLOUR,
    OGLEDIT_ABOUT = wxID_ABOUT
};

#endif
    // _OGLSAMPLE_OGLEDIT_H_

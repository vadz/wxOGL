/////////////////////////////////////////////////////////////////////////////
// Name:        samples/ogl/ogledit/palette.cpp
// Purpose:     OGLEdit palette
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/toolbar.h"

#include "palette.h"
#include "ogledit.h"

// Include pixmaps
#include "bitmaps/arrow.xpm"
#include "bitmaps/tool1.xpm"
#include "bitmaps/tool2.xpm"
#include "bitmaps/tool3.xpm"
#include "bitmaps/tool4.xpm"

/*
 * Object editor tool palette
 *
 */

EditorToolPalette::EditorToolPalette(wxWindow* parent, const wxPoint& pos, const wxSize& size,
            long style):
  wxToolBar(parent, wxID_ANY, pos, size, style)
{
}

EditorToolPalette *MyApp::CreatePalette(wxFrame *parent)
{
  // Load palette bitmaps.
  wxBitmap PaletteTool1(tool1_xpm);
  wxBitmap PaletteTool2(tool2_xpm);
  wxBitmap PaletteTool3(tool3_xpm);
  wxBitmap PaletteTool4(tool4_xpm);
  wxBitmap PaletteArrow(arrow_xpm);

  EditorToolPalette *palette = new EditorToolPalette(parent, wxPoint(0, 0), wxDefaultSize,
      wxTB_VERTICAL);

  palette->AddRadioTool(PALETTE_ARROW, "", PaletteArrow, wxNullBitmap, _T("Pointer"));
  palette->AddRadioTool(PALETTE_TOOL1, "", PaletteTool1, wxNullBitmap, _T("Tool 1"));
  palette->AddRadioTool(PALETTE_TOOL2, "", PaletteTool2, wxNullBitmap, _T("Tool 2"));
  palette->AddRadioTool(PALETTE_TOOL3, "", PaletteTool3, wxNullBitmap, _T("Tool 3"));
  palette->AddRadioTool(PALETTE_TOOL4, "", PaletteTool4, wxNullBitmap, _T("Tool 4"));

  palette->Realize();

  palette->ToggleTool(PALETTE_ARROW, true);
  return palette;
}

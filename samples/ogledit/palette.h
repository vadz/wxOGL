/////////////////////////////////////////////////////////////////////////////
// Name:        contrib/samples/ogl/ogledit/palette.h
// Purpose:     OGL sample palette
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGLSAMPLE_PALETTE_H_
#define _OGLSAMPLE_PALETTE_H_

#include "wx/toolbar.h"

/*
 * Object editor tool palette
 *
 */

class EditorToolPalette: public wxToolBar
{
 public:
  EditorToolPalette(wxWindow *parent, const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = wxTB_VERTICAL);
};

#define PALETTE_TOOL1           1
#define PALETTE_TOOL2           2
#define PALETTE_TOOL3           3
#define PALETTE_TOOL4           4
#define PALETTE_TOOL5           5
#define PALETTE_TOOL6           6
#define PALETTE_TOOL7           7
#define PALETTE_TOOL8           8
#define PALETTE_TOOL9           9
#define PALETTE_ARROW           10

#endif
    // _OGLSAMPLE_PALETTE_H_

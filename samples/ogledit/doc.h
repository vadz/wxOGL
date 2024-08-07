/////////////////////////////////////////////////////////////////////////////
// Name:        contrib/samples/ogl/ogledit/doc.h
// Purpose:     Document classes
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGLSAMPLE_DOC_H_
#define _OGLSAMPLE_DOC_H_

#include "wx/docview.h"
#include "wx/cmdproc.h"
#include "wx/string.h"

#include "wx/ogl/ogl.h"

#include "wx/ogl/ogl.h"

#if wxUSE_STD_IOSTREAM
 #include <iosfwd>
#endif

/*
 * Override a few members for this application
 */

class MyDiagram: public wxDiagram
{
 public:
  MyDiagram(void) {}
};

/*
 * A few new shape classes so we have a 1:1 mapping
 * between palette symbol and unique class
 */

class wxRoundedRectangleShape: public wxRectangleShape
{
  DECLARE_DYNAMIC_CLASS(wxRoundedRectangleShape)
 private:
 public:
  wxRoundedRectangleShape(double w = 0.0, double h = 0.0);
};

class wxDiamondShape: public wxPolygonShape
{
  DECLARE_DYNAMIC_CLASS(wxDiamondShape)
 private:
 public:
  wxDiamondShape(double w = 0.0, double h = 0.0);
};

/*
 * All shape event behaviour is routed through this handler, so we don't
 * have to derive from each shape class. We plug this in to each shape.
 */

class MyEvtHandler: public wxShapeEvtHandler
{
 public:
  wxString label;
  MyEvtHandler(wxShapeEvtHandler *prev = NULL, wxShape *shape = NULL, const wxString& lab = wxEmptyString):wxShapeEvtHandler(prev, shape)
  {
    label = lab;
  }
  ~MyEvtHandler(void)
  {
  }
  void OnLeftClick(double x, double y, int keys = 0, int attachment = 0);
  void OnBeginDragRight(double x, double y, int keys = 0, int attachment = 0);
  void OnDragRight(bool draw, double x, double y, int keys = 0, int attachment = 0);
  void OnEndDragRight(double x, double y, int keys = 0, int attachment = 0);
  void OnEndSize(double x, double y);
};

/*
 * A diagram document, which contains a diagram.
 */

class DiagramDocument: public wxDocument
{
  DECLARE_DYNAMIC_CLASS(DiagramDocument)
 private:
 public:
  MyDiagram diagram;

  DiagramDocument(void);
  ~DiagramDocument(void);

#if wxUSE_STD_IOSTREAM
    virtual wxSTD ostream& SaveObject(wxSTD ostream& stream);
    virtual wxSTD istream& LoadObject(wxSTD istream& stream);
#else
    virtual wxOutputStream& SaveObject(wxOutputStream& stream);
    virtual wxInputStream& LoadObject(wxInputStream& stream);
#endif

  inline wxDiagram *GetDiagram() { return &diagram; }

  bool OnCloseDocument(void);
};

/*
 * Most user interface commands are routed through this, to give us the
 * Undo/Redo mechanism. If you add more commands, such as changing the shape colour,
 * you will need to add members to 'remember' what the user applied (for 'Do') and what the
 * previous state was (for 'Undo').
 * You can have one member for each property to be changed. Assume we also have
 * a pointer member wxShape *shape, which is set to the shape being changed.
 * Let's assume we're changing the shape colour. Our member for this is shapeColour.
 *
 * - In 'Do':
 *   o Set a temporary variable 'temp' to the current colour for 'shape'.
 *   o Change the colour to the new colour.
 *   o Set shapeColour to the _old_ colour, 'temp'.
 * - In 'Undo':
 *   o Set a temporary variable 'temp' to the current colour for 'shape'.
 *   o Change the colour to shapeColour (the old colour).
 *   o Set shapeColour to 'temp'.
 *
 * So, as long as we have a pointer to the shape being changed,
 * we only need one member variable for each property.
 *
 * PROBLEM: when an Add shape command is redone, the 'shape' pointer changes.
 * Assume, as here, that we keep a pointer to the old shape so we reuse it
 * when we recreate.
 */

class DiagramCommand: public wxCommand
{
private:
  // Constructor providing the required parameters for all commands.
  DiagramCommand(const wxString& name, int command, DiagramDocument* ddoc);

 protected:
  DiagramDocument* const doc;
  int const cmd;
  wxShape *shape = NULL; // Pointer to the shape we're acting on
  wxShape *fromShape = NULL;
  int attachmentFrom = 0;
  wxShape *toShape = NULL;
  int attachmentTo = 0;
  wxClassInfo *shapeInfo = NULL;
  double x = 0.;
  double y = 0.;
  bool selected = false;
  bool deleteShape = false;

  // Storage for property commands
  const wxBrush *shapeBrush = NULL;
  wxPen *shapePen = NULL;
  wxString shapeLabel;
 public:
  // Static constructors for various operations.
  static DiagramCommand* AddShape(DiagramDocument* ddoc, wxClassInfo* ci, double x, double y);
  static DiagramCommand* AddLine(DiagramDocument* ddoc, wxShape* from, int attachmentFrom, wxShape* to, int attachmentTo);
  static DiagramCommand* Cut(DiagramDocument* ddoc, bool sel, wxShape* theShape);
  static DiagramCommand* ChangeColour(DiagramDocument *ddoc, wxBrush *backgroundColour, wxShape *theShape);
  static DiagramCommand* ChangeLabel(DiagramDocument *ddoc, const wxString& lab, wxShape *theShape);

  ~DiagramCommand(void);

  bool Do(void);
  bool Undo(void);

  inline void SetShape(wxShape *s) { shape = s; }
  inline wxShape *GetShape(void) { return shape; }
  inline wxShape *GetFromShape(void) { return fromShape; }
  inline wxShape *GetToShape(void) { return toShape; }
  inline wxClassInfo *GetShapeInfo(void) { return shapeInfo; }
  inline bool GetSelected(void) { return selected; }

  void RemoveLines(wxShape *shape);
};

#endif
  // _OGLSAMPLE_DOC_H_

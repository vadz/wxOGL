/////////////////////////////////////////////////////////////////////////////
// Name:        contrib/samples/ogl/ogledit/doc.cpp
// Purpose:     Implements document functionality in OGLEdit
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

#include "wx/ioswrap.h"

#if !wxUSE_DOC_VIEW_ARCHITECTURE
#error You must set wxUSE_DOC_VIEW_ARCHITECTURE to 1 in wx_setup.h!
#endif

#include "ogledit.h"
#include "doc.h"
#include "view.h"

IMPLEMENT_DYNAMIC_CLASS(DiagramDocument, wxDocument)

DiagramDocument::DiagramDocument(void)
{
}

DiagramDocument::~DiagramDocument(void)
{
}

bool DiagramDocument::OnCloseDocument(void)
{
    diagram.DeleteAllShapes();
    return true;
}

wxSTD ostream& DiagramDocument::SaveObject(wxSTD ostream& stream)
{
    wxLogWarning("Sorry, saving is not implemented yet");

    return stream;
}

wxSTD istream& DiagramDocument::LoadObject(wxSTD istream& stream)
{
    wxLogWarning("Sorry, loading is not implemented yet");

    return stream;
}

/*
 * Implementation of drawing command
 */

DiagramCommand::DiagramCommand(const wxString& name, int command, DiagramDocument *ddoc)
               :wxCommand(true, name),
                doc(ddoc),
                cmd(command)
{
}

/* static */
DiagramCommand* DiagramCommand::AddShape(DiagramDocument* ddoc, wxClassInfo* ci, double x, double y)
{
    auto const command = new DiagramCommand(ci->GetClassName(), OGLEDIT_ADD_SHAPE, ddoc);
    command->shapeInfo = ci;
    command->x = x;
    command->y = y;

    return command;
}

/* static */
DiagramCommand*
DiagramCommand::AddLine(DiagramDocument* ddoc,
                        wxShape* from, int attachmentFrom,
                        wxShape* to, int attachmentTo)
{
    auto const command = new DiagramCommand(_T("wxLineShape"), OGLEDIT_ADD_LINE, ddoc);
    command->shapeInfo = wxCLASSINFO(wxLineShape);
    command->fromShape = from;
    command->attachmentFrom = attachmentFrom;
    command->toShape = to;
    command->attachmentTo = attachmentTo;

    return command;
}

/* static */
DiagramCommand* DiagramCommand::Cut(DiagramDocument* ddoc, bool sel, wxShape* theShape)
{
    auto const command = new DiagramCommand(_T("Cut"), wxID_CUT, ddoc);
    command->selected = sel;
    command->shape = theShape;

    return command;
}

/* static */
DiagramCommand* DiagramCommand::ChangeColour(DiagramDocument* ddoc, wxBrush* brush, wxShape* theShape)
{
    auto const command = new DiagramCommand(_T("Change colour"), OGLEDIT_CHANGE_BACKGROUND_COLOUR, ddoc);
    command->shape = theShape;
    command->shapeBrush = brush;

    return command;
}

/* static */
DiagramCommand* DiagramCommand::ChangeLabel(DiagramDocument* ddoc, const wxString& label, wxShape* theShape)
{
    auto const command = new DiagramCommand(_T("Edit label"), OGLEDIT_EDIT_LABEL, ddoc);
    command->shape = theShape;
    command->shapeLabel = label;

    return command;
}

DiagramCommand::~DiagramCommand(void)
{
  if (shape && deleteShape)
  {
    shape->SetCanvas(NULL);
    delete shape;
  }
}

bool DiagramCommand::Do(void)
{
  switch (cmd)
  {
    case wxID_CUT:
    {
      if (shape)
      {
        deleteShape = true;

        shape->Select(false);

        // Generate commands to explicitly remove each connected line.
        RemoveLines(shape);

        doc->GetDiagram()->RemoveShape(shape);
        if (shape->IsKindOf(CLASSINFO(wxLineShape)))
        {
          wxLineShape *lineShape = (wxLineShape *)shape;
          fromShape = lineShape->GetFrom();
          toShape = lineShape->GetTo();
        }
        shape->Unlink();

        doc->Modify(true);
        doc->UpdateAllViews();
      }

      break;
    }
    case OGLEDIT_ADD_SHAPE:
    {
      wxShape *theShape;
      if (shape)
        theShape = shape; // Saved from undoing the shape
      else
      {
        theShape = (wxShape *)shapeInfo->CreateObject();
        theShape->AssignNewIds();
        theShape->SetEventHandler(new MyEvtHandler(theShape, theShape, wxEmptyString));
        theShape->SetCentreResize(false);
        theShape->SetPen(wxBLACK_PEN);
        theShape->SetBrush(wxCYAN_BRUSH);

        theShape->SetSize(60, 60);
      }
      doc->GetDiagram()->AddShape(theShape);
      theShape->Show(true);

      wxClientDC dc(theShape->GetCanvas());
      theShape->GetCanvas()->PrepareDC(dc);

      theShape->Move(dc, x, y);

      shape = theShape;
      deleteShape = false;

      doc->Modify(true);
      doc->UpdateAllViews();
      break;
    }
    case OGLEDIT_ADD_LINE:
    {
      wxShape *theShape;
      if (shape)
        theShape = shape; // Saved from undoing the line
      else
      {
        theShape = (wxShape *)shapeInfo->CreateObject();
        theShape->AssignNewIds();
        theShape->SetEventHandler(new MyEvtHandler(theShape, theShape, wxEmptyString));
        theShape->SetPen(wxBLACK_PEN);
        theShape->SetBrush(wxRED_BRUSH);

        wxLineShape *lineShape = (wxLineShape *)theShape;

        lineShape->SetAttachments(attachmentFrom, attachmentTo);

        // Yes, you can have more than 2 control points, in which case
        // it becomes a multi-segment line.
        lineShape->MakeLineControlPoints(2);
        lineShape->AddArrow(ARROW_ARROW, ARROW_POSITION_END, 10.0, 0.0, _T("Normal arrowhead"));
      }

      doc->GetDiagram()->AddShape(theShape);

      fromShape->AddLine((wxLineShape *)theShape, toShape);

      theShape->Show(true);

      wxClientDC dc(theShape->GetCanvas());
      theShape->GetCanvas()->PrepareDC(dc);

      shape = theShape;
      deleteShape = false;

      doc->Modify(true);
      doc->UpdateAllViews();
      break;
    }
    case OGLEDIT_CHANGE_BACKGROUND_COLOUR:
    {
      if (shape)
      {
        wxClientDC dc(shape->GetCanvas());
        shape->GetCanvas()->PrepareDC(dc);

        const wxBrush *oldBrush = shape->GetBrush();
        shape->SetBrush(shapeBrush);
        shapeBrush = oldBrush;
        shape->Draw(dc);

        doc->Modify(true);
        doc->UpdateAllViews();
      }

      break;
    }
    case OGLEDIT_EDIT_LABEL:
    {
      if (shape)
      {
        MyEvtHandler *myHandler = (MyEvtHandler *)shape->GetEventHandler();
        wxString oldLabel(myHandler->label);
        myHandler->label = shapeLabel;
        shapeLabel = oldLabel;

        wxClientDC dc(shape->GetCanvas());
        shape->GetCanvas()->PrepareDC(dc);

        shape->FormatText(dc, /* (char*) (const char*) */ myHandler->label);
        shape->Draw(dc);

        doc->Modify(true);
        doc->UpdateAllViews();
      }

      break;
    }
  }
  return true;
}

bool DiagramCommand::Undo(void)
{
  switch (cmd)
  {
    case wxID_CUT:
    {
      if (shape)
      {
        doc->GetDiagram()->AddShape(shape);
        shape->Show(true);

        if (shape->IsKindOf(CLASSINFO(wxLineShape)))
        {
          wxLineShape *lineShape = (wxLineShape *)shape;

          fromShape->AddLine(lineShape, toShape);
        }
        if (selected)
          shape->Select(true);

        deleteShape = false;
      }
      doc->Modify(true);
      doc->UpdateAllViews();
      break;
    }
    case OGLEDIT_ADD_SHAPE:
    case OGLEDIT_ADD_LINE:
    {
      if (shape)
      {
        wxClientDC dc(shape->GetCanvas());
        shape->GetCanvas()->PrepareDC(dc);

        shape->Select(false, &dc);
        doc->GetDiagram()->RemoveShape(shape);
        shape->Unlink();
        deleteShape = true;
      }
      doc->Modify(true);
      doc->UpdateAllViews();
      break;
    }
    case OGLEDIT_CHANGE_BACKGROUND_COLOUR:
    {
      if (shape)
      {
        wxClientDC dc(shape->GetCanvas());
        shape->GetCanvas()->PrepareDC(dc);

        const wxBrush *oldBrush = shape->GetBrush();
        shape->SetBrush(shapeBrush);
        shapeBrush = oldBrush;
        shape->Draw(dc);

        doc->Modify(true);
        doc->UpdateAllViews();
      }
      break;
    }
    case OGLEDIT_EDIT_LABEL:
    {
      if (shape)
      {
        MyEvtHandler *myHandler = (MyEvtHandler *)shape->GetEventHandler();
        wxString oldLabel(myHandler->label);
        myHandler->label = shapeLabel;
        shapeLabel = oldLabel;

        wxClientDC dc(shape->GetCanvas());
        shape->GetCanvas()->PrepareDC(dc);

        shape->FormatText(dc, /* (char*) (const char*) */ myHandler->label);
        shape->Draw(dc);

        doc->Modify(true);
        doc->UpdateAllViews();
      }

      break;
    }
  }
  return true;
}

// Remove each individual line connected to a shape by sending a command.
void DiagramCommand::RemoveLines(wxShape *shapeFrom)
{
  wxObjectList::compatibility_iterator node = shapeFrom->GetLines().GetFirst();
  while (node)
  {
    wxLineShape *line = (wxLineShape *)node->GetData();
    doc->GetCommandProcessor()->Submit(DiagramCommand::Cut(doc, line->Selected(), line));

    node = shapeFrom->GetLines().GetFirst();
  }
}

/*
 * MyEvtHandler: an event handler class for all shapes
 */

void MyEvtHandler::OnLeftClick(double WXUNUSED(x), double WXUNUSED(y), int keys, int WXUNUSED(attachment))
{
  if (keys == 0)
  {
    // Selection is a concept the library knows about
    if (GetShape()->Selected())
    {
      GetShape()->Select(false);
      GetShape()->GetCanvas()->Refresh(); // Redraw because bits of objects will be are missing
    }
    else
    {
      // Ensure no other shape is selected, to simplify Undo/Redo code
      wxObjectList::compatibility_iterator node = GetShape()->GetCanvas()->GetDiagram()->GetShapeList()->GetFirst();
      while (node)
      {
        wxShape *eachShape = (wxShape *)node->GetData();
        if (eachShape->GetParent() == NULL)
        {
          if (eachShape->Selected())
          {
            eachShape->Select(false);
          }
        }
        node = node->GetNext();
      }
      GetShape()->Select(true);
      GetShape()->GetCanvas()->Refresh();
    }
  }
  else if (keys & KEY_CTRL)
  {
    // Do something for CONTROL
  }
  else
  {
#if wxUSE_STATUSBAR
    wxGetApp().frame->SetStatusText(label);
#endif // wxUSE_STATUSBAR
  }
}

/*
 * Implement connection of two shapes by right-dragging between them.
 */

void MyEvtHandler::OnBeginDragRight(double x, double y, int keys, int attachment)
{
  GetShape()->GetCanvas()->CaptureMouse();

  wxLogStatus("Create new connection by dragging from this shape to another");

  OnDragRight(true, x, y, keys, attachment);
}

void MyEvtHandler::OnDragRight(bool draw, double x, double y, int WXUNUSED(keys), int attachment)
{
  if (!draw)
      return;

  wxShapeCanvasOverlay overlay(GetShape()->GetCanvas());
  wxDC& dc = overlay.GetDC();

  wxPen dottedPen(*wxBLACK, 1, wxPENSTYLE_DOT);
  dc.SetPen(dottedPen);
  double xp, yp;
  GetShape()->GetAttachmentPosition(attachment, &xp, &yp);
  dc.DrawLine((long) xp, (long) yp, (long) x, (long) y);
}

void MyEvtHandler::OnEndDragRight(double x, double y, int WXUNUSED(keys), int attachment)
{
  GetShape()->GetCanvas()->ClearHints();

  GetShape()->GetCanvas()->ReleaseMouse();
  MyCanvas *canvas = (MyCanvas *)GetShape()->GetCanvas();

  // Check if we're on an object
  int new_attachment;
  wxShape *otherShape = canvas->FindFirstSensitiveShape(x, y, &new_attachment, OP_DRAG_RIGHT);

  if (otherShape && !otherShape->IsKindOf(CLASSINFO(wxLineShape)))
  {
    canvas->view->GetDocument()->GetCommandProcessor()->Submit(
      DiagramCommand::AddLine((DiagramDocument *)canvas->view->GetDocument(),
      GetShape(), attachment, otherShape, new_attachment));

    wxLogStatus("New connection created");
  }
}

void MyEvtHandler::OnEndSize(double WXUNUSED(x), double WXUNUSED(y))
{
  wxClientDC dc(GetShape()->GetCanvas());
  GetShape()->GetCanvas()->PrepareDC(dc);

  GetShape()->FormatText(dc, /* (char*) (const char*) */ label);
}
/*
 * New shapes
 */

IMPLEMENT_DYNAMIC_CLASS(wxRoundedRectangleShape, wxRectangleShape)

wxRoundedRectangleShape::wxRoundedRectangleShape(double w, double h):
 wxRectangleShape(w, h)
{
  // 0.3 of the smaller rectangle dimension
  SetCornerRadius((double) -0.3);
}

IMPLEMENT_DYNAMIC_CLASS(wxDiamondShape, wxPolygonShape)

wxDiamondShape::wxDiamondShape(double w, double h):
  wxPolygonShape()
{
  // wxPolygonShape::SetSize relies on the shape having non-zero
  // size initially.
  if (w == 0.0)
    w = 60.0;
  if (h == 0.0)
    h = 60.0;

  wxList *thePoints = new wxList;
  wxRealPoint *point = new wxRealPoint(0.0, (-h/2.0));
  thePoints->Append((wxObject*) point);

  point = new wxRealPoint((w/2.0), 0.0);
  thePoints->Append((wxObject*) point);

  point = new wxRealPoint(0.0, (h/2.0));
  thePoints->Append((wxObject*) point);

  point = new wxRealPoint((-w/2.0), 0.0);
  thePoints->Append((wxObject*) point);

  Create(thePoints);
}

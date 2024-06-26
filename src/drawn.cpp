/////////////////////////////////////////////////////////////////////////////
// Name:        drawn.cpp
// Purpose:     wxDrawnShape
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

#include "wx/ogl/ogl.h"

#include <algorithm>

extern wxChar *oglBuffer;

#define gyTYPE_PEN   40
#define gyTYPE_BRUSH 41
#define gyTYPE_FONT  42

// Helper function for checking whether a vector contains the given element:
namespace
{

inline bool Contains(const std::vector<int>& vec, int value)
{
    return find(vec.begin(), vec.end(), value) != vec.end();
}

} // anonymous namespace

/*
 * Drawn object
 *
 */

IMPLEMENT_DYNAMIC_CLASS(wxDrawnShape, wxRectangleShape)

wxDrawnShape::wxDrawnShape():wxRectangleShape(100.0, 50.0)
{
  m_saveToFile = true;
  m_currentAngle = oglDRAWN_ANGLE_0;
}

wxDrawnShape::~wxDrawnShape()
{
}

void wxDrawnShape::OnDraw(wxDC& dc)
{
  // Pass pen and brush in case we have force outline
  // and fill colours
  if (m_shadowMode != SHADOW_NONE)
  {
    if (m_shadowBrush)
      m_metafiles[m_currentAngle].m_fillBrush = m_shadowBrush;
    m_metafiles[m_currentAngle].m_outlinePen = g_oglTransparentPen;
    m_metafiles[m_currentAngle].Draw(dc, m_xpos + m_shadowOffsetX, m_ypos + m_shadowOffsetY);
  }

  m_metafiles[m_currentAngle].m_outlinePen = m_pen;
  m_metafiles[m_currentAngle].m_fillBrush = m_brush;
  m_metafiles[m_currentAngle].Draw(dc, m_xpos, m_ypos);
}

void wxDrawnShape::SetSize(double w, double h, bool WXUNUSED(recursive))
{
  SetAttachmentSize(w, h);

  double scaleX;
  double scaleY;
  if (GetWidth() == 0.0)
    scaleX = 1.0;
  else scaleX = w/GetWidth();
  if (GetHeight() == 0.0)
    scaleY = 1.0;
  else scaleY = h/GetHeight();

  for (int i = 0; i < 4; i++)
  {
    if (m_metafiles[i].IsValid())
        m_metafiles[i].Scale(scaleX, scaleY);
  }
  m_width = w;
  m_height = h;
  SetDefaultRegionSize();
}

void wxDrawnShape::Scale(double sx, double sy)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        if (m_metafiles[i].IsValid())
        {
            m_metafiles[i].Scale(sx, sy);
            m_metafiles[i].CalculateSize(this);
        }
    }
}

void wxDrawnShape::Translate(double x, double y)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        if (m_metafiles[i].IsValid())
        {
            m_metafiles[i].Translate(x, y);
            m_metafiles[i].CalculateSize(this);
        }
    }
}

// theta is absolute rotation from the zero position
void wxDrawnShape::Rotate(double x, double y, double theta)
{
  m_currentAngle = DetermineMetaFile(theta);

  if (m_currentAngle == 0)
  {
    // Rotate metafile
    if (!m_metafiles[0].GetRotateable())
      return;

    m_metafiles[0].Rotate(x, y, theta);
  }

  double actualTheta = theta-m_rotation;

  // Rotate attachment points
  double sinTheta = (double)sin(actualTheta);
  double cosTheta = (double)cos(actualTheta);
  auto node = m_attachmentPoints.GetFirst();
  while (node)
  {
    wxAttachmentPoint *point = (wxAttachmentPoint *)node->GetData();
    double x1 = point->m_x;
    double y1 = point->m_y;
    point->m_x = x1*cosTheta - y1*sinTheta + x*(1.0 - cosTheta) + y*sinTheta;
    point->m_y = x1*sinTheta + y1*cosTheta + y*(1.0 - cosTheta) + x*sinTheta;
    node = node->GetNext();
  }
  m_rotation = theta;

  m_metafiles[m_currentAngle].CalculateSize(this);
}

// Which metafile do we use now? Based on current rotation and validity
// of metafiles.

int wxDrawnShape::DetermineMetaFile(double rotation)
{
    double tolerance = 0.0001;
    const double pi = M_PI ;
    double angle1 = 0.0;
    double angle2 = pi/2.0;
    double angle3 = pi;
    double angle4 = 3.0*pi/2.0;

    int whichMetafile = 0;

    if (oglRoughlyEqual(rotation, angle1, tolerance))
    {
        whichMetafile = 0;
    }
    else if (oglRoughlyEqual(rotation, angle2, tolerance))
    {
        whichMetafile = 1;
    }
    else if (oglRoughlyEqual(rotation, angle3, tolerance))
    {
        whichMetafile = 2;
    }
    else if (oglRoughlyEqual(rotation, angle4, tolerance))
    {
        whichMetafile = 3;
    }

    if ((whichMetafile > 0) && !m_metafiles[whichMetafile].IsValid())
        whichMetafile = 0;

    return whichMetafile;
}

void wxDrawnShape::OnDrawOutline(wxDC& dc, double x, double y, double w, double h)
{
    if (m_metafiles[m_currentAngle].GetOutlineOp() != -1)
    {
        const auto node = m_metafiles[m_currentAngle].GetOps().Item(m_metafiles[m_currentAngle].GetOutlineOp());
        wxDrawOp* op = (wxDrawOp*) node->GetData();

        if (op->OnDrawOutline(dc, x, y, w, h, m_width, m_height))
            return;
    }

    // Default... just use a rectangle
    wxRectangleShape::OnDrawOutline(dc, x, y, w, h);
}

// Get the perimeter point using the special outline op, if there is one,
// otherwise use default wxRectangleShape scheme
bool wxDrawnShape::GetPerimeterPoint(double x1, double y1,
                                     double x2, double y2,
                                     double *x3, double *y3)
{
    if (m_metafiles[m_currentAngle].GetOutlineOp() != -1)
    {
        const auto node = m_metafiles[m_currentAngle].GetOps().Item(m_metafiles[m_currentAngle].GetOutlineOp());
        wxDrawOp* op = (wxDrawOp*) node->GetData();

        if (op->GetPerimeterPoint(x1, y1, x2, y2, x3, y3, GetX(), GetY(), GetAttachmentMode()))
            return true;
    }

    // Default... just use a rectangle
    return wxRectangleShape::GetPerimeterPoint(x1, y1, x2, y2, x3, y3);
}

// Does the copying for this object
void wxDrawnShape::Copy(wxShape& copy)
{
  wxRectangleShape::Copy(copy);

  wxASSERT( copy.IsKindOf(CLASSINFO(wxDrawnShape)) ) ;

  wxDrawnShape& drawnCopy = (wxDrawnShape&) copy;

  for (int i = 0; i < 4; i++)
  {
    m_metafiles[i].Copy(drawnCopy.m_metafiles[i]);
  }
  drawnCopy.m_saveToFile = m_saveToFile;
  drawnCopy.m_currentAngle = m_currentAngle;
}

bool wxDrawnShape::LoadFromMetaFile(const wxString& filename)
{
  return m_metafiles[0].LoadFromMetaFile(filename, &m_width, &m_height);
}

// Set of functions for drawing into a pseudo metafile.
// They use integers, but doubles are used internally for accuracy
// when scaling.

void wxDrawnShape::DrawLine(const wxPoint& pt1, const wxPoint& pt2)
{
    m_metafiles[m_currentAngle].DrawLine(pt1, pt2);
}

void wxDrawnShape::DrawRectangle(const wxRect& rect)
{
    m_metafiles[m_currentAngle].DrawRectangle(rect);
}

void wxDrawnShape::DrawRoundedRectangle(const wxRect& rect, double radius)
{
    m_metafiles[m_currentAngle].DrawRoundedRectangle(rect, radius);
}

void wxDrawnShape::DrawEllipse(const wxRect& rect)
{
    m_metafiles[m_currentAngle].DrawEllipse(rect);
}

void wxDrawnShape::DrawArc(const wxPoint& centrePt, const wxPoint& startPt, const wxPoint& endPt)
{
    m_metafiles[m_currentAngle].DrawArc(centrePt, startPt, endPt);
}

void wxDrawnShape::DrawEllipticArc(const wxRect& rect, double startAngle, double endAngle)
{
    m_metafiles[m_currentAngle].DrawEllipticArc(rect, startAngle, endAngle);
}

void wxDrawnShape::DrawPoint(const wxPoint& pt)
{
    m_metafiles[m_currentAngle].DrawPoint(pt);
}

void wxDrawnShape::DrawText(const wxString& text, const wxPoint& pt)
{
    m_metafiles[m_currentAngle].DrawText(text, pt);
}

void wxDrawnShape::DrawLines(int n, wxPoint pts[])
{
    m_metafiles[m_currentAngle].DrawLines(n, pts);
}

void wxDrawnShape::DrawPolygon(int n, wxPoint pts[], int flags)
{
    if (flags & oglMETAFLAGS_ATTACHMENTS)
    {
        ClearAttachments();
        int i;
        for (i = 0; i < n; i++)
            m_attachmentPoints.Append(new wxAttachmentPoint(i, pts[i].x, pts[i].y));
    }
    m_metafiles[m_currentAngle].DrawPolygon(n, pts, flags);
}

void wxDrawnShape::DrawSpline(int n, wxPoint pts[])
{
    m_metafiles[m_currentAngle].DrawSpline(n, pts);
}

void wxDrawnShape::SetClippingRect(const wxRect& rect)
{
    m_metafiles[m_currentAngle].SetClippingRect(rect);
}

void wxDrawnShape::DestroyClippingRect()
{
    m_metafiles[m_currentAngle].DestroyClippingRect();
}

void wxDrawnShape::SetDrawnPen(const wxPen* pen, bool isOutline)
{
    m_metafiles[m_currentAngle].SetPen(pen, isOutline);
}

void wxDrawnShape::SetDrawnBrush(const wxBrush* brush, bool isFill)
{
    m_metafiles[m_currentAngle].SetBrush(brush, isFill);
}

void wxDrawnShape::SetDrawnFont(wxFont* font)
{
    m_metafiles[m_currentAngle].SetFont(font);
}

void wxDrawnShape::SetDrawnTextColour(const wxColour& colour)
{
    m_metafiles[m_currentAngle].SetTextColour(colour);
}

void wxDrawnShape::SetDrawnBackgroundColour(const wxColour& colour)
{
    m_metafiles[m_currentAngle].SetBackgroundColour(colour);
}

void wxDrawnShape::SetDrawnBackgroundMode(int mode)
{
    m_metafiles[m_currentAngle].SetBackgroundMode(mode);
}


/*
 * Individual operations
 *
 */

/*
 * Set font, brush, text colour
 *
 */

wxOpSetGDI::wxOpSetGDI(int theOp, wxPseudoMetaFile *theImage, int theGdiIndex, int theMode):
  wxDrawOp(theOp)
{
  m_gdiIndex = theGdiIndex;
  m_image = theImage;
  m_mode = theMode;
}

void wxOpSetGDI::Do(wxDC& dc, double WXUNUSED(xoffset), double WXUNUSED(yoffset))
{
  switch (m_op)
  {
    case DRAWOP_SET_PEN:
    {
      // Check for overriding this operation for outline
      // colour
      if (Contains(m_image->m_outlineColours, m_gdiIndex))
      {
        if (m_image->m_outlinePen)
          dc.SetPen(* m_image->m_outlinePen);
      }
      else
      {
        const auto node = m_image->m_gdiObjects.Item(m_gdiIndex);
        if (node)
        {
          wxPen *pen = (wxPen *)node->GetData();
          if (pen)
            dc.SetPen(* pen);
        }
      }
      break;
    }
    case DRAWOP_SET_BRUSH:
    {
      // Check for overriding this operation for outline or fill
      // colour
      if (Contains(m_image->m_outlineColours, m_gdiIndex))
      {
        // Need to construct a brush to match the outline pen's colour
        if (m_image->m_outlinePen)
        {
          wxBrush *br = wxTheBrushList->FindOrCreateBrush(m_image->m_outlinePen->GetColour());
          if (br)
            dc.SetBrush(* br);
        }
      }
      else if (Contains(m_image->m_fillColours, m_gdiIndex))
      {
        if (m_image->m_fillBrush)
        {
          dc.SetBrush(* m_image->m_fillBrush);
        }
      }
      else
      {
        const auto node = m_image->m_gdiObjects.Item(m_gdiIndex);
        if (node)
        {
          wxBrush *brush = (wxBrush *)node->GetData();
          if (brush)
            dc.SetBrush(* brush);
        }
      }
      break;
    }
    case DRAWOP_SET_FONT:
    {
      const auto node = m_image->m_gdiObjects.Item(m_gdiIndex);
      if (node)
      {
        wxFont *font = (wxFont *)node->GetData();
        if (font)
          dc.SetFont(* font);
      }
      break;
    }
    case DRAWOP_SET_TEXT_COLOUR:
    {
      wxColour col(m_r,m_g,m_b);
      dc.SetTextForeground(col);
      break;
    }
    case DRAWOP_SET_BK_COLOUR:
    {
      wxColour col(m_r,m_g,m_b);
      dc.SetTextBackground(col);
      break;
    }
    case DRAWOP_SET_BK_MODE:
    {
      dc.SetBackgroundMode(m_mode);
      break;
    }
    default:
      break;
  }
}

wxDrawOp *wxOpSetGDI::Copy(wxPseudoMetaFile *newImage)
{
  wxOpSetGDI *newOp = new wxOpSetGDI(m_op, newImage, m_gdiIndex, m_mode);
  newOp->m_r = m_r;
  newOp->m_g = m_g;
  newOp->m_b = m_b;
  return newOp;
}

/*
 * Set/destroy clipping
 *
 */

wxOpSetClipping::wxOpSetClipping(int theOp, double theX1, double theY1,
    double theX2, double theY2):wxDrawOp(theOp)
{
  m_x1 = theX1;
  m_y1 = theY1;
  m_x2 = theX2;
  m_y2 = theY2;
}

wxDrawOp *wxOpSetClipping::Copy(wxPseudoMetaFile *WXUNUSED(newImage))
{
  wxOpSetClipping *newOp = new wxOpSetClipping(m_op, m_x1, m_y1, m_x2, m_y2);
  return newOp;
}

void wxOpSetClipping::Do(wxDC& dc, double xoffset, double yoffset)
{
  switch (m_op)
  {
    case DRAWOP_SET_CLIPPING_RECT:
    {
      dc.SetClippingRegion((long)(m_x1 + xoffset), (long)(m_y1 + yoffset), (long)(m_x2 + xoffset), (long)(m_y2 + yoffset));
      break;
    }
    case DRAWOP_DESTROY_CLIPPING_RECT:
    {
      dc.DestroyClippingRegion();
      break;
    }
    default:
      break;
  }
}

void wxOpSetClipping::Scale(double xScale, double yScale)
{
  m_x1 *= xScale;
  m_y1 *= yScale;
  m_x2 *= xScale;
  m_y2 *= yScale;
}

void wxOpSetClipping::Translate(double x, double y)
{
  m_x1 += x;
  m_y1 += y;
}

/*
 * Draw line, rectangle, rounded rectangle, ellipse, point, arc, text
 *
 */

wxOpDraw::wxOpDraw(int theOp, double theX1, double theY1, double theX2, double theY2,
         double theRadius, const wxString& s) : wxDrawOp(theOp)
{
  m_x1 = theX1;
  m_y1 = theY1;
  m_x2 = theX2;
  m_y2 = theY2;
  m_x3 = 0.0;
  m_y3 = 0.0;
  m_radius = theRadius;
  m_textString = s;
}

wxOpDraw::~wxOpDraw()
{
}

wxDrawOp *wxOpDraw::Copy(wxPseudoMetaFile *WXUNUSED(newImage))
{
  wxOpDraw *newOp = new wxOpDraw(m_op, m_x1, m_y1, m_x2, m_y2, m_radius, m_textString);
  newOp->m_x3 = m_x3;
  newOp->m_y3 = m_y3;
  return newOp;
}

void wxOpDraw::Do(wxDC& dc, double xoffset, double yoffset)
{
  switch (m_op)
  {
    case DRAWOP_DRAW_LINE:
    {
      dc.DrawLine(WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset), WXROUND(m_x2+xoffset), WXROUND(m_y2+yoffset));
      break;
    }
    case DRAWOP_DRAW_RECT:
    {
      dc.DrawRectangle(WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset), WXROUND(m_x2), WXROUND(m_y2));
      break;
    }
    case DRAWOP_DRAW_ROUNDED_RECT:
    {
      dc.DrawRoundedRectangle(WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset), WXROUND(m_x2), WXROUND(m_y2), m_radius);
      break;
    }
    case DRAWOP_DRAW_ELLIPSE:
    {
      dc.DrawEllipse(WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset), WXROUND(m_x2), WXROUND(m_y2));
      break;
    }
    case DRAWOP_DRAW_ARC:
    {
      dc.DrawArc(WXROUND(m_x2+xoffset), WXROUND(m_y2+yoffset),
                 WXROUND(m_x3+xoffset), WXROUND(m_y3+yoffset),
                 WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset));
      break;
    }
    case DRAWOP_DRAW_ELLIPTIC_ARC:
    {
      const double pi = M_PI ;

      // Convert back to degrees
      dc.DrawEllipticArc(
                 WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset),
                 WXROUND(m_x2), WXROUND(m_y2),
                 WXROUND(m_x3*(360.0/(2.0*pi))), WXROUND(m_y3*(360.0/(2.0*pi))));
      break;
    }
    case DRAWOP_DRAW_POINT:
    {
      dc.DrawPoint(WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset));
      break;
    }
    case DRAWOP_DRAW_TEXT:
    {
      dc.DrawText(m_textString, WXROUND(m_x1+xoffset), WXROUND(m_y1+yoffset));
      break;
    }
    default:
      break;
  }
}

void wxOpDraw::Scale(double scaleX, double scaleY)
{
  m_x1 *= scaleX;
  m_y1 *= scaleY;
  m_x2 *= scaleX;
  m_y2 *= scaleY;

  if (m_op != DRAWOP_DRAW_ELLIPTIC_ARC)
  {
    m_x3 *= scaleX;
    m_y3 *= scaleY;
  }

  m_radius *= scaleX;
}

void wxOpDraw::Translate(double x, double y)
{
  m_x1 += x;
  m_y1 += y;

  switch (m_op)
  {
    case DRAWOP_DRAW_LINE:
    {
      m_x2 += x;
      m_y2 += y;
      break;
    }
    case DRAWOP_DRAW_ARC:
    {
      m_x2 += x;
      m_y2 += y;
      m_x3 += x;
      m_y3 += y;
      break;
    }
    case DRAWOP_DRAW_ELLIPTIC_ARC:
    {
      break;
    }
    default:
      break;
  }
}

void wxOpDraw::Rotate(double x, double y, double theta, double sinTheta, double cosTheta)
{
  double newX1 = m_x1*cosTheta - m_y1*sinTheta + x*(1.0 - cosTheta) + y*sinTheta;
  double newY1 = m_x1*sinTheta + m_y1*cosTheta + y*(1.0 - cosTheta) + x*sinTheta;

  switch (m_op)
  {
    case DRAWOP_DRAW_LINE:
    {
      double newX2 = m_x2*cosTheta - m_y2*sinTheta + x*(1.0 - cosTheta) + y*sinTheta;
      double newY2 = m_x2*sinTheta + m_y2*cosTheta + y*(1.0 - cosTheta) + x*sinTheta;

      m_x1 = newX1;
      m_y1 = newY1;
      m_x2 = newX2;
      m_y2 = newY2;
      break;
    }
    case DRAWOP_DRAW_RECT:
    case DRAWOP_DRAW_ROUNDED_RECT:
    case DRAWOP_DRAW_ELLIPTIC_ARC:
    {
      // Assume only 0, 90, 180, 270 degree rotations.
      // oldX1, oldY1 represents the top left corner. Find the
      // bottom right, and rotate that. Then the width/height is the difference
      // between x/y values.
      double oldBottomRightX = m_x1 + m_x2;
      double oldBottomRightY = m_y1 + m_y2;
      double newBottomRightX = oldBottomRightX*cosTheta - oldBottomRightY*sinTheta + x*(1.0 - cosTheta) + y*sinTheta;
      double newBottomRightY = oldBottomRightX*sinTheta + oldBottomRightY*cosTheta + y*(1.0 - cosTheta) + x*sinTheta;

      // Now find the new top-left, bottom-right coordinates.
      double minX = wxMin(newX1, newBottomRightX);
      double minY = wxMin(newY1, newBottomRightY);
      double maxX = wxMax(newX1, newBottomRightX);
      double maxY = wxMax(newY1, newBottomRightY);

      m_x1 = minX;
      m_y1 = minY;
      m_x2 = maxX - minX; // width
      m_y2 = maxY - minY; // height

      if (m_op == DRAWOP_DRAW_ELLIPTIC_ARC)
      {
        // Add rotation to angles
        m_x3 += theta;
        m_y3 += theta;
      }

      break;
    }
    case DRAWOP_DRAW_ARC:
    {
      double newX2 = m_x2*cosTheta - m_y2*sinTheta + x*(1.0 - cosTheta) + y*sinTheta;
      double newY2 = m_x2*sinTheta + m_y2*cosTheta + y*(1.0 - cosTheta) + x*sinTheta;
      double newX3 = m_x3*cosTheta - m_y3*sinTheta + x*(1.0 - cosTheta) + y*sinTheta;
      double newY3 = m_x3*sinTheta + m_y3*cosTheta + y*(1.0 - cosTheta) + x*sinTheta;

      m_x1 = newX1;
      m_y1 = newY1;
      m_x2 = newX2;
      m_y2 = newY2;
      m_x3 = newX3;
      m_y3 = newY3;

      break;
    }
    default:
      break;
  }
}

/*
 * Draw polygon, polyline, spline
 *
 */

wxOpPolyDraw::wxOpPolyDraw(int theOp, int n, wxRealPoint *thePoints):wxDrawOp(theOp)
{
  m_noPoints = n;
  m_points = thePoints;
}

wxOpPolyDraw::~wxOpPolyDraw()
{
  delete[] m_points;
}

wxDrawOp *wxOpPolyDraw::Copy(wxPseudoMetaFile *WXUNUSED(newImage))
{
  wxRealPoint *newPoints = new wxRealPoint[m_noPoints];
  for (int i = 0; i < m_noPoints; i++)
  {
    newPoints[i].x = m_points[i].x;
    newPoints[i].y = m_points[i].y;
  }
  wxOpPolyDraw *newOp = new wxOpPolyDraw(m_op, m_noPoints, newPoints);
  return newOp;
}

void wxOpPolyDraw::Do(wxDC& dc, double xoffset, double yoffset)
{
  switch (m_op)
  {
    case DRAWOP_DRAW_POLYLINE:
    {
        wxPoint *actualPoints = new wxPoint[m_noPoints];
        int i;
        for (i = 0; i < m_noPoints; i++)
        {
            actualPoints[i].x = WXROUND(m_points[i].x);
            actualPoints[i].y = WXROUND(m_points[i].y);
        }

        dc.DrawLines(m_noPoints, actualPoints, WXROUND(xoffset), WXROUND(yoffset));

        delete[] actualPoints;
        break;
    }
    case DRAWOP_DRAW_POLYGON:
    {
        wxPoint *actualPoints = new wxPoint[m_noPoints];
        int i;
        for (i = 0; i < m_noPoints; i++)
        {
            actualPoints[i].x = WXROUND(m_points[i].x);
            actualPoints[i].y = WXROUND(m_points[i].y);
        }

        dc.DrawPolygon(m_noPoints, actualPoints, WXROUND(xoffset), WXROUND(yoffset));

        delete[] actualPoints;
        break;
    }
    case DRAWOP_DRAW_SPLINE:
    {
        wxPoint *actualPoints = new wxPoint[m_noPoints];
        int i;
        for (i = 0; i < m_noPoints; i++)
        {
            actualPoints[i].x = WXROUND(m_points[i].x);
            actualPoints[i].y = WXROUND(m_points[i].y);
        }

        dc.DrawSpline(m_noPoints, actualPoints); // no offsets in DrawSpline // , xoffset, yoffset);

        delete[] actualPoints;
        break;
    }
    default:
      break;
  }
}

void wxOpPolyDraw::Scale(double scaleX, double scaleY)
{
  for (int i = 0; i < m_noPoints; i++)
  {
    m_points[i].x *= scaleX;
    m_points[i].y *= scaleY;
  }
}

void wxOpPolyDraw::Translate(double x, double y)
{
  for (int i = 0; i < m_noPoints; i++)
  {
    m_points[i].x += x;
    m_points[i].y += y;
  }
}

void wxOpPolyDraw::Rotate(double x, double y, double WXUNUSED(theta), double sinTheta, double cosTheta)
{
  for (int i = 0; i < m_noPoints; i++)
  {
    double x1 = m_points[i].x;
    double y1 = m_points[i].y;
    m_points[i].x = x1*cosTheta - y1*sinTheta + x*(1.0 - cosTheta) + y*sinTheta;
    m_points[i].y = x1*sinTheta + y1*cosTheta + y*(1.0 - cosTheta) + x*sinTheta;
  }
}

// Draw an outline using the current operation.
bool wxOpPolyDraw::OnDrawOutline(wxDC& dc, double x, double y, double w, double h, double oldW, double oldH)
{
    dc.SetBrush(* wxTRANSPARENT_BRUSH);

    // Multiply all points by proportion of new size to old size
    double x_proportion = (double)(fabs(w/oldW));
    double y_proportion = (double)(fabs(h/oldH));

    int n = m_noPoints;
    wxPoint *intPoints = new wxPoint[n];
    int i;
    for (i = 0; i < n; i++)
    {
        intPoints[i].x = WXROUND (x_proportion * m_points[i].x);
        intPoints[i].y = WXROUND (y_proportion * m_points[i].y);
    }
    dc.DrawPolygon(n, intPoints, (long) x, (long) y);
    delete[] intPoints;
    return true;
}

// Assume (x1, y1) is centre of box (most generally, line end at box)
bool wxOpPolyDraw::GetPerimeterPoint(double x1, double y1,
                                     double x2, double y2,
                                     double *x3, double *y3,
                                     double xOffset, double yOffset,
                                     int attachmentMode)
{
  int n = m_noPoints;

  // First check for situation where the line is vertical,
  // and we would want to connect to a point on that vertical --
  // oglFindEndForPolyline can't cope with this (the arrow
  // gets drawn to the wrong place).
  if ((attachmentMode == ATTACHMENT_MODE_NONE) && (x1 == x2))
  {
    // Look for the point we'd be connecting to. This is
    // a heuristic...
    int i;
    for (i = 0; i < n; i++)
    {
      wxRealPoint *point = & (m_points[i]);
      if (point->x == 0.0)
      {
        if ((y2 > y1) && (point->y > 0.0))
        {
          *x3 = point->x + xOffset;
          *y3 = point->y + yOffset;
          return true;
        }
        else if ((y2 < y1) && (point->y < 0.0))
        {
          *x3 = point->x + xOffset;
          *y3 = point->y + yOffset;
          return true;
        }
      }
    }
  }

  double *xpoints = new double[n];
  double *ypoints = new double[n];

  for (int i = 0; i < n; i++)
  {
    wxRealPoint *point = & (m_points[i]);
    xpoints[i] = point->x + xOffset;
    ypoints[i] = point->y + yOffset;
  }

  oglFindEndForPolyline(n, xpoints, ypoints,
                        x1, y1, x2, y2, x3, y3);

  delete[] xpoints;
  delete[] ypoints;

  return true;
}


/*
 * wxPseudo meta-file
 *
 */

IMPLEMENT_DYNAMIC_CLASS(wxPseudoMetaFile, wxObject)

wxPseudoMetaFile::wxPseudoMetaFile()
{
  m_currentRotation = 0;
  m_rotateable = true;
  m_width = 0.0;
  m_height = 0.0;
  m_outlinePen = nullptr;
  m_fillBrush = nullptr;
  m_outlineOp = -1;
}

wxPseudoMetaFile::wxPseudoMetaFile(wxPseudoMetaFile& mf):wxObject()
{
  mf.Copy(*this);
}

wxPseudoMetaFile::~wxPseudoMetaFile()
{
  Clear();
}

void wxPseudoMetaFile::Clear()
{
  auto node = m_ops.GetFirst();
  while (node)
  {
    wxDrawOp *op = (wxDrawOp *)node->GetData();
    delete op;
    node = node->GetNext();
  }
  m_ops.Clear();
  m_gdiObjects.Clear();
  m_outlineColours.clear();
  m_fillColours.clear();
  m_outlineOp = -1;
}

void wxPseudoMetaFile::Draw(wxDC& dc, double xoffset, double yoffset)
{
  auto node = m_ops.GetFirst();
  while (node)
  {
    wxDrawOp *op = (wxDrawOp *)node->GetData();
    op->Do(dc, xoffset, yoffset);
    node = node->GetNext();
  }
}

void wxPseudoMetaFile::Scale(double sx, double sy)
{
  auto node = m_ops.GetFirst();
  while (node)
  {
    wxDrawOp *op = (wxDrawOp *)node->GetData();
    op->Scale(sx, sy);
    node = node->GetNext();
  }
  m_width *= sx;
  m_height *= sy;
}

void wxPseudoMetaFile::Translate(double x, double y)
{
  auto node = m_ops.GetFirst();
  while (node)
  {
    wxDrawOp *op = (wxDrawOp *)node->GetData();
    op->Translate(x, y);
    node = node->GetNext();
  }
}

void wxPseudoMetaFile::Rotate(double x, double y, double theta)
{
  double theta1 = theta-m_currentRotation;
  if (theta1 == 0.0) return;
  double cosTheta = (double)cos(theta1);
  double sinTheta = (double)sin(theta1);

  auto node = m_ops.GetFirst();
  while (node)
  {
    wxDrawOp *op = (wxDrawOp *)node->GetData();
    op->Rotate(x, y, theta, sinTheta, cosTheta);
    node = node->GetNext();
  }
  m_currentRotation = theta;
}

// Does the copying for this object
void wxPseudoMetaFile::Copy(wxPseudoMetaFile& copy)
{
  copy.Clear();

  copy.m_currentRotation = m_currentRotation;
  copy.m_width = m_width;
  copy.m_height = m_height;
  copy.m_rotateable = m_rotateable;
  copy.m_fillBrush = m_fillBrush;
  copy.m_outlinePen = m_outlinePen;
  copy.m_outlineOp = m_outlineOp;

  // Copy the GDI objects
  auto node = m_gdiObjects.GetFirst();
  while (node)
  {
    wxObject *obj = (wxObject *)node->GetData();
    copy.m_gdiObjects.Append(obj);
    node = node->GetNext();
  }

  // Copy the operations
  node = m_ops.GetFirst();
  while (node)
  {
    wxDrawOp *op = (wxDrawOp *)node->GetData();
    copy.m_ops.Append(op->Copy(&copy));
    node = node->GetNext();
  }

  // Copy the outline/fill operations
  copy.m_outlineColours = m_outlineColours;
  copy.m_fillColours = m_fillColours;
}

/*
 * Pass size of existing image; scale height to
 * fit width and return new width and height.
 *
 */

bool wxPseudoMetaFile::LoadFromMetaFile(const wxString& filename, double *rwidth, double *rheight)
{
  if (!wxFileExists(filename))
    return false;

  wxXMetaFile *metaFile = new wxXMetaFile;

  if (!metaFile->ReadFile(filename))
  {
    delete metaFile;
    return false;
  }

  double lastX = 0.0;
  double lastY = 0.0;

  // Convert from metafile records to wxDrawnShape records
  auto node = metaFile->metaRecords.GetFirst();
  while (node)
  {
    wxMetaRecord *record = (wxMetaRecord *)node->GetData();
    switch (record->metaFunction)
    {
      case META_SETBKCOLOR:
      {
        wxOpSetGDI *op = new wxOpSetGDI(DRAWOP_SET_BK_COLOUR, this, 0);
        op->m_r = (unsigned char)record->param1;
        op->m_g = (unsigned char)record->param2;
        op->m_b = (unsigned char)record->param3;
        m_ops.Append(op);
        break;
      }
      case META_SETBKMODE:
      {
        wxOpSetGDI *op = new wxOpSetGDI(DRAWOP_SET_BK_MODE, this, 0, (int)record->param1);
        m_ops.Append(op);
        break;
      }
      case META_SETMAPMODE:
      {
        break;
      }
//      case META_SETROP2:
//      case META_SETRELABS:
//      case META_SETPOLYFILLMODE:
//      case META_SETSTRETCHBLTMODE:
//      case META_SETTEXTCHAREXTRA:
      case META_SETTEXTCOLOR:
      {
        wxOpSetGDI *op = new wxOpSetGDI(DRAWOP_SET_TEXT_COLOUR, this, 0);
        op->m_r = (unsigned char)record->param1;
        op->m_g = (unsigned char)record->param2;
        op->m_b = (unsigned char)record->param3;
        m_ops.Append(op);
        break;
      }
//      case META_SETTEXTJUSTIFICATION:
//      case META_SETWINDOWORG:
//      case META_SETWINDOWEXT:
//      case META_SETVIEWPORTORG:
//      case META_SETVIEWPORTEXT:
//      case META_OFFSETWINDOWORG:
//      case META_SCALEWINDOWEXT:
//      case META_OFFSETVIEWPORTORG:
//      case META_SCALEVIEWPORTEXT:
      case META_LINETO:
      {
        wxOpDraw *op = new wxOpDraw(DRAWOP_DRAW_LINE, (double)lastX, (double)lastY,
                               (double)record->param1, (double)record->param2);
        m_ops.Append(op);
        break;
      }
      case META_MOVETO:
      {
        lastX = (double)record->param1;
        lastY = (double)record->param2;
        break;
      }
      case META_EXCLUDECLIPRECT:
      {
/*
        wxMetaRecord *rec = new wxMetaRecord(META_EXCLUDECLIPRECT);
        rec->param4 = getshort(handle); // m_y2
        rec->param3 = getshort(handle); // x2
        rec->param2 = getshort(handle); // y1
        rec->param1 = getshort(handle); // x1
*/
        break;
      }
      case META_INTERSECTCLIPRECT:
      {
/*
        rec->param4 = getshort(handle); // m_y2
        rec->param3 = getshort(handle); // x2
        rec->param2 = getshort(handle); // y1
        rec->param1 = getshort(handle); // x1
*/
        break;
      }
//      case META_ARC: // DO!!!
      case META_ELLIPSE:
      {
        wxOpDraw *op = new wxOpDraw(DRAWOP_DRAW_ELLIPSE,
                               (double)record->param1, (double)record->param2,
                               (double)(record->param3 - record->param1),
                               (double)(record->param4 - record->param2));
        m_ops.Append(op);
        break;
      }
//      case META_FLOODFILL:
//      case META_PIE: // DO!!!
      case META_RECTANGLE:
      {
        wxOpDraw *op = new wxOpDraw(DRAWOP_DRAW_RECT,
                               (double)record->param1, (double)record->param2,
                               (double)(record->param3 - record->param1),
                               (double)(record->param4 - record->param2));
        m_ops.Append(op);
        break;
      }
      case META_ROUNDRECT:
      {
        wxOpDraw *op = new wxOpDraw(DRAWOP_DRAW_ROUNDED_RECT,
              (double)record->param1, (double)record->param2,
              (double)(record->param3 - record->param1),
              (double)(record->param4 - record->param2), (double)record->param5);
        m_ops.Append(op);
        break;
      }
//      case META_PATBLT:
//      case META_SAVEDC:
      case META_SETPIXEL:
      {
        wxOpDraw *op = new wxOpDraw(DRAWOP_DRAW_POINT,
              (double)record->param1, (double)record->param2,
              0.0, 0.0);

//        SHOULD SET THE COLOUR - SET PEN?
//        rec->param3 = getint(handle);   // COLORREF
        m_ops.Append(op);
        break;
      }
//      case META_OFFSETCLIPRGN:
      case META_TEXTOUT:
      {
        wxOpDraw *op = new wxOpDraw(DRAWOP_DRAW_TEXT,
              (double)record->param1, (double)record->param2,
              0.0, 0.0, 0.0, record->stringParam);
        m_ops.Append(op);
        break;
      }
//      case META_BITBLT:
//      case META_STRETCHBLT:
      case META_POLYGON:
      {
        int n = (int)record->param1;
        wxRealPoint *newPoints = new wxRealPoint[n];
        for (int i = 0; i < n; i++)
        {
          newPoints[i].x = record->points[i].x;
          newPoints[i].y = record->points[i].y;
        }

        wxOpPolyDraw *op = new wxOpPolyDraw(DRAWOP_DRAW_POLYGON, n, newPoints);
        m_ops.Append(op);
        break;
      }
      case META_POLYLINE:
      {
        int n = (int)record->param1;
        wxRealPoint *newPoints = new wxRealPoint[n];
        for (int i = 0; i < n; i++)
        {
          newPoints[i].x = record->points[i].x;
          newPoints[i].y = record->points[i].y;
        }

        wxOpPolyDraw *op = new wxOpPolyDraw(DRAWOP_DRAW_POLYLINE, n, newPoints);
        m_ops.Append(op);
        break;
      }
//      case META_ESCAPE:
//      case META_RESTOREDC:
//      case META_FILLREGION:
//      case META_FRAMEREGION:
//      case META_INVERTREGION:
//      case META_PAINTREGION:
//      case META_SELECTCLIPREGION: // DO THIS!
      case META_SELECTOBJECT:
      {
        // The pen, brush etc. has already been created when the metafile
        // was read in, so we don't create it - we set it.
        const auto recNode = metaFile->gdiObjects.Item((int)record->param2);
        if (recNode)
        {
          wxMetaRecord *gdiRec = (wxMetaRecord *)recNode->GetData();
          if (gdiRec && (gdiRec->param1 != 0))
          {
            wxObject *obj = (wxObject *)gdiRec->param1;
            if (obj->IsKindOf(CLASSINFO(wxPen)))
            {
              wxOpSetGDI *op = new wxOpSetGDI(DRAWOP_SET_PEN, this, (int)record->param2);
              m_ops.Append(op);
            }
            else if (obj->IsKindOf(CLASSINFO(wxBrush)))
            {
              wxOpSetGDI *op = new wxOpSetGDI(DRAWOP_SET_BRUSH, this, (int)record->param2);
              m_ops.Append(op);
            }
            else if (obj->IsKindOf(CLASSINFO(wxFont)))
            {
              wxOpSetGDI *op = new wxOpSetGDI(DRAWOP_SET_FONT, this, (int)record->param2);
              m_ops.Append(op);
            }
          }
        }
        break;
      }
//      case META_SETTEXTALIGN:
//      case META_DRAWTEXT:
//      case META_CHORD:
//      case META_SETMAPPERFLAGS:
//      case META_EXTTEXTOUT:
//      case META_SETDIBTODEV:
//      case META_SELECTPALETTE:
//      case META_REALIZEPALETTE:
//      case META_ANIMATEPALETTE:
//      case META_SETPALENTRIES:
//      case META_POLYPOLYGON:
//      case META_RESIZEPALETTE:
//      case META_DIBBITBLT:
//      case META_DIBSTRETCHBLT:
      case META_DIBCREATEPATTERNBRUSH:
      {
        // Place holder
        m_gdiObjects.Append(nullptr);
        break;
      }
//      case META_STRETCHDIB:
//      case META_EXTFLOODFILL:
//      case META_RESETDC:
//      case META_STARTDOC:
//      case META_STARTPAGE:
//      case META_ENDPAGE:
//      case META_ABORTDOC:
//      case META_ENDDOC:
//      case META_DELETEOBJECT: // DO!!
      case META_CREATEPALETTE:
      {
        // Place holder
        m_gdiObjects.Append(nullptr);
        break;
      }
      case META_CREATEBRUSH:
      {
        // Place holder
        m_gdiObjects.Append(nullptr);
        break;
      }
      case META_CREATEPATTERNBRUSH:
      {
        // Place holder
        m_gdiObjects.Append(nullptr);
        break;
      }
      case META_CREATEPENINDIRECT:
      {
        // The pen is created when the metafile is read in.
        // We keep track of all the GDI objects needed for this
        // image so when reading the wxDrawnShape from file,
        // we can read in all the GDI objects, then refer
        // to them by an index starting from zero thereafter.
        m_gdiObjects.Append((wxObject *)record->param1);
        break;
      }
      case META_CREATEFONTINDIRECT:
      {
        m_gdiObjects.Append((wxObject *)record->param1);
        break;
      }
      case META_CREATEBRUSHINDIRECT:
      {
        // Don't have to do anything here: the pen is created
        // when the metafile is read in.
        m_gdiObjects.Append((wxObject *)record->param1);
        break;
      }
      case META_CREATEBITMAPINDIRECT:
      {
        // Place holder
        m_gdiObjects.Append(nullptr);
        break;
      }
      case META_CREATEBITMAP:
      {
        // Place holder
        m_gdiObjects.Append(nullptr);
        break;
      }
      case META_CREATEREGION:
      {
        // Place holder
        m_gdiObjects.Append(nullptr);
        break;
      }
      default:
      {
        break;
      }
    }
    node = node->GetNext();
  }
  double actualWidth = (double)fabs(metaFile->right - metaFile->left);
  double actualHeight = (double)fabs(metaFile->bottom - metaFile->top);

  double initialScaleX = 1.0;
  double initialScaleY = 1.0;

  double xoffset, yoffset;

  // Translate so origin is at centre of rectangle
  if (metaFile->bottom > metaFile->top)
    yoffset = - (double)((metaFile->bottom - metaFile->top)/2.0);
  else
    yoffset = - (double)((metaFile->top - metaFile->bottom)/2.0);

  if (metaFile->right > metaFile->left)
    xoffset = - (double)((metaFile->right - metaFile->left)/2.0);
  else
    xoffset = - (double)((metaFile->left - metaFile->right)/2.0);

  Translate(xoffset, yoffset);

  // Scale to a reasonable size (take the width of this wxDrawnShape
  // as a guide)
  if (actualWidth != 0.0)
  {
    initialScaleX = (double)((*rwidth) / actualWidth);
    initialScaleY = initialScaleX;
    (*rheight) = initialScaleY*actualHeight;
  }
  Scale(initialScaleX, initialScaleY);

  m_width = (actualWidth*initialScaleX);
  m_height = *rheight;

  delete metaFile;
  return true;
}

// Scale to fit size
void wxPseudoMetaFile::ScaleTo(double w, double h)
{
  double scaleX = (double)(w/m_width);
  double scaleY = (double)(h/m_height);

  // Do the scaling
  Scale(scaleX, scaleY);
}

void wxPseudoMetaFile::GetBounds(double *boundMinX, double *boundMinY, double *boundMaxX, double *boundMaxY)
{
  double maxX = (double) -99999.9;
  double maxY = (double) -99999.9;
  double minX = (double) 99999.9;
  double minY = (double) 99999.9;

  auto node = m_ops.GetFirst();
  while (node)
  {
    wxDrawOp *op = (wxDrawOp *)node->GetData();
    switch (op->GetOp())
    {
      case DRAWOP_DRAW_LINE:
      case DRAWOP_DRAW_RECT:
      case DRAWOP_DRAW_ROUNDED_RECT:
      case DRAWOP_DRAW_ELLIPSE:
      case DRAWOP_DRAW_ELLIPTIC_ARC:
      case DRAWOP_DRAW_POINT:
      case DRAWOP_DRAW_TEXT:
      {
        wxOpDraw *opDraw = (wxOpDraw *)op;
        if (opDraw->m_x1 < minX) minX = opDraw->m_x1;
        if (opDraw->m_x1 > maxX) maxX = opDraw->m_x1;
        if (opDraw->m_y1 < minY) minY = opDraw->m_y1;
        if (opDraw->m_y1 > maxY) maxY = opDraw->m_y1;
        if (op->GetOp() == DRAWOP_DRAW_LINE)
        {
          if (opDraw->m_x2 < minX) minX = opDraw->m_x2;
          if (opDraw->m_x2 > maxX) maxX = opDraw->m_x2;
          if (opDraw->m_y2 < minY) minY = opDraw->m_y2;
          if (opDraw->m_y2 > maxY) maxY = opDraw->m_y2;
        }
        else if (op->GetOp() == DRAWOP_DRAW_RECT ||
                 op->GetOp() == DRAWOP_DRAW_ROUNDED_RECT ||
                 op->GetOp() == DRAWOP_DRAW_ELLIPSE ||
                 op->GetOp() == DRAWOP_DRAW_ELLIPTIC_ARC)
        {
          if ((opDraw->m_x1 + opDraw->m_x2) < minX) minX = (opDraw->m_x1 + opDraw->m_x2);
          if ((opDraw->m_x1 + opDraw->m_x2) > maxX) maxX = (opDraw->m_x1 + opDraw->m_x2);
          if ((opDraw->m_y1 + opDraw->m_y2) < minY) minY = (opDraw->m_y1 + opDraw->m_y2);
          if ((opDraw->m_y1 + opDraw->m_y2) > maxY) maxY = (opDraw->m_y1 + opDraw->m_y2);
        }
        break;
      }
      case DRAWOP_DRAW_ARC:
      {
        // TODO: don't yet know how to calculate the bounding box
        // for an arc. So pretend it's a line; to get a correct
        // bounding box, draw a blank rectangle first, of the correct
        // size.
        wxOpDraw *opDraw = (wxOpDraw *)op;
        if (opDraw->m_x1 < minX) minX = opDraw->m_x1;
        if (opDraw->m_x1 > maxX) maxX = opDraw->m_x1;
        if (opDraw->m_y1 < minY) minY = opDraw->m_y1;
        if (opDraw->m_y1 > maxY) maxY = opDraw->m_y1;
        if (opDraw->m_x2 < minX) minX = opDraw->m_x2;
        if (opDraw->m_x2 > maxX) maxX = opDraw->m_x2;
        if (opDraw->m_y2 < minY) minY = opDraw->m_y2;
        if (opDraw->m_y2 > maxY) maxY = opDraw->m_y2;
        break;
      }
      case DRAWOP_DRAW_POLYLINE:
      case DRAWOP_DRAW_POLYGON:
      case DRAWOP_DRAW_SPLINE:
      {
        wxOpPolyDraw *poly = (wxOpPolyDraw *)op;
        for (int i = 0; i < poly->m_noPoints; i++)
        {
          if (poly->m_points[i].x < minX) minX = poly->m_points[i].x;
          if (poly->m_points[i].x > maxX) maxX = poly->m_points[i].x;
          if (poly->m_points[i].y < minY) minY = poly->m_points[i].y;
          if (poly->m_points[i].y > maxY) maxY = poly->m_points[i].y;
        }
        break;
      }
      default:
        break;
    }
    node = node->GetNext();
  }

  *boundMinX = minX;
  *boundMinY = minY;
  *boundMaxX = maxX;
  *boundMaxY = maxY;
/*
  *w = (double)fabs(maxX - minX);
  *h = (double)fabs(maxY - minY);
*/
}

// Calculate size from current operations
void wxPseudoMetaFile::CalculateSize(wxDrawnShape* shape)
{
  double boundMinX, boundMinY, boundMaxX, boundMaxY;

  GetBounds(& boundMinX, & boundMinY, & boundMaxX, & boundMaxY);

  SetSize(boundMaxX - boundMinX, boundMaxY - boundMinY);

  if (shape)
  {
    shape->SetWidth(m_width);
    shape->SetHeight(m_height);
  }
}

// Set of functions for drawing into a pseudo metafile.
// They use integers, but doubles are used internally for accuracy
// when scaling.

void wxPseudoMetaFile::DrawLine(const wxPoint& pt1, const wxPoint& pt2)
{
    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_LINE,
          (double) pt1.x, (double) pt1.y, (double) pt2.x, (double) pt2.y);

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawRectangle(const wxRect& rect)
{
    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_RECT,
          (double) rect.x, (double) rect.y, (double) rect.width, (double) rect.height);

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawRoundedRectangle(const wxRect& rect, double radius)
{
    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_ROUNDED_RECT,
          (double) rect.x, (double) rect.y, (double) rect.width, (double) rect.height);

    theOp->m_radius = radius;

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawEllipse(const wxRect& rect)
{
    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_ELLIPSE,
          (double) rect.x, (double) rect.y, (double) rect.width, (double) rect.height);

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawArc(const wxPoint& centrePt, const wxPoint& startPt, const wxPoint& endPt)
{
    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_ARC,
          (double) centrePt.x, (double) centrePt.y, (double) startPt.x, (double) startPt.y);

    theOp->m_x3 = (double) endPt.x;
    theOp->m_y3 = (double) endPt.y;

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawEllipticArc(const wxRect& rect, double startAngle, double endAngle)
{
    const double pi = M_PI ;

    double startAngleRadians = startAngle* (pi*2.0/360.0);
    double endAngleRadians = endAngle* (pi*2.0/360.0);

    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_ELLIPTIC_ARC,
          (double) rect.x, (double) rect.y, (double) rect.width, (double) rect.height);

    theOp->m_x3 = startAngleRadians;
    theOp->m_y3 = endAngleRadians;

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawPoint(const wxPoint& pt)
{
    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_POINT,
          (double) pt.x, (double) pt.y, 0.0, 0.0);

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawText(const wxString& text, const wxPoint& pt)
{
    wxOpDraw *theOp = new wxOpDraw(DRAWOP_DRAW_TEXT,
          (double) pt.x, (double) pt.y, 0.0, 0.0);

    theOp->m_textString = text;

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawLines(int n, wxPoint pts[])
{
    wxRealPoint* realPoints = new wxRealPoint[n];
    int i;
    for (i = 0; i < n; i++)
    {
        realPoints[i].x = pts[i].x;
        realPoints[i].y = pts[i].y;
    }
    wxOpPolyDraw* theOp = new wxOpPolyDraw(DRAWOP_DRAW_POLYLINE, n, realPoints);
    m_ops.Append(theOp);
}

void wxPseudoMetaFile::DrawPolygon(int n, wxPoint pts[], int flags)
{
    wxRealPoint* realPoints = new wxRealPoint[n];
    int i;
    for (i = 0; i < n; i++)
    {
        realPoints[i].x = pts[i].x;
        realPoints[i].y = pts[i].y;
    }
    wxOpPolyDraw* theOp = new wxOpPolyDraw(DRAWOP_DRAW_POLYGON, n, realPoints);
    m_ops.Append(theOp);

    if (flags & oglMETAFLAGS_OUTLINE)
        m_outlineOp = (m_ops.GetCount() - 1);
}

void wxPseudoMetaFile::DrawSpline(int n, wxPoint pts[])
{
    wxRealPoint* realPoints = new wxRealPoint[n];
    int i;
    for (i = 0; i < n; i++)
    {
        realPoints[i].x = pts[i].x;
        realPoints[i].y = pts[i].y;
    }
    wxOpPolyDraw* theOp = new wxOpPolyDraw(DRAWOP_DRAW_SPLINE, n, realPoints);
    m_ops.Append(theOp);
}

void wxPseudoMetaFile::SetClippingRect(const wxRect& rect)
{
    /* wxOpSetClipping* theOp = */ new wxOpSetClipping(DRAWOP_SET_CLIPPING_RECT,
        (double) rect.x, (double) rect.y, (double) rect.width, (double) rect.height);
}

void wxPseudoMetaFile::DestroyClippingRect()
{
    wxOpSetClipping* theOp = new wxOpSetClipping(DRAWOP_DESTROY_CLIPPING_RECT,
        0.0, 0.0, 0.0, 0.0);

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::SetPen(const wxPen* pen, bool isOutline)
{
    m_gdiObjects.Append(wx_const_cast(wxPen*, pen));
    int n = m_gdiObjects.GetCount();

    wxOpSetGDI* theOp = new wxOpSetGDI(DRAWOP_SET_PEN, this, n - 1);

    m_ops.Append(theOp);

    if (isOutline)
    {
        m_outlineColours.push_back(n - 1);
    }
}

void wxPseudoMetaFile::SetBrush(const wxBrush* brush, bool isFill)
{
    m_gdiObjects.Append(wx_const_cast(wxBrush*, brush));
    int n = m_gdiObjects.GetCount();

    wxOpSetGDI* theOp = new wxOpSetGDI(DRAWOP_SET_BRUSH, this, n - 1);

    m_ops.Append(theOp);

    if (isFill)
    {
        m_fillColours.push_back(n - 1);
    }
}

void wxPseudoMetaFile::SetFont(wxFont* font)
{
    m_gdiObjects.Append(font);
    int n = m_gdiObjects.GetCount();

    wxOpSetGDI* theOp = new wxOpSetGDI(DRAWOP_SET_FONT, this, n - 1);

    m_ops.Append(theOp);
}

void wxPseudoMetaFile::SetTextColour(const wxColour& colour)
{
   wxOpSetGDI* theOp = new wxOpSetGDI(DRAWOP_SET_TEXT_COLOUR, this, 0);
   theOp->m_r = colour.Red();
   theOp->m_g = colour.Green();
   theOp->m_b = colour.Blue();

   m_ops.Append(theOp);
}

void wxPseudoMetaFile::SetBackgroundColour(const wxColour& colour)
{
   wxOpSetGDI* theOp = new wxOpSetGDI(DRAWOP_SET_BK_COLOUR, this, 0);
   theOp->m_r = colour.Red();
   theOp->m_g = colour.Green();
   theOp->m_b = colour.Blue();

   m_ops.Append(theOp);
}

void wxPseudoMetaFile::SetBackgroundMode(int mode)
{
   wxOpSetGDI* theOp = new wxOpSetGDI(DRAWOP_SET_BK_MODE, this, 0, mode);

   m_ops.Append(theOp);
}


/////////////////////////////////////////////////////////////////////////////
// Name:        basicp.h
// Purpose:     Private OGL classes and definitions
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_BASICP_H_
#define _OGL_BASICP_H_

#define CONTROL_POINT_SIZE       6

class WXDLLIMPEXP_OGL wxShapeTextLine: public wxObject
{
 DECLARE_DYNAMIC_CLASS(wxShapeTextLine)
public:
   wxShapeTextLine(double the_x = 0.0, double the_y = 0.0, const wxString& the_line = wxEmptyString);
   ~wxShapeTextLine();

   inline double GetX() const { return m_x; }
   inline double GetY() const { return m_y; }

   inline void SetX(double x) { m_x = x; }
   inline void SetY(double y) { m_y = y; }

   inline void SetText(const wxString& text) { m_line = text; }
   inline wxString GetText() const { return m_line; }

protected:
   wxString     m_line;
   double        m_x;
   double        m_y;
};

class WXDLLIMPEXP_OGL wxShape;
class WXDLLIMPEXP_OGL wxControlPoint: public wxRectangleShape
{
 DECLARE_DYNAMIC_CLASS(wxControlPoint)

 friend class WXDLLIMPEXP_OGL wxShapeEvtHandler;
 friend class WXDLLIMPEXP_OGL wxShape;

 public:
  wxControlPoint(wxShapeCanvas *the_canvas = nullptr, wxShape *object = nullptr, double size = 0.0, double the_xoffset = 0.0,
    double the_yoffset = 0.0, int the_type = 0);
  ~wxControlPoint();

  void OnDraw(wxDC& dc) override;
  void OnErase(wxReadOnlyDC& dc) override;
  void OnDrawContents(wxDC& dc) override;
  void OnDragLeft(bool draw, double x, double y, int keys=0, int attachment = 0) override;
  void OnBeginDragLeft(double x, double y, int keys=0, int attachment = 0) override;
  void OnEndDragLeft(double x, double y, int keys=0, int attachment = 0) override;

  bool GetAttachmentPosition(int attachment, double *x, double *y,
                             int nth = 0, int no_arcs = 1, wxLineShape *line = nullptr) override;
  int GetNumberOfAttachments() const override;

  inline void SetEraseObject(bool er) { m_eraseObject = er; }

public:
  int             m_type;
  double          m_xoffset;
  double          m_yoffset;
  wxShape*        m_shape;
  const wxCursor* m_oldCursor;
  bool            m_eraseObject; // If true, erases object before dragging handle.

/*
 * Store original top-left, bottom-right coordinates
 * in case we're doing non-vertical resizing.
 */
  static double sm_controlPointDragStartX;
  static double sm_controlPointDragStartY;
  static double sm_controlPointDragStartWidth;
  static double sm_controlPointDragStartHeight;
  static double sm_controlPointDragEndWidth;
  static double sm_controlPointDragEndHeight;
  static double sm_controlPointDragPosX;
  static double sm_controlPointDragPosY;
};

class WXDLLIMPEXP_OGL wxPolygonShape;
class WXDLLIMPEXP_OGL wxPolygonControlPoint: public wxControlPoint
{
 DECLARE_DYNAMIC_CLASS(wxPolygonControlPoint)
  friend class WXDLLIMPEXP_OGL wxPolygonShape;
 public:
  wxPolygonControlPoint(wxShapeCanvas *the_canvas = nullptr, wxShape *object = nullptr, double size = 0.0, wxRealPoint *vertex = nullptr,
    double the_xoffset = 0.0, double the_yoffset = 0.0);
  ~wxPolygonControlPoint();

  void OnDragLeft(bool draw, double x, double y, int keys=0, int attachment = 0) override;
  void OnBeginDragLeft(double x, double y, int keys=0, int attachment = 0) override;
  void OnEndDragLeft(double x, double y, int keys=0, int attachment = 0) override;

  // Calculate what new size would be, at end of resize
  virtual void CalculateNewSize(double x, double y);

  // Get new size
  inline wxRealPoint GetNewSize() const { return m_newSize; };

public:
  wxRealPoint*      m_polygonVertex;
  wxRealPoint       m_originalSize;
  double             m_originalDistance;
  wxRealPoint       m_newSize;
};

/*
 * Object regions.
 * Every shape has one or more text regions with various
 * properties. Not all of a region's properties will be used
 * by a shape.
 *
 */

class WXDLLIMPEXP_OGL wxShapeRegion: public wxObject
{
 DECLARE_DYNAMIC_CLASS(wxShapeRegion)

 public:
  // Constructor
  wxShapeRegion();
  // Copy constructor
  wxShapeRegion(wxShapeRegion& region);
  // Destructor
  ~wxShapeRegion();

  // Accessors
  inline void SetText(const wxString& s)
    { m_regionText = s; }
  void SetFont(wxFont *f);
  void SetMinSize(double w, double h);
  void SetSize(double w, double h);
  void SetPosition(double x, double y);
  void SetProportions(double x, double y);
  void SetFormatMode(int mode);
  inline void SetName(const wxString& s) { m_regionName = s; };
  void SetColour(const wxString& col); // Text colour

  inline wxString GetText() const { return m_regionText; }
  inline wxFont *GetFont() const { return m_font; }
  inline void GetMinSize(double *x, double *y) const { *x = m_minWidth; *y = m_minHeight; }
  inline void GetProportion(double *x, double *y) const { *x = m_regionProportionX; *y = m_regionProportionY; }
  inline void GetSize(double *x, double *y) const { *x = m_width; *y = m_height; }
  inline void GetPosition(double *xp, double *yp) const { *xp = m_x; *yp = m_y; }
  inline int GetFormatMode() const { return m_formatMode; }
  inline wxString GetName() const { return m_regionName; }
  inline wxString GetColour() const { return m_textColour; }
  wxColour GetActualColourObject();
  inline wxList& GetFormattedText() { return m_formattedText; }
  inline wxString GetPenColour() const { return m_penColour; }
  inline wxPenStyle GetPenStyle() const { return m_penStyle; }
  inline void SetPenStyle(wxPenStyle style) { m_penStyle = style; m_actualPenObject = nullptr; }
  void SetPenColour(const wxString& col);
  wxPen *GetActualPen();
  inline double GetWidth() const { return m_width; }
  inline double GetHeight() const { return m_height; }

  void ClearText();

public:
  wxString              m_regionText;
  wxList                m_formattedText;   // List of wxShapeTextLines
  wxFont*               m_font;
  double                 m_minHeight;        // If zero, hide region.
  double                 m_minWidth;        // If zero, hide region.
  double                 m_width;
  double                 m_height;
  double                 m_x;
  double                 m_y;

  double                 m_regionProportionX; // Proportion of total object size;
                                             // -1.0 indicates equal proportion
  double                 m_regionProportionY; // Proportion of total object size;
                                             // -1.0 indicates equal proportion

  int                   m_formatMode;        // FORMAT_CENTRE_HORIZ | FORMAT_CENTRE_VERT | FORMAT_NONE
  wxString              m_regionName;
  wxString              m_textColour;
  wxColour              m_actualColourObject; // For speed purposes

  // New members for specifying divided rectangle division colour/style 30/6/94
  wxString              m_penColour;
  wxPenStyle            m_penStyle;
  wxPen*                m_actualPenObject;

};

/*
 * User-defined attachment point
 */

class WXDLLIMPEXP_OGL wxAttachmentPoint: public wxObject
{
 DECLARE_DYNAMIC_CLASS(wxAttachmentPoint)

public:
  inline wxAttachmentPoint()
  {
    m_id = 0; m_x = 0.0; m_y = 0.0;
  }
  inline wxAttachmentPoint(int id, double x, double y)
  {
    m_id = id; m_x = x; m_y = y;
  }

public:
  int            m_id;           // Identifier
  double         m_x;            // x offset from centre of object
  double         m_y;            // y offset from centre of object
};

#endif
  // _OGL_BASICP_H_

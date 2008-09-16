                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2008 Andrew Trevorrow and Tomas Rokicki.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Web site:  http://sourceforge.net/projects/golly
 Authors:   rokicki@gmail.com  andrew@trevorrow.com

                        / ***/

#include "wx/wxprec.h"     // for compilers that support precompilation
#ifndef WX_PRECOMP
   #include "wx/wx.h"      // for all others include the necessary headers
#endif

#include "wx/colordlg.h"   // for wxColourDialog

#include "bigint.h"
#include "lifealgo.h"
#include "qlifealgo.h"
#include "hlifealgo.h"
#include "slifealgo.h"
#include "jvnalgo.h"
#include "wwalgo.h"
#include "generationsalgo.h"

#include "wxgolly.h"       // for wxGetApp, mainptr, viewptr
#include "wxmain.h"        // for mainptr->...
#include "wxview.h"        // for viewptr->...
#include "wxprefs.h"       // for swapcolors, etc
#include "wxscript.h"      // for inscript
#include "wxutils.h"       // for Warning, Fatal, FillRect
#include "wxlayer.h"       // for currlayer, InvertIconColors
#include "wxalgos.h"

// -----------------------------------------------------------------------------

// exported data:

wxMenu* algomenu;                   // menu of algorithm names
algo_type initalgo = QLIFE_ALGO;    // initial layer's algorithm
AlgoData* algoinfo[MAX_ALGOS];      // static info for each algorithm

// -----------------------------------------------------------------------------

// These default cell colors were generated by continuously finding the
// color furthest in rgb space from the closest of the already selected
// colors, black, and white.
static unsigned char default_colors[] = {
255,127,0,0,255,127,127,0,255,148,148,148,128,255,0,255,0,128,
0,128,255,1,159,0,159,0,1,255,254,96,0,1,159,96,255,254,
254,96,255,126,125,21,21,126,125,125,21,126,255,116,116,116,255,116,
116,116,255,228,227,0,28,255,27,255,27,28,0,228,227,227,0,228,
27,28,255,59,59,59,234,195,176,175,196,255,171,194,68,194,68,171,
68,171,194,72,184,71,184,71,72,71,72,184,169,255,188,252,179,63,
63,252,179,179,63,252,80,9,0,0,80,9,9,0,80,255,175,250,
199,134,213,115,100,95,188,163,0,0,188,163,163,0,188,203,73,0,
0,203,73,73,0,203,94,189,0,189,0,94,0,94,189,187,243,119,
55,125,32,125,32,55,32,55,125,255,102,185,102,185,255,120,209,168,
208,166,119,135,96,192,182,255,41,83,153,130,247,88,55,89,247,55,
88,55,247,87,75,0,0,87,75,75,0,87,200,135,59,51,213,127,
255,255,162,255,37,182,37,182,255,228,57,117,142,163,210,57,117,228,
193,255,246,188,107,123,123,194,107,145,59,5,5,145,59,59,5,145,
119,39,198,40,197,23,197,23,40,23,40,197,178,199,158,255,201,121,
134,223,223,39,253,84,149,203,15,203,15,149,15,149,203,152,144,90,
143,75,139,71,97,132,224,65,219,65,219,224,255,255,40,218,223,69,
74,241,0,241,0,74,0,74,241,122,171,51,220,211,227,61,127,87,
90,124,176,36,39,13,165,142,255,255,38,255,38,255,255,83,50,107,
224,142,165,255,181,9,9,255,181,181,9,255,140,238,70,255,74,5,
74,5,255,138,84,51,31,172,101,177,115,17,221,0,0,0,221,0,
0,0,221,220,255,200,0,41,50,255,150,205,178,45,116,113,255,189,
47,0,44,40,119,171,205,107,255,177,115,172,133,73,236,109,0,168,
168,46,207,188,181,203,212,188,35,90,97,52,39,209,184,41,164,152,
227,46,70,46,70,227,211,156,255,98,146,222,136,56,95,102,54,152,
86,142,0,142,0,86,0,86,142,86,223,96,246,135,46,4,208,120,
212,233,158,177,92,214,104,147,88,149,240,147,227,93,148,72,255,133,
209,27,194,147,255,255,44,93,0,160,36,158,182,233,0,96,94,217,
218,103,88,163,154,38,118,114,139,94,0,43,113,164,174,168,188,114,
0,23,119,42,86,93,255,226,202,80,191,155,255,158,136,0,247,62,
234,146,88,0,183,229,110,212,36,0,143,161,105,191,210,133,164,0,
41,30,89,164,0,132,30,89,42,178,222,217,121,22,11,221,107,22,
69,151,255,45,158,3,158,3,45,3,45,158,86,42,29,9,122,22,
213,209,110,53,221,57,159,101,91,93,140,45,247,213,37,185,34,0,
0,185,34,34,0,185,236,0,172,210,180,78,231,107,221,162,49,43,
43,162,49,49,43,162,36,248,213,114,0,214,213,36,248,149,34,243,
185,158,167,144,122,224,34,245,149,255,31,98,31,98,255,152,200,193,
255,80,95,128,123,63,102,62,72,255,62,148,151,226,108,159,99,255,
226,255,126,98,223,136,80,95,255,225,153,15,73,41,211,212,71,41,
83,217,187,180,235,79,0,166,127,251,135,243,229,41,0,41,0,229,
82,255,216,141,174,249,249,215,255,167,31,79,31,79,167,213,102,185,
255,215,83,4,2,40,224,171,220,41,0,4,6,50,90,221,15,113,
15,113,221,33,0,115,108,23,90,182,215,36
};

// -----------------------------------------------------------------------------

static wxBitmap** CreateIconBitmaps(char** xpmdata)
{
   if (xpmdata == NULL) return NULL;
   
   wxImage image(xpmdata);
   image.SetMaskColour(0, 0, 0);    // make black transparent
   wxBitmap allicons(image);

   int wd = allicons.GetWidth();
   int numicons = allicons.GetHeight() / wd;
   
   wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
   if (iconptr) {
      for (int i = 0; i < 256; i++) iconptr[i] = NULL;
      
      if (numicons > 255) numicons = 255;    // play safe
      for (int i = 0; i < numicons; i++) {
         wxRect rect(0, i*wd, wd, wd);
         // add 1 because iconptr[0] must be NULL (ie. dead state)
         iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
      }
   }
   return iconptr;
}

// -----------------------------------------------------------------------------

static wxBitmap** ScaleIconBitmaps(wxBitmap** srcicons, int size)
{
   if (srcicons == NULL) return NULL;
   
   wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
   if (iconptr) {
      for (int i = 0; i < 256; i++) {
         if (srcicons[i] == NULL) {
            iconptr[i] = NULL;
         } else {
            wxImage image = srcicons[i]->ConvertToImage();
            iconptr[i] = new wxBitmap(image.Scale(size, size));
         }
      }
   }
   return iconptr;
}

// -----------------------------------------------------------------------------

AlgoData::AlgoData() {
   algomem = algobase = 0;
   statusbrush = NULL;
   icons7x7 = icons15x15 = NULL;
}

// -----------------------------------------------------------------------------

AlgoData& AlgoData::tick() {
   AlgoData* r = new AlgoData();
   algoinfo[r->id] = r;
   return *r;
}

// -----------------------------------------------------------------------------

void AlgoData::createIconBitmaps(int size, char** xpmdata) {
   wxBitmap** bm = CreateIconBitmaps(xpmdata);
   if (size == 7)
      icons7x7 = bm;
   else if (size == 15)
      icons15x15 = bm;
}

// -----------------------------------------------------------------------------

void InitAlgorithms()
{
   // qlife must be 1st and hlife must be 2nd
   qlifealgo::doInitializeAlgoInfo(AlgoData::tick());
   hlifealgo::doInitializeAlgoInfo(AlgoData::tick());
   // nicer if the rest are in alphabetical order
   generationsalgo::doInitializeAlgoInfo(AlgoData::tick());
   jvnalgo::doInitializeAlgoInfo(AlgoData::tick());
   slifealgo::doInitializeAlgoInfo(AlgoData::tick());
   wwalgo::doInitializeAlgoInfo(AlgoData::tick());

   // algomenu is used when algo button is pressed and for Set Algo submenu
   algomenu = new wxMenu();

   // init algoinfo array
   for (int i = 0; i < NumAlgos(); i++) {
      AlgoData* ad = algoinfo[i];
      if (ad->algoName == 0 || ad->creator == 0)
         Fatal(_("Algorithm did not set name and/or creator"));
      
      wxString name = wxString(ad->algoName, wxConvLocal);
      algomenu->AppendCheckItem(ID_ALGO0 + i, name);
      
      // does algo use hashing?
      ad->canhash = ad->algobase == 8;    //!!! safer method needed???
      
      // set status bar background by cycling thru a few pale colors
      switch (i % 9) {
         case 0: ad->statusrgb.Set(255, 255, 206); break;  // pale yellow
         case 1: ad->statusrgb.Set(226, 250, 248); break;  // pale blue
         case 2: ad->statusrgb.Set(255, 233, 233); break;  // pale pink
         case 3: ad->statusrgb.Set(225, 255, 225); break;  // pale green
         case 4: ad->statusrgb.Set(243, 225, 255); break;  // pale purple
         case 5: ad->statusrgb.Set(255, 220, 180); break;  // pale orange
         case 6: ad->statusrgb.Set(200, 255, 255); break;  // pale aqua
         case 7: ad->statusrgb.Set(200, 200, 200); break;  // pale gray
         case 8: ad->statusrgb.Set(255, 255, 255); break;  // white
      }
      ad->statusbrush = new wxBrush(ad->statusrgb);

      // create scaled bitmaps if only one size is supplied
      if (!ad->icons15x15) {
         // scale up 7x7 bitmaps (looks ugly)
         ad->icons15x15 = ScaleIconBitmaps(ad->icons7x7, 15);
      }
      if (!ad->icons7x7) {
         // scale down 15x15 bitmaps (not too bad)
         ad->icons7x7 = ScaleIconBitmaps(ad->icons15x15, 7);
      }
      
      // set initial color scheme to default color scheme
      ad->gradient = ad->defgradient;
      ad->fromrgb.Set(ad->defr1, ad->defg1, ad->defb1);
      ad->torgb.Set(ad->defr2, ad->defg2, ad->defb2);
      if (ad->defr[0] == ad->defr[1] &&
          ad->defg[0] == ad->defg[1] &&
          ad->defb[0] == ad->defb[1]) {
         // colors are nonsensical, probably unset, so use above defaults
         unsigned char* rgbptr = default_colors;
         for (int c = 0; c < ad->maxstates; c++) {
            ad->defr[c] = *rgbptr++;
            ad->defg[c] = *rgbptr++;
            ad->defb[c] = *rgbptr++;
         }
      }
      for (int c = 0; c < ad->maxstates; c++) {
         ad->algor[c] = ad->defr[c];
         ad->algog[c] = ad->defg[c];
         ad->algob[c] = ad->defb[c];
      }
   }
}

// -----------------------------------------------------------------------------

lifealgo* CreateNewUniverse(algo_type algotype, bool allowcheck)
{
   lifealgo* newalgo = NULL;
   newalgo = algoinfo[algotype]->creator();

   if (newalgo == NULL) Fatal(_("Failed to create new universe!"));

   if (algoinfo[algotype]->algomem >= 0)
      newalgo->setMaxMemory(algoinfo[algotype]->algomem);

   if (allowcheck) newalgo->setpoll(wxGetApp().Poller());

   return newalgo;
}

// -----------------------------------------------------------------------------

const char* GetAlgoName(algo_type algotype)
{
   return algoinfo[algotype]->algoName;
}

// -----------------------------------------------------------------------------

int NumAlgos()
{
   return staticAlgoInfo::getNumAlgos();
}

// -----------------------------------------------------------------------------

// global data used in CellPanel and ColorDialog methods:

static int algoindex;         // currently selected algorithm
static bool seeicons;         // show icons?

const int CELLSIZE = 16;      // wd and ht of each cell in CellPanel
const int NUMCOLS = 32;       // number of columns in CellPanel
const int NUMROWS = 8;        // number of rows in CellPanel

// -----------------------------------------------------------------------------

// define a window for displaying cell colors/icons:

class CellPanel : public wxPanel
{
public:
   CellPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos,
             const wxSize& size) : wxPanel(parent, id, pos, size) { }

   wxStaticText* statebox;    // for showing state of cell under cursor
   
private:
   void GetGradientColor(int state, unsigned char* r,
                                    unsigned char* g,
                                    unsigned char* b);

   void OnEraseBackground(wxEraseEvent& event);
   void OnPaint(wxPaintEvent& event);
   void OnMouseDown(wxMouseEvent& event);
   void OnMouseMotion(wxMouseEvent& event);
   void OnMouseExit(wxMouseEvent& event);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CellPanel, wxPanel)
   EVT_ERASE_BACKGROUND (CellPanel::OnEraseBackground)
   EVT_PAINT            (CellPanel::OnPaint)
   EVT_LEFT_DOWN        (CellPanel::OnMouseDown)
   EVT_LEFT_DCLICK      (CellPanel::OnMouseDown)
   EVT_MOTION           (CellPanel::OnMouseMotion)
   EVT_ENTER_WINDOW     (CellPanel::OnMouseMotion)
   EVT_LEAVE_WINDOW     (CellPanel::OnMouseExit)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void CellPanel::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
   // do nothing
}

// -----------------------------------------------------------------------------

void CellPanel::GetGradientColor(int state, unsigned char* r,
                                            unsigned char* g,
                                            unsigned char* b)
{
   // calculate gradient color for given state (> 0 and < maxstates)
   AlgoData* ad = algoinfo[algoindex];
   if (state == 1) {
      *r = ad->fromrgb.Red();
      *g = ad->fromrgb.Green();
      *b = ad->fromrgb.Blue();
   } else if (state == ad->maxstates - 1) {
      *r = ad->torgb.Red();
      *g = ad->torgb.Green();
      *b = ad->torgb.Blue();
   } else {
      unsigned char r1 = ad->fromrgb.Red();
      unsigned char g1 = ad->fromrgb.Green();
      unsigned char b1 = ad->fromrgb.Blue();
      unsigned char r2 = ad->torgb.Red();
      unsigned char g2 = ad->torgb.Green();
      unsigned char b2 = ad->torgb.Blue();
      int N = ad->maxstates - 1;
      double rfrac = (double)(r2 - r1) / (double)N;
      double gfrac = (double)(g2 - g1) / (double)N;
      double bfrac = (double)(b2 - b1) / (double)N;
      *r = (int)(r1 + (state-1) * rfrac + 0.5);
      *g = (int)(g1 + (state-1) * gfrac + 0.5);
      *b = (int)(b1 + (state-1) * bfrac + 0.5);
   }
}

// -----------------------------------------------------------------------------

void CellPanel::OnPaint(wxPaintEvent& event)
{
   wxPaintDC dc(this);
   
   dc.SetPen(*wxBLACK_PEN);

   // draw cell boxes
   wxRect r = wxRect(0, 0, CELLSIZE+1, CELLSIZE+1);
   int col = 0;
   for (int state = 0; state < 256; state++) {
      if (state == 0) {
         if (seeicons) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
         } else {
            dc.SetBrush(*deadbrush);
         }
         dc.DrawRectangle(r);
         dc.SetBrush(wxNullBrush);

      } else if (state < algoinfo[algoindex]->maxstates) {
         if (seeicons) {
            wxBitmap** iconmaps = algoinfo[algoindex]->icons15x15;
            if (iconmaps && iconmaps[state]) {
               dc.SetBrush(*deadbrush);
               dc.DrawRectangle(r);
               dc.SetBrush(wxNullBrush);
               dc.DrawBitmap(*iconmaps[state], r.x + 1, r.y + 1, true);
            } else {
               dc.SetBrush(*wxTRANSPARENT_BRUSH);
               dc.DrawRectangle(r);
               dc.SetBrush(wxNullBrush);
            }
         } else if (algoinfo[algoindex]->gradient) {
            unsigned char red, green, blue;
            GetGradientColor(state, &red, &green, &blue);
            wxColor color(red, green, blue);
            dc.SetBrush(wxBrush(color));
            dc.DrawRectangle(r);
            dc.SetBrush(wxNullBrush);
         } else {
            wxColor color(algoinfo[algoindex]->algor[state],
                          algoinfo[algoindex]->algog[state],
                          algoinfo[algoindex]->algob[state]);
            dc.SetBrush(wxBrush(color));
            dc.DrawRectangle(r);
            dc.SetBrush(wxNullBrush);
         }

      } else {
         // state >= maxstates
         dc.SetBrush(*wxTRANSPARENT_BRUSH);
         dc.DrawRectangle(r);
         dc.SetBrush(wxNullBrush);
      }
      
      col++;
      if (col < NUMCOLS) {
         r.x += CELLSIZE;
      } else {
         r.x = 0;
         r.y += CELLSIZE;
         col = 0;
      }
   }

   dc.SetPen(wxNullPen);
}

// -----------------------------------------------------------------------------

void CellPanel::OnMouseDown(wxMouseEvent& event)
{
   int col = event.GetX() / CELLSIZE;
   int row = event.GetY() / CELLSIZE;
   int state = row * NUMCOLS + col;
   if (state >= 0 || state < algoinfo[algoindex]->maxstates) {
      if (seeicons || algoinfo[algoindex]->gradient) {
         wxBell();
      } else {
         // let user change color of this cell state
         //!!!
      }
   } 

   event.Skip();
}

// -----------------------------------------------------------------------------

void CellPanel::OnMouseMotion(wxMouseEvent& event)
{
   int col = event.GetX() / CELLSIZE;
   int row = event.GetY() / CELLSIZE;
   int state = row * NUMCOLS + col;
   if (state < 0 || state > 255) {
      statebox->SetLabel(_(" "));
   } else {
      statebox->SetLabel(wxString::Format(_("%d"),state));
   }
}

// -----------------------------------------------------------------------------

void CellPanel::OnMouseExit(wxMouseEvent& WXUNUSED(event))
{
   statebox->SetLabel(_(" "));
}

// -----------------------------------------------------------------------------

// define a modal dialog for changing colors

class ColorDialog : public wxDialog
{
public:
   ColorDialog(wxWindow* parent);
   virtual bool TransferDataFromWindow();    // called when user hits OK

   enum {
      // control ids
      ALGO_CHOICE = wxID_HIGHEST + 1,
      GRADIENT_CHECK,
      ICON_CHECK,
      CELL_PANEL,
      STATE_BOX,
      STATUS_BUTT,
      FROM_BUTT,
      TO_BUTT,
      DEAD_BUTT,
      PASTE_BUTT,
      SELECT_BUTT,
      DEFAULT_BUTT
   };

   void CreateControls();     // initialize all the controls

   void AddColorButton(wxWindow* parent, wxBoxSizer* hbox,
                       int id, wxColor* rgb, const wxString& text);
   void ChangeButtonColor(int id, wxColor* rgb);
   void UpdateButtonColor(int id, wxColor* rgb);

   wxChoice* algochoice;      // menu of algorithms

   CellPanel* cellpanel;      // for displaying cell colors/icons
   wxCheckBox* gradcheck;     // use gradient?
   wxCheckBox* iconcheck;     // show icons?
   wxButton* defbutt;         // button to restore default color scheme
   
private:
   // event handlers
   void OnChooseAlgo(wxCommandEvent& event);
   void OnCheckBoxClicked(wxCommandEvent& event);
   void OnDefaultButton(wxCommandEvent& event);
   void OnColorButton(wxCommandEvent& event);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ColorDialog, wxDialog)
   EVT_CHOICE     (ALGO_CHOICE,     ColorDialog::OnChooseAlgo)
   EVT_CHECKBOX   (wxID_ANY,        ColorDialog::OnCheckBoxClicked)
   EVT_BUTTON     (DEFAULT_BUTT,    ColorDialog::OnDefaultButton)
   EVT_BUTTON     (wxID_ANY,        ColorDialog::OnColorButton)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

// these consts are used to get nicely spaced controls on each platform:

const int HGAP = 12;          // space left and right of vertically stacked boxes
const int BIGVGAP = 12;       // vertical gap between groups of controls
#ifdef __WXMAC__
   #define SBTOPGAP (2)       // vertical gap before first item in wxStaticBoxSizer
   #define SBBOTGAP (2)       // vertical gap after last item in wxStaticBoxSizer
#elif defined(__WXMSW__)
   #define SBTOPGAP (7)
   #define SBBOTGAP (7)
#else // assume Unix
   #define SBTOPGAP (12)
   #define SBBOTGAP (7)
#endif

#if defined(__WXMAC__) && wxCHECK_VERSION(2,8,0)
   // fix wxALIGN_CENTER_VERTICAL bug in wxMac 2.8.0+;
   // only happens when a wxStaticText/wxButton box is next to a wxChoice box
   #define FIX_ALIGN_BUG wxBOTTOM,4
#else
   #define FIX_ALIGN_BUG wxALL,0
#endif

const int BITMAP_WD = 60;     // width of bitmap in color buttons
const int BITMAP_HT = 20;     // height of bitmap in color buttons

// -----------------------------------------------------------------------------

ColorDialog::ColorDialog(wxWindow* parent)
{
   Create(parent, wxID_ANY, _("Set Colors"), wxDefaultPosition, wxDefaultSize);
   CreateControls();
   Centre();
}

// -----------------------------------------------------------------------------

void ColorDialog::CreateControls()
{
   // create a choice menu to select algo
   wxArrayString algoarray;
   for (int i = 0; i < NumAlgos(); i++) {
      algoarray.Add( wxString(GetAlgoName(i),wxConvLocal) );
   }
   algochoice = new wxChoice(this, ALGO_CHOICE, wxDefaultPosition, wxDefaultSize, algoarray);
   algoindex = currlayer->algtype;
   algochoice->SetSelection(algoindex);
   
   // create bitmap buttons
   wxBoxSizer* statusbox = new wxBoxSizer(wxHORIZONTAL);
   wxBoxSizer* frombox = new wxBoxSizer(wxHORIZONTAL);
   wxBoxSizer* tobox = new wxBoxSizer(wxHORIZONTAL);
   wxBoxSizer* deadbox = new wxBoxSizer(wxHORIZONTAL);
   wxBoxSizer* selectbox = new wxBoxSizer(wxHORIZONTAL);
   AddColorButton(this, statusbox, STATUS_BUTT,
                        &algoinfo[algoindex]->statusrgb, _("Status bar background"));
   AddColorButton(this, frombox, FROM_BUTT,
                        &algoinfo[algoindex]->fromrgb, _("to"));
   AddColorButton(this, tobox, TO_BUTT,
                        &algoinfo[algoindex]->torgb, _(" "));
   AddColorButton(this, deadbox, DEAD_BUTT,
                        deadrgb, _("Dead cells (state 0)"));
   deadbox->AddStretchSpacer();
   AddColorButton(this, deadbox, PASTE_BUTT,
                        pastergb, _("Paste rectangle"));
   deadbox->AddStretchSpacer();
   AddColorButton(this, selectbox, SELECT_BUTT,
                        selectrgb, _("Selection (will be 50% transparent)"));

   wxBoxSizer* algobox = new wxBoxSizer(wxHORIZONTAL);
   wxBoxSizer* algolabel = new wxBoxSizer(wxHORIZONTAL);
   algolabel->Add(new wxStaticText(this, wxID_STATIC, _("Algorithm:")), 0, FIX_ALIGN_BUG);
   algobox->Add(algolabel, 0, wxALIGN_CENTER_VERTICAL, 0);
   algobox->Add(algochoice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
   algobox->AddStretchSpacer();
   algobox->Add(statusbox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
   algobox->AddStretchSpacer();
   
   wxStaticBox* sbox1 = new wxStaticBox(this, wxID_ANY, _("Color scheme for this algorithm:"));
   wxBoxSizer* ssizer1 = new wxStaticBoxSizer(sbox1, wxVERTICAL);

   gradcheck = new wxCheckBox(this, GRADIENT_CHECK, _("Use gradient from"));
   gradcheck->SetValue(algoinfo[algoindex]->gradient);
   
   wxBoxSizer* gradbox = new wxBoxSizer(wxHORIZONTAL);
   gradbox->Add(gradcheck, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 0);
   gradbox->AddSpacer(5);
   gradbox->Add(frombox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 0);
   gradbox->AddSpacer(5);
   gradbox->Add(tobox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 0);

   // create child window for displaying cell colors/icons
   cellpanel = new CellPanel(this, CELL_PANEL, wxPoint(0,0),
                             wxSize(NUMCOLS*CELLSIZE+1,NUMROWS*CELLSIZE+1));

   iconcheck = new wxCheckBox(this, ICON_CHECK, _("Show icons"));
   seeicons = false;
   iconcheck->SetValue(seeicons);

   defbutt = new wxButton(this, DEFAULT_BUTT, _("Default"));

   wxStaticText* statebox = new wxStaticText(this, STATE_BOX, _("999"));
   cellpanel->statebox = statebox;
   statebox->SetLabel(_(" "));

   wxBoxSizer* defbox = new wxBoxSizer(wxHORIZONTAL);
   defbox->Add(defbutt, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 0);
   defbox->AddStretchSpacer();
   defbox->Add(new wxStaticText(this, wxID_STATIC, _("Cell state: ")),
               0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 0);
   defbox->Add(statebox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 0);
   defbox->AddStretchSpacer();
   defbox->Add(iconcheck, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 0);
   
   ssizer1->AddSpacer(SBTOPGAP);
   ssizer1->Add(gradbox, 0, wxLEFT | wxRIGHT, 0);
   ssizer1->AddSpacer(10);
   ssizer1->Add(cellpanel, 0, wxLEFT | wxRIGHT, 0);
   ssizer1->AddSpacer(10);
   ssizer1->Add(defbox, 1, wxGROW | wxLEFT | wxRIGHT, 0);
   ssizer1->AddSpacer(SBBOTGAP);
   
   wxStaticBox* sbox2 = new wxStaticBox(this, wxID_ANY, _("Global colors for all algorithms:"));
   wxBoxSizer* ssizer2 = new wxStaticBoxSizer(sbox2, wxVERTICAL);
   
   ssizer2->AddSpacer(SBTOPGAP);
   ssizer2->Add(deadbox, 1, wxGROW | wxLEFT | wxRIGHT, 0);
   ssizer2->AddSpacer(10);
   ssizer2->Add(selectbox, 0, wxLEFT | wxRIGHT, 0);
   ssizer2->AddSpacer(SBBOTGAP);

   wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);

   wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(algobox, 1, wxGROW | wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(ssizer1, 0, wxGROW | wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(ssizer2, 0, wxGROW | wxLEFT | wxRIGHT, HGAP);
   // topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(stdbutts, 1, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 10);
   SetSizer(topSizer);
   topSizer->SetSizeHints(this);    // calls Fit
}

// -----------------------------------------------------------------------------

void ColorDialog::OnChooseAlgo(wxCommandEvent& event)
{
   int i = event.GetSelection();
   if (i >= 0 && i < NumAlgos() && i != algoindex) {
      algoindex = i;

      gradcheck->SetValue(algoinfo[algoindex]->gradient);

      // update colors in some bitmap buttons
      UpdateButtonColor(STATUS_BUTT, &algoinfo[algoindex]->statusrgb);
      UpdateButtonColor(FROM_BUTT, &algoinfo[algoindex]->fromrgb);
      UpdateButtonColor(TO_BUTT, &algoinfo[algoindex]->torgb);
      
      cellpanel->Refresh(false);
   }
}

// -----------------------------------------------------------------------------

void ColorDialog::OnCheckBoxClicked(wxCommandEvent& event)
{
   if ( event.GetId() == GRADIENT_CHECK ) {
      if (gradcheck->GetValue()) {
         algoinfo[algoindex]->gradient = true;
         cellpanel->Refresh(false);
      } else {
         algoinfo[algoindex]->gradient = false;
         cellpanel->Refresh(false);
      }
   }

   if ( event.GetId() == ICON_CHECK ) {
      if (iconcheck->GetValue()) {
         seeicons = true;
         cellpanel->Refresh(false);
      } else {
         seeicons = false;
         cellpanel->Refresh(false);
      }
   }
}

// -----------------------------------------------------------------------------

void ColorDialog::OnDefaultButton(wxCommandEvent& WXUNUSED(event))
{
   // restore default color scheme for selected algo
   //!!!
}

// -----------------------------------------------------------------------------

void ColorDialog::AddColorButton(wxWindow* parent, wxBoxSizer* hbox,
                                 int id, wxColor* rgb, const wxString& text)
{
   wxBitmap bitmap(BITMAP_WD, BITMAP_HT);
   wxMemoryDC dc;
   dc.SelectObject(bitmap);
   wxRect rect(0, 0, BITMAP_WD, BITMAP_HT);
   wxBrush brush(*rgb);
   FillRect(dc, rect, brush);
   dc.SelectObject(wxNullBitmap);
   
   wxBitmapButton* bb = new wxBitmapButton(parent, id, bitmap, wxPoint(0,0));
   wxBoxSizer* textbox = new wxBoxSizer(wxHORIZONTAL);
   if (bb && textbox) {
      hbox->Add(bb, 0, wxALIGN_CENTER_VERTICAL, 0);
      textbox->Add(new wxStaticText(parent, wxID_STATIC, text), 0, wxALL, 0);
      hbox->Add(textbox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
   }
}

// -----------------------------------------------------------------------------

void ColorDialog::UpdateButtonColor(int id, wxColor* rgb)
{
   wxBitmapButton* bb = (wxBitmapButton*) FindWindow(id);
   if (bb) {
      wxBitmap bitmap(BITMAP_WD, BITMAP_HT);
      wxMemoryDC dc;
      dc.SelectObject(bitmap);
      wxRect rect(0, 0, BITMAP_WD, BITMAP_HT);
      wxBrush brush(*rgb);
      FillRect(dc, rect, brush);
      dc.SelectObject(wxNullBitmap);
      bb->SetBitmapLabel(bitmap);
      bb->Refresh();
      bb->Update();
   }
}

// -----------------------------------------------------------------------------

void ColorDialog::ChangeButtonColor(int id, wxColor* rgb)
{
   wxColourData data;
   data.SetChooseFull(true);    // for Windows
   data.SetColour(*rgb);
   
   wxColourDialog dialog(this, &data);
   if ( dialog.ShowModal() == wxID_OK ) {
      wxColourData retData = dialog.GetColourData();
      wxColour c = retData.GetColour();
      
      if (*rgb != c) {
         // change given color
         rgb->Set(c.Red(), c.Green(), c.Blue());
         
         // also change color of bitmap in corresponding button
         UpdateButtonColor(id, rgb);
      }
   }
}

// -----------------------------------------------------------------------------

void ColorDialog::OnColorButton(wxCommandEvent& event)
{
   int id = event.GetId();

   if ( id == STATUS_BUTT ) {
      ChangeButtonColor(id, &algoinfo[algoindex]->statusrgb);
   
   } else if ( id == FROM_BUTT ) {
      ChangeButtonColor(id, &algoinfo[algoindex]->fromrgb);
   
   } else if ( id == TO_BUTT ) {
      ChangeButtonColor(id, &algoinfo[algoindex]->torgb);
   
   } else if ( id == DEAD_BUTT ) {
      ChangeButtonColor(id, deadrgb);

   } else if ( id == PASTE_BUTT ) {
      ChangeButtonColor(id, pastergb);

   } else if ( id == SELECT_BUTT ) {
      ChangeButtonColor(id, selectrgb);
   
   } else {
      // process other buttons like Cancel and OK
      event.Skip();
   }
}

// -----------------------------------------------------------------------------

bool ColorDialog::TransferDataFromWindow()
{
   // no need to do any validation!!!???
   return true;
}

// -----------------------------------------------------------------------------

// class for saving and restoring AlgoData color info in SetColors()
class SaveData {
public:
   SaveData(AlgoData* ad) {
      statusrgb = ad->statusrgb;
      gradient = ad->gradient;
      fromrgb = ad->fromrgb;
      torgb = ad->torgb;
      for (int i = 0; i < ad->maxstates; i++) {
         algor[i] = ad->algor[i];
         algog[i] = ad->algog[i];
         algob[i] = ad->algob[i];
      }
   }

   void RestoreData(AlgoData* ad) {
      ad->statusrgb = statusrgb;
      ad->gradient = gradient;
      ad->fromrgb = fromrgb;
      ad->torgb = torgb;
      for (int i = 0; i < ad->maxstates; i++) {
         ad->algor[i] = algor[i];
         ad->algog[i] = algog[i];
         ad->algob[i] = algob[i];
      }
   }
   
   // this must match color info in AlgoData
   wxColor statusrgb;
   bool gradient;
   wxColor fromrgb;
   wxColor torgb;
   unsigned char algor[256];
   unsigned char algog[256];
   unsigned char algob[256];
};

// -----------------------------------------------------------------------------

void SetColors()
{
   if (inscript || viewptr->waitingforclick) return;

   if (mainptr->generating) {
      // terminate generating loop and set command_pending flag
      mainptr->Stop();
      mainptr->command_pending = true;
      mainptr->cmdevent.SetId(ID_SETCOLORS);
      return;
   }
   
   if (swapcolors) InvertIconColors();
   
   // save current color info so we can restore it if user cancels changes
   wxColor save_deadrgb = *deadrgb;
   wxColor save_pastergb = *pastergb;
   wxColor save_selectrgb = *selectrgb;
   SaveData* save_info[MAX_ALGOS];
   for (int i = 0; i < NumAlgos(); i++) {
      save_info[i] = new SaveData(algoinfo[i]);
   }

   ColorDialog dialog( wxGetApp().GetTopWindow() );
   if ( dialog.ShowModal() == wxID_OK ) {
      // TransferDataFromWindow returned true;
      SetBrushesAndPens();
   } else {
      // user hit Cancel so restore color info saved above
      *deadrgb = save_deadrgb;
      *pastergb = save_pastergb;
      *selectrgb = save_selectrgb;
      for (int i = 0; i < NumAlgos(); i++) {
         save_info[i]->RestoreData(algoinfo[i]);
      }
   }

   for (int i = 0; i < NumAlgos(); i++) {
      delete save_info[i];
   }
   
   if (swapcolors) InvertIconColors();
}

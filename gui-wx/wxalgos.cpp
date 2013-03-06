/*** /
 
 This file is part of Golly, a Game of Life Simulator.
 Copyright (C) 2012 Andrew Trevorrow and Tomas Rokicki.
 
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

#include "wx/wxprec.h"  // for compilers that support precompilation
#ifndef WX_PRECOMP
#include "wx/wx.h"      // for all others include the necessary headers
#endif

#include "wx/filename.h"   // for wxFileName

#include "bigint.h"
#include "lifealgo.h"
#include "qlifealgo.h"
#include "hlifealgo.h"
#include "generationsalgo.h"
#include "jvnalgo.h"
#include "ruleloaderalgo.h"

#include "wxgolly.h"       // for wxGetApp
#include "wxmain.h"        // for ID_ALGO0
#include "wxutils.h"       // for Fatal, Warning
#include "wxrender.h"      // for DrawOneIcon
#include "wxprefs.h"       // for gollydir
#include "wxlayer.h"       // for currlayer
#include "wxalgos.h"

// -----------------------------------------------------------------------------

// exported data:

wxMenu* algomenu;                   // menu of algorithm names
wxMenu* algomenupop;                // copy of algomenu for PopupMenu calls
algo_type initalgo = QLIFE_ALGO;    // initial layer's algorithm
AlgoData* algoinfo[MAX_ALGOS];      // static info for each algorithm

wxBitmap** hexicons7x7;             // hexagonal icon bitmaps for scale 1:8
wxBitmap** hexicons15x15;           // hexagonal icon bitmaps for scale 1:16
wxBitmap** hexicons31x31;           // hexagonal icon bitmaps for scale 1:32

wxBitmap** vnicons7x7;              // diamond-shaped icon bitmaps for scale 1:8
wxBitmap** vnicons15x15;            // diamond-shaped icon bitmaps for scale 1:16
wxBitmap** vnicons31x31;            // diamond-shaped icon bitmaps for scale 1:32

// -----------------------------------------------------------------------------

// These default cell colors were generated by continuously finding the
// color furthest in rgb space from the closest of the already selected
// colors, black, and white.

static unsigned char default_colors[] = {
    48,48,48, // better if state 0 is dark gray (was 255,127,0)
    0,255,127,127,0,255,148,148,148,128,255,0,255,0,128,
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

// Note that all the default icons are grayscale bitmaps.
// These icons are used for lots of different rules with different numbers
// of states, and at rendering time we will replace the white pixels in each
// icon with the cell's state color to avoid "color shock" when switching
// between icon and non-icon view.  Gray pixels are used to do anti-aliasing.

// XPM data for default 7x7 icon
static const char* default7x7[] = {
// width height ncolors chars_per_pixel
"7 7 4 1",
// colors
". c #000000",    // black will be transparent
"D c #404040",
"E c #E0E0E0",
"W c #FFFFFF",    // white
// pixels
".DEWED.",
"DWWWWWD",
"EWWWWWE",
"WWWWWWW",
"EWWWWWE",
"DWWWWWD",
".DEWED."
};

// XPM data for default 15x15 icon
static const char* default15x15[] = {
// width height ncolors chars_per_pixel
"15 15 5 1",
// colors
". c #000000",    // black will be transparent
"D c #404040",
"C c #808080",
"B c #C0C0C0",
"W c #FFFFFF",    // white
// pixels
"...............",
"....DBWWWBD....",
"...BWWWWWWWB...",
"..BWWWWWWWWWB..",
".DWWWWWWWWWWWD.",
".BWWWWWWWWWWWB.",
".WWWWWWWWWWWWW.",
".WWWWWWWWWWWWW.",
".WWWWWWWWWWWWW.",
".BWWWWWWWWWWWB.",
".DWWWWWWWWWWWD.",
"..BWWWWWWWWWB..",
"...BWWWWWWWB...",
"....DBWWWBD....",
"..............."
};

// XPM data for default 31x31 icon
static const char* default31x31[] = {
// width height ncolors chars_per_pixel
"31 31 5 1",
// colors
". c #000000",    // black will be transparent
"D c #404040",
"C c #808080",
"B c #C0C0C0",
"W c #FFFFFF",    // white
// pixels
"...............................",
"...............................",
"..........DCBWWWWWBCD..........",
".........CWWWWWWWWWWWC.........",
".......DWWWWWWWWWWWWWWWD.......",
"......BWWWWWWWWWWWWWWWWWB......",
".....BWWWWWWWWWWWWWWWWWWWB.....",
"....DWWWWWWWWWWWWWWWWWWWWWD....",
"....WWWWWWWWWWWWWWWWWWWWWWW....",
"...CWWWWWWWWWWWWWWWWWWWWWWWC...",
"..DWWWWWWWWWWWWWWWWWWWWWWWWWD..",
"..CWWWWWWWWWWWWWWWWWWWWWWWWWC..",
"..BWWWWWWWWWWWWWWWWWWWWWWWWWB..",
"..WWWWWWWWWWWWWWWWWWWWWWWWWWW..",
"..WWWWWWWWWWWWWWWWWWWWWWWWWWW..",
"..WWWWWWWWWWWWWWWWWWWWWWWWWWW..",
"..WWWWWWWWWWWWWWWWWWWWWWWWWWW..",
"..WWWWWWWWWWWWWWWWWWWWWWWWWWW..",
"..BWWWWWWWWWWWWWWWWWWWWWWWWWB..",
"..CWWWWWWWWWWWWWWWWWWWWWWWWWC..",
"..DWWWWWWWWWWWWWWWWWWWWWWWWWD..",
"...CWWWWWWWWWWWWWWWWWWWWWWWC...",
"....WWWWWWWWWWWWWWWWWWWWWWW....",
"....DWWWWWWWWWWWWWWWWWWWWWD....",
".....BWWWWWWWWWWWWWWWWWWWB.....",
"......BWWWWWWWWWWWWWWWWWB......",
".......DWWWWWWWWWWWWWWWD.......",
".........CWWWWWWWWWWWC.........",
"..........DCBWWWWWBCD..........",
"...............................",
"..............................."
};

// XPM data for the 7x7 icon used for hexagonal CA
static const char* hex7x7[] = {
// width height ncolors chars_per_pixel
"7 7 3 1",
// colors
". c #000000",    // black will be transparent
"C c #808080",
"W c #FFFFFF",    // white
// pixels
".WWC...",
"WWWWW..",
"WWWWWW.",
"CWWWWWC",
".WWWWWW",
"..WWWWW",
"...CWW."};

// XPM data for the 15x15 icon used for hexagonal CA
static const char* hex15x15[] = {
// width height ncolors chars_per_pixel
"15 15 3 1",
// colors
". c #000000",    // black will be transparent
"C c #808080",
"W c #FFFFFF",    // white
// pixels
"...WWC.........",
"..WWWWWC.......",
".WWWWWWWWC.....",
"WWWWWWWWWWW....",
"WWWWWWWWWWWW...",
"CWWWWWWWWWWWC..",
".WWWWWWWWWWWW..",
".CWWWWWWWWWWWC.",
"..WWWWWWWWWWWW.",
"..CWWWWWWWWWWWC",
"...WWWWWWWWWWWW",
"....WWWWWWWWWWW",
".....CWWWWWWWW.",
".......CWWWWW..",
".........CWW..."};

// XPM data for 31x31 icon used for hexagonal CA
static const char* hex31x31[] = {
// width height ncolors chars_per_pixel
"31 31 3 1",
// colors
". c #000000",    // black will be transparent
"C c #808080",
"W c #FFFFFF",    // white
// pixels
".....WWC.......................",
"....WWWWWC.....................",
"...WWWWWWWWC...................",
"..WWWWWWWWWWWC.................",
".WWWWWWWWWWWWWWC...............",
"WWWWWWWWWWWWWWWWWC.............",
"WWWWWWWWWWWWWWWWWWWC...........",
"CWWWWWWWWWWWWWWWWWWWWC.........",
".WWWWWWWWWWWWWWWWWWWWWW........",
".CWWWWWWWWWWWWWWWWWWWWWC.......",
"..WWWWWWWWWWWWWWWWWWWWWW.......",
"..CWWWWWWWWWWWWWWWWWWWWWC......",
"...WWWWWWWWWWWWWWWWWWWWWW......",
"...CWWWWWWWWWWWWWWWWWWWWWC.....",
"....WWWWWWWWWWWWWWWWWWWWWW.....",
"....CWWWWWWWWWWWWWWWWWWWWWC....",
".....WWWWWWWWWWWWWWWWWWWWWW....",
".....CWWWWWWWWWWWWWWWWWWWWWC...",
"......WWWWWWWWWWWWWWWWWWWWWW...",
"......CWWWWWWWWWWWWWWWWWWWWWC..",
".......WWWWWWWWWWWWWWWWWWWWWW..",
".......CWWWWWWWWWWWWWWWWWWWWWC.",
"........WWWWWWWWWWWWWWWWWWWWWW.",
".........CWWWWWWWWWWWWWWWWWWWWC",
"...........CWWWWWWWWWWWWWWWWWWW",
".............CWWWWWWWWWWWWWWWWW",
"...............CWWWWWWWWWWWWWW.",
".................CWWWWWWWWWWW..",
"...................CWWWWWWWW...",
".....................CWWWWW....",
".......................CWW....."
};

// XPM data for the 7x7 icon used for von Neumann CA
static const char* vn7x7[] = {
// width height ncolors chars_per_pixel
"7 7 2 1",
// colors
". c #000000",    // black will be transparent
"W c #FFFFFF",    // white
// pixels
"...W...",
"..WWW..",
".WWWWW.",
"WWWWWWW",
".WWWWW.",
"..WWW..",
"...W..."
};

// XPM data for the 15x15 icon used for von Neumann CA
static const char* vn15x15[] = {
// width height ncolors chars_per_pixel
"15 15 2 1",
// colors
". c #000000",    // black will be transparent
"W c #FFFFFF",    // white
// pixels
"...............",
".......W.......",
"......WWW......",
".....WWWWW.....",
"....WWWWWWW....",
"...WWWWWWWWW...",
"..WWWWWWWWWWW..",
".WWWWWWWWWWWWW.",
"..WWWWWWWWWWW..",
"...WWWWWWWWW...",
"....WWWWWWW....",
".....WWWWW.....",
"......WWW......",
".......W.......",
"..............."
};

// XPM data for 31x31 icon used for von Neumann CA
static const char* vn31x31[] = {
// width height ncolors chars_per_pixel
"31 31 2 1",
// colors
". c #000000",    // black will be transparent
"W c #FFFFFF",    // white
// pixels
"...............................",
"...............................",
"...............W...............",
"..............WWW..............",
".............WWWWW.............",
"............WWWWWWW............",
"...........WWWWWWWWW...........",
"..........WWWWWWWWWWW..........",
".........WWWWWWWWWWWWW.........",
"........WWWWWWWWWWWWWWW........",
".......WWWWWWWWWWWWWWWWW.......",
"......WWWWWWWWWWWWWWWWWWW......",
".....WWWWWWWWWWWWWWWWWWWWW.....",
"....WWWWWWWWWWWWWWWWWWWWWWW....",
"...WWWWWWWWWWWWWWWWWWWWWWWWW...",
"..WWWWWWWWWWWWWWWWWWWWWWWWWWW..",
"...WWWWWWWWWWWWWWWWWWWWWWWWW...",
"....WWWWWWWWWWWWWWWWWWWWWWW....",
".....WWWWWWWWWWWWWWWWWWWWW.....",
"......WWWWWWWWWWWWWWWWWWW......",
".......WWWWWWWWWWWWWWWWW.......",
"........WWWWWWWWWWWWWWW........",
".........WWWWWWWWWWWWW.........",
"..........WWWWWWWWWWW..........",
"...........WWWWWWWWW...........",
"............WWWWWWW............",
".............WWWWW.............",
"..............WWW..............",
"...............W...............",
"...............................",
"..............................."
};

// -----------------------------------------------------------------------------

wxBitmap** CreateIconBitmaps(const char** xpmdata, int maxstates)
{
    if (xpmdata == NULL) return NULL;
    
    wxImage image(xpmdata);
    
    if (!image.HasAlpha()) {
        // add alpha channel and set to opaque
        image.InitAlpha();
    }

#ifdef __WXGTK__
    // need alpha channel on Linux
    //???!!! image.SetMaskColour(0, 0, 0);    // make black transparent
#endif
    
    wxBitmap allicons(image, 32);    // RGBA
    
    int wd = allicons.GetWidth();
    int numicons = allicons.GetHeight() / wd;
    if (numicons > 255) numicons = 255;          // play safe
    
    wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
    if (iconptr) {
        // initialize all pointers (not just those < maxstates)
        for (int i = 0; i < 256; i++) iconptr[i] = NULL;
        
        for (int i = 0; i < numicons; i++) {
            wxRect rect(0, i*wd, wd, wd);
            // add 1 to skip iconptr[0] (ie. dead state)
            iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
        }
        
        if (numicons < maxstates-1 && iconptr[numicons]) {
            // duplicate last icon
            wxRect rect(0, (numicons-1)*wd, wd, wd);
            for (int i = numicons; i < maxstates-1; i++) {
                iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
            }
        }
    }
    return iconptr;
}

// -----------------------------------------------------------------------------

void FreeIconBitmaps(wxBitmap** icons)
{
    if (icons) {
        for (int i = 0; i < 256; i++) delete icons[i];
        free(icons);
    }
}

// -----------------------------------------------------------------------------

wxBitmap** ScaleIconBitmaps(wxBitmap** srcicons, int size)
{
    if (srcicons == NULL) return NULL;
    
    wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
    if (iconptr) {
        for (int i = 0; i < 256; i++) {
            if (srcicons[i] == NULL) {
                iconptr[i] = NULL;
            } else {
                wxImage image = srcicons[i]->ConvertToImage();
                // do NOT scale using wxIMAGE_QUALITY_HIGH (thin lines can disappear)
                image.Rescale(size, size, wxIMAGE_QUALITY_NORMAL);
                if (!image.HasAlpha()) {
                    // add alpha channel and set to opaque
                    image.InitAlpha();
                }
                iconptr[i] = new wxBitmap(image, 32);
            }
        }
    }
    return iconptr;
}

// -----------------------------------------------------------------------------

static void CreateDefaultIcons(AlgoData* ad)
{
    if (ad->defxpm7x7 || ad->defxpm15x15 || ad->defxpm31x31) {
        // create icons using given algo's default XPM data
        ad->icons7x7 = CreateIconBitmaps(ad->defxpm7x7, ad->maxstates);
        ad->icons15x15 = CreateIconBitmaps(ad->defxpm15x15, ad->maxstates);
        ad->icons31x31 = CreateIconBitmaps(ad->defxpm31x31, ad->maxstates);
        
        // create scaled bitmaps if size(s) not supplied
        if (!ad->icons7x7) {
            if (ad->icons15x15)
                // scale down 15x15 bitmaps
                ad->icons7x7 = ScaleIconBitmaps(ad->icons15x15, 7);
            else
                // scale down 31x31 bitmaps
                ad->icons7x7 = ScaleIconBitmaps(ad->icons31x31, 7);
        }
        if (!ad->icons15x15) {
            if (ad->icons31x31)
                // scale down 31x31 bitmaps
                ad->icons15x15 = ScaleIconBitmaps(ad->icons31x31, 15);
            else
                // scale up 7x7 bitmaps
                ad->icons15x15 = ScaleIconBitmaps(ad->icons7x7, 15);
        }
        if (!ad->icons31x31) {
            if (ad->icons15x15)
                // scale up 15x15 bitmaps
                ad->icons31x31 = ScaleIconBitmaps(ad->icons15x15, 31);
            else
                // scale up 7x7 bitmaps
                ad->icons31x31 = ScaleIconBitmaps(ad->icons7x7, 31);
        }
    } else {
        // algo didn't supply any icons so use static XPM data defined above
        ad->icons7x7 = CreateIconBitmaps(default7x7, ad->maxstates);
        ad->icons15x15 = CreateIconBitmaps(default15x15, ad->maxstates);
        ad->icons31x31 = CreateIconBitmaps(default31x31, ad->maxstates);
    }
}

// -----------------------------------------------------------------------------

AlgoData::AlgoData() {
    algomem = defbase = 0;
    statusbrush = NULL;
    icons7x7 = NULL;
    icons15x15 = NULL;
    icons31x31 = NULL;
}

// -----------------------------------------------------------------------------

AlgoData::~AlgoData() 
{
    FreeIconBitmaps(icons7x7);
    FreeIconBitmaps(icons15x15);
    FreeIconBitmaps(icons31x31);
    delete statusbrush;
}

// -----------------------------------------------------------------------------

AlgoData& AlgoData::tick() {
    AlgoData* r = new AlgoData();
    algoinfo[r->id] = r;
    return *r;
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
    ruleloaderalgo::doInitializeAlgoInfo(AlgoData::tick());
    
    // algomenu is used for the Control > Set Algorithm submenu;
    // algomenupop is used when the tool bar's algo button is pressed
    // (we can't share a single menu for both purposes because we get
    // assert messages with wxOSX and wxGTK 2.9+)
    algomenu = new wxMenu();
    algomenupop = new wxMenu();
    
    // init algoinfo array
    for (int i = 0; i < NumAlgos(); i++) {
        AlgoData* ad = algoinfo[i];
        if (ad->algoName == 0 || ad->creator == 0)
            Fatal(_("Algorithm did not set name and/or creator"));
        
        wxString name = wxString(ad->algoName, wxConvLocal);
        algomenu->AppendCheckItem(ID_ALGO0 + i, name);
        algomenupop->AppendCheckItem(ID_ALGO0 + i, name);
        
        // does algo use hashing?
        ad->canhash = ad->defbase == 8;    //!!! safer method needed???
        
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
        
        // initialize default color scheme
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
        ad->gradient = ad->defgradient;
        ad->fromrgb.Set(ad->defr1, ad->defg1, ad->defb1);
        ad->torgb.Set(ad->defr2, ad->defg2, ad->defb2);
        for (int c = 0; c < ad->maxstates; c++) {
            ad->algor[c] = ad->defr[c];
            ad->algog[c] = ad->defg[c];
            ad->algob[c] = ad->defb[c];
        }
        
        CreateDefaultIcons(ad);
    }
    
    hexicons7x7 = CreateIconBitmaps(hex7x7,256);
    hexicons15x15 = CreateIconBitmaps(hex15x15,256);
    hexicons31x31 = CreateIconBitmaps(hex31x31,256);
    
    vnicons7x7 = CreateIconBitmaps(vn7x7,256);
    vnicons15x15 = CreateIconBitmaps(vn15x15,256);
    vnicons31x31 = CreateIconBitmaps(vn31x31,256);
}

// -----------------------------------------------------------------------------

void DeleteAlgorithms()
{
    for (int i = 0; i < NumAlgos(); i++)
        delete algoinfo[i];
    FreeIconBitmaps(hexicons7x7);
    FreeIconBitmaps(hexicons15x15);
    FreeIconBitmaps(hexicons31x31);
    FreeIconBitmaps(vnicons7x7);
    FreeIconBitmaps(vnicons15x15);
    FreeIconBitmaps(vnicons31x31);
    delete algomenupop;
}

// -----------------------------------------------------------------------------

lifealgo* CreateNewUniverse(algo_type algotype, bool allowcheck)
{
    lifealgo* newalgo = algoinfo[algotype]->creator();
    
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

bool MultiColorImage(wxImage& image)
{
    // return true if image contains at least one color that isn't a shade of gray
    int numpixels = image.GetWidth() * image.GetHeight();
    bool skipalpha = image.HasAlpha();
    unsigned char* p = image.GetData();
    for (int i = 0; i < numpixels; i++) {
        unsigned char r = *p++;
        unsigned char g = *p++;
        unsigned char b = *p++;
        if (r != g || g != b) return true;
        if (skipalpha) p++;
    }
    return false;   // grayscale image
}

// -----------------------------------------------------------------------------

bool LoadIconFile(const wxString& path, int maxstate,
                  wxBitmap*** out7x7, wxBitmap*** out15x15, wxBitmap*** out31x31)
{
    wxImage image;
    if (!image.LoadFile(path)) {
        Warning(_("Could not load icon bitmaps from file:\n") + path);
        return false;
    }

    if (!image.HasAlpha()) {
        // add alpha channel and set to opaque
        image.InitAlpha();
    }
    
#ifdef __WXGTK__
    // need alpha channel on Linux
    //!!!??? image.SetMaskColour(0, 0, 0);    // make black transparent
#endif
    
    // check for multi-color icons
    currlayer->multicoloricons = MultiColorImage(image);
    
    wxBitmap allicons(image, 32);   // RGBA
    int wd = allicons.GetWidth();
    int ht = allicons.GetHeight();
    
    // check dimensions
    if (ht != 15 && ht != 22) {
        Warning(_("Wrong bitmap height in icon file (must be 15 or 22):\n") + path);
        return false;
    }
    if (wd % 15 != 0) {
        Warning(_("Wrong bitmap width in icon file (must be multiple of 15):\n") + path);
        return false;
    }
    
    // first extract 15x15 icons
    int numicons = wd / 15;
    if (numicons > 255) numicons = 255;    // play safe
    
    wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
    if (iconptr) {
        for (int i = 0; i < 256; i++) iconptr[i] = NULL;
        for (int i = 0; i < numicons; i++) {
            wxRect rect(i*15, 0, 15, 15);
            // add 1 to skip iconptr[0] (ie. dead state)
            iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
        }
        if (numicons < maxstate && iconptr[numicons]) {
            // duplicate last icon
            wxRect rect((numicons-1)*15, 0, 15, 15);
            for (int i = numicons; i < maxstate; i++) {
                iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
            }
        }
        
        // if there is an extra icon at the right end of the multi-color icons then
        // store it in iconptr[0] -- it will be used later in UpdateCurrentColors()
        // to set the color of state 0
        if (currlayer->multicoloricons && (wd / 15) > maxstate) {
            wxRect rect(maxstate*15, 0, 15, 15);
            iconptr[0] = new wxBitmap(allicons.GetSubBitmap(rect));
        }
    }
    *out15x15 = iconptr;
    
    if (ht == 22) {
        // extract 7x7 icons (at bottom left corner of each 15x15 icon)
        iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
        if (iconptr) {
            for (int i = 0; i < 256; i++) iconptr[i] = NULL;
            for (int i = 0; i < numicons; i++) {
                wxRect rect(i*15, 15, 7, 7);
                // add 1 to skip iconptr[0] (ie. dead state)
                iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
            }
            if (numicons < maxstate && iconptr[numicons]) {
                // duplicate last icon
                wxRect rect((numicons-1)*15, 15, 7, 7);
                for (int i = numicons; i < maxstate; i++) {
                    iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
                }
            }
        }
        *out7x7 = iconptr;
    } else {
        // create 7x7 icons by scaling down 15x15 icons
        *out7x7 = ScaleIconBitmaps(*out15x15, 7);
    }

    // create 31x31 icons by scaling up 15x15 icons
    *out31x31 = ScaleIconBitmaps(*out15x15, 31);
    
    return true;
}

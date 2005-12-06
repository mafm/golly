                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2005 Andrew Trevorrow and Tomas Rokicki.

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

/* A GUI for Golly, implemented in wxWidgets (www.wxwidgets.org).
   Unfinished code is flagged by "!!!".
   Uncertain code is flagged by "???".
*/

#include "wx/wxprec.h"     // for compilers that support precompilation
#ifndef WX_PRECOMP
   #include "wx/wx.h"      // for all others include the necessary headers
#endif

#include "wx/image.h"      // for wxImage
#include "wx/stdpaths.h"   // for wxStandardPaths
#include "wx/sysopt.h"     // for wxSystemOptions

#include "lifealgo.h"
#include "lifepoll.h"
#include "util.h"          // for lifeerrors

#include "wxgolly.h"       // defines GollyApp class
#include "wxmain.h"        // defines MainFrame class
#include "wxstatus.h"      // defines StatusBar class
#include "wxview.h"        // defines PatternView class
#include "wxutils.h"       // for Warning, Fatal, BeginProgress, etc
#include "wxhelp.h"        // for GetHelpFrame
#include "wxprefs.h"       // for GetPrefs

#ifdef __WXMSW__
   // app icons are loaded via .rc file
#else
   #include "appicon.xpm"
#endif

// -----------------------------------------------------------------------------

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution and also implements the
// accessor function wxGetApp() which will return the reference of the correct
// type (i.e. GollyApp and not wxApp).

IMPLEMENT_APP(GollyApp)

// -----------------------------------------------------------------------------

#define STRINGIFY(arg) STR2(arg)
#define STR2(arg) #arg
const char *BANNER = "This is Golly version " STRINGIFY(VERSION)
                     ".  Copyright 2005 The Golly Gang.";

MainFrame *mainptr = NULL;       // main window
PatternView *viewptr = NULL;     // viewport child window (in main window)
StatusBar *statusptr = NULL;     // status bar child window (in main window)
lifealgo *curralgo = NULL;       // current life algorithm (qlife or hlife)

// -----------------------------------------------------------------------------

// let non-wx modules call Fatal, Warning, BeginProgress, etc

class wx_errors : public lifeerrors
{
public:
   virtual void fatal(const char *s) { Fatal(s); }
   virtual void warning(const char *s) { Warning(s); }
   virtual void status(const char *s) { statusptr->DisplayMessage(s); }
   virtual void beginprogress(const char *s) {
      BeginProgress(s);
      aborted = false;     // needed for isaborted() calls in non-wx modules
   }
   virtual bool abortprogress(double f, const char *s) {
      return AbortProgress(f, s);
   }
   virtual void endprogress() { EndProgress(); }
};

wx_errors wxerrhandler;    // create instance

// -----------------------------------------------------------------------------

// let non-wx modules process events

class wx_poll : public lifepoll
{
public:
   virtual int checkevents();
   virtual void updatePop();
   long nextcheck;
};

int wx_poll::checkevents()
{
   #ifdef __WXMSW__
      // on Windows wxGetElapsedTime has a higher overhead than Yield
      wxGetApp().Yield(true);
      if (GetHelpFrame() && GetHelpFrame()->IsActive()) {
         // send idle events to html window so cursor gets updated
         wxIdleEvent event;
         wxGetApp().SendIdleEvents(GetHtmlWindow(), event);
      }
   #else
      // on Mac and X11 it is much faster to avoid calling Yield too often
      long t = wxGetElapsedTime(false);
      if (t > nextcheck) {
         nextcheck = t + 50;        // 20th of a sec
         wxGetApp().Yield(true);
         #ifdef __WXMAC__
            if (GetHelpFrame() && GetHelpFrame()->IsActive()) {
               // send idle events to html window so cursor gets updated
               wxIdleEvent event;
               wxGetApp().SendIdleEvents(GetHtmlWindow(), event);
            }
         #endif
      }
   #endif
   return isInterrupted();
}

void wx_poll::updatePop()
{
   if (mainptr->StatusVisible()) {
      statusptr->Refresh(false, NULL);
      statusptr->Update();
   }
}

wx_poll wxpoller;    // create instance

lifepoll* GollyApp::Poller()
{
   return &wxpoller;
}

void GollyApp::PollerReset()
{
   wxpoller.resetInterrupted();
   wxpoller.nextcheck = 0;
}

void GollyApp::PollerInterrupt()
{
   wxpoller.setInterrupted();
   wxpoller.nextcheck = 0;
}

// -----------------------------------------------------------------------------

void SetAppDirectory(const char *argv0)
{
   #ifdef __WXMSW__
      // on Windows we need to reset current directory to app directory if user
      // dropped file from somewhere else onto app to start it up (otherwise we
      // can't find Help files and prefs file gets saved to wrong location)
      wxStandardPaths wxstdpaths;
      wxString appdir = wxstdpaths.GetDataDir();
      wxString currdir = wxGetCwd();
      if ( currdir.CmpNoCase(appdir) != 0 )
         wxSetWorkingDirectory(appdir);
      // avoid VC++ warning
      if (argv0) currdir = wxEmptyString;
   #elif defined(__WXMAC__)
      // wxMac has set current directory to location of .app bundle so no need
      // to do anything
   #elif defined(__UNIX__)
      // user might have started app from a different directory so find
      // last "/" in argv0 and change cwd if "/" isn't part of "./" prefix
      unsigned int pos = strlen(argv0);
      while (pos > 0) {
         pos--;
         if (argv0[pos] == '/') break;
      }
      if ( pos > 0 && !(pos == 1 && argv0[0] == '.') ) {
         char appdir[2048];
         if (pos < sizeof(appdir)) {
            strncpy(appdir, argv0, pos);
            appdir[pos] = 0;
            wxSetWorkingDirectory(appdir);
         }
      }
   #endif
}

// -----------------------------------------------------------------------------

void GollyApp::SetFrameIcon(wxFrame *frame)
{
   // set frame icon
   #ifdef __WXMSW__
      // create a bundle with 32x32 and 16x16 icons
      wxIconBundle icb(wxICON(appicon0));
      icb.AddIcon(wxICON(appicon1));
      frame->SetIcons(icb);
   #else
      // use appicon.xpm on other platforms (ignored on Mac)
      frame->SetIcon(wxICON(appicon));
   #endif
}

// -----------------------------------------------------------------------------

#ifdef __WXMAC__
// handle odoc event
void GollyApp::MacOpenFile(const wxString &fullPath)
{
   if (mainptr->generating) return;

   mainptr->Raise();
   // need to process events to avoid crash if info window was in front
   while (wxGetApp().Pending()) wxGetApp().Dispatch();

   // convert path to UTF8 encoding so fopen will work
   mainptr->ConvertPathAndOpen(fullPath, true);
}
#endif

// -----------------------------------------------------------------------------

// app execution starts here
bool GollyApp::OnInit()
{
   #ifdef __WXMAC__
      // prevent rectangle animation when windows open/close
      wxSystemOptions::SetOption(wxMAC_WINDOW_PLAIN_TRANSITION, 1);
      // prevent position problem in wxTextCtrl with wxTE_DONTWRAP style
      // (but doesn't fix problem with I-beam cursor over scroll bars)
      wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_MLTE, 1);
   #endif

   // make sure current working directory contains application otherwise
   // we can't open Help files and prefs file gets saved in wrong location
   SetAppDirectory(argv[0]);

   // let non-wx modules call Fatal, Warning, BeginProgress, etc
   lifeerrors::seterrorhandler(&wxerrhandler);

   // start timer so we can use wxGetElapsedTime(false) to get elapsed millisecs
   wxStartTimer();

   // allow our .html files to include common graphic formats;
   // note that wxBMPHandler is always installed
   wxImage::AddHandler(new wxPNGHandler);
   wxImage::AddHandler(new wxGIFHandler);
   wxImage::AddHandler(new wxJPEGHandler);
   
   // get main window location and other user preferences
   GetPrefs();
   
   // create main window (also inits viewptr and statusptr)
   mainptr = new MainFrame();
   if (mainptr == NULL) Fatal("Failed to create main window!");
   
   // initialize some stuff before showing main window
   mainptr->SetRandomFillPercentage();
   mainptr->SetMinimumWarp();
   viewptr->SetViewSize();
   statusptr->SetMessage(BANNER);
   
   // load pattern if file supplied on Win/Unix command line
   if (argc > 1) {
      // no need to convert path to UTF8
      mainptr->ConvertPathAndOpen(argv[1], false);
   } else {
      mainptr->NewPattern();
   }   

   if (maximize) mainptr->Maximize(true);
   if (!showstatus) mainptr->ToggleStatusBar();
   if (!showtool) mainptr->ToggleToolBar();

   // now show main window
   mainptr->Show(true);
   SetTopWindow(mainptr);

   #ifdef __WXX11__
      // prevent main window being resized very small to avoid nasty errors
      // mainptr->SetMinSize(wxSize(minmainwd, minmainht));
      // above works but moves window to default pos!!!
      // and calling Move clobbers effect of SetMinSize!!! sigh
      // wxGetApp().Yield(true);
      // mainptr->Move(mainx, mainy);
   #endif

   // true means call wxApp::OnRun() which will enter the main event loop;
   // false means exit immediately
   return true;
}

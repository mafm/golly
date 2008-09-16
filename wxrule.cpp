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

#include "wx/wxhtml.h"     // for wxHtmlWindow

#include "lifealgo.h"

#include "wxgolly.h"       // for wxGetApp, mainptr
#include "wxmain.h"        // for mainptr->...
#include "wxprefs.h"       // for namedrules, gollydir, etc
#include "wxutils.h"       // for Warning, Fatal
#include "wxlayer.h"       // for currlayer
#include "wxalgos.h"       // for NumAlgos, CreateNewUniverse, etc
#include "wxrule.h"

// -----------------------------------------------------------------------------

bool ValidRule(wxString& rule)
{
   // return true if given rule is valid in at least one algorithm
   // and convert rule to canonical form
   
   // qlife and hlife share global_liferules, so we need to save
   // and restore the current rule -- yuk
   wxString oldrule = wxString(currlayer->algo->getrule(),wxConvLocal);
   
   for (int i = 0; i < NumAlgos(); i++) {
      lifealgo* tempalgo = CreateNewUniverse(i);
      const char* err = tempalgo->setrule( rule.mb_str(wxConvLocal) );
      if (!err) {
         // convert rule to canonical form
         rule = wxString(tempalgo->getrule(),wxConvLocal);
         delete tempalgo;
         currlayer->algo->setrule( oldrule.mb_str(wxConvLocal) );
         return true;
      }
      delete tempalgo;
   }
   currlayer->algo->setrule( oldrule.mb_str(wxConvLocal) );
   return false;
}

// -----------------------------------------------------------------------------

bool MatchingRules(const wxString& rule1, const wxString& rule2)
{
   // return true if given strings are equivalent rules
   if (rule1 == rule2) {
      return true;
   } else {
      // we want "s23b3" or "23/3" to match "B3/S23" so convert given rules
      // to canonical form (if valid) and then compare
      wxString canon1 = rule1;
      wxString canon2 = rule2;
      return ValidRule(canon1) && ValidRule(canon2) && canon1 == canon2;
   }
}

// -----------------------------------------------------------------------------

wxString GetRuleName(const wxString& rulestring)
{
   // search namedrules array for matching rule
   wxString rulename;
   size_t i;
   for (i = 0; i < namedrules.GetCount(); i++) {
      // extract rule after '|'
      wxString thisrule = namedrules[i].AfterFirst('|');
      if ( MatchingRules(rulestring, thisrule) ) {
         // extract name before '|'
         rulename = namedrules[i].BeforeFirst('|');
         return rulename;
      }
   }
   // given rulestring has not been named
   rulename = rulestring;
   return rulename;
}

// -----------------------------------------------------------------------------

// define a html window for displaying algo help:

class AlgoHelp : public wxHtmlWindow
{
public:
   AlgoHelp(wxWindow* parent, wxWindowID id, const wxPoint& pos,
            const wxSize& size, long style)
      : wxHtmlWindow(parent, id, pos, size, style) { }

   virtual void OnLinkClicked(const wxHtmlLinkInfo& link);

   void DisplayFile(const wxString& filepath);

private:
   void OnKeyUp(wxKeyEvent& event);
   void OnSize(wxSizeEvent& event);
   void OnMouseDown(wxMouseEvent& event);
   void OnSetFocus(wxFocusEvent& event);
   void OnKillFocus(wxFocusEvent& event);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AlgoHelp, wxHtmlWindow)
#ifdef __WXGTK__
   // don't call OnKeyUp in wxGTK because for some reason it prevents
   // copied help text being pasted into the rule box
#else
   EVT_KEY_UP        (AlgoHelp::OnKeyUp)
#endif
#ifdef __WXMAC__
   // on the Mac OnKeyUp is also called on a key-down event because the
   // key-up handler gets a key code of 400 if cmd-C is pressed quickly
   EVT_KEY_DOWN      (AlgoHelp::OnKeyUp)
   //!!! try to fix wxMac bug losing focus after scroll bar clicked
   /* this failed
   EVT_LEFT_DOWN     (AlgoHelp::OnMouseDown)
   EVT_LEFT_DCLICK   (AlgoHelp::OnMouseDown)
   EVT_SET_FOCUS     (AlgoHelp::OnSetFocus)
   EVT_KILL_FOCUS    (AlgoHelp::OnKillFocus)
   */
#endif
   EVT_SIZE          (AlgoHelp::OnSize)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void AlgoHelp::OnLinkClicked(const wxHtmlLinkInfo& link)
{
   wxString url = link.GetHref();
   if ( url.StartsWith(wxT("http:")) || url.StartsWith(wxT("mailto:")) ) {
      // pass http/mailto URL to user's preferred browser/emailer
      #ifdef __WXMAC__
         // wxLaunchDefaultBrowser doesn't work on Mac with IE (get msg in console.log)
         // but it's easier just to use the Mac OS X open command
         if ( wxExecute(wxT("open ") + url, wxEXEC_ASYNC) == -1 )
            Warning(_("Could not open URL!"));
      #elif defined(__WXGTK__)
         // wxLaunchDefaultBrowser is not reliable on Linux/GTK so we call gnome-open;
         // unfortunately it does not bring browser to front if it's already running
         if ( wxExecute(wxT("gnome-open ") + url, wxEXEC_ASYNC) == -1 )
            Warning(_("Could not open URL!"));
      #else
         if ( !wxLaunchDefaultBrowser(url) )
            Warning(_("Could not launch browser!"));
      #endif

   } else {
      // assume it's a link to a local target or another help file
      DisplayFile(url);
   }
}

// -----------------------------------------------------------------------------

void AlgoHelp::DisplayFile(const wxString& filepath)
{
   if ( filepath.IsEmpty() ) {
      wxString contents =
      wxT("<html><body bgcolor=\"#FFFFCE\">")
      wxT("<p>The given rule is not valid in any algorithm.")
      wxT("</body></html>");
      SetPage(contents);

   } else if ( filepath.StartsWith(gollydir) &&
               !wxFileName::FileExists(filepath) ) {
      wxString contents =
      wxT("<html><body bgcolor=\"#FFFFCE\">")
      wxT("<p>There is no help available for this algorithm.")
      wxT("</body></html>");
      SetPage(contents);

   } else {
      LoadPage(filepath);
   }
}

// -----------------------------------------------------------------------------

// we have to use a key-up handler otherwise wxHtmlWindow's key-up handler
// gets called which detects cmd/ctrl-C and clobbers our clipboard fix
void AlgoHelp::OnKeyUp(wxKeyEvent& event)
{
   int key = event.GetKeyCode();
   if (event.CmdDown()) {
      // let cmd-A select all text
      if (key == 'A') {
         SelectAll();
         event.Skip(false);
         return;
      }
      #ifdef __WXMAC__
         // let cmd-W close dialog
         if (key == 'W') {
            GetParent()->Close(true);
            event.Skip(false);
            return;
         }
      #endif
   }
   if ( event.CmdDown() || event.AltDown() ) {
      if ( key == 'C' ) {
         // copy any selected text to the clipboard
         wxString text = SelectionToText();
         if ( text.Length() > 0 ) {
            // remove any leading/trailing white space
            while (text.GetChar(0) <= wxChar(' ')) {
               text = text.Mid(1);
            }
            while (text.GetChar(text.Length()-1) <= wxChar(' ')) {
               text.Truncate(text.Length()-1);
            }
            mainptr->CopyTextToClipboard(text);
            // don't call wxHtmlWindow's default key-up handler
            event.Skip(false);
            return;
         }
      }
   }
   event.Skip();
}

// -----------------------------------------------------------------------------

void AlgoHelp::OnSize(wxSizeEvent& event)
{
   // avoid scroll position being reset to top when wxHtmlWindow is resized
   int x, y;
   GetViewStart(&x, &y);         // save current position

   wxHtmlWindow::OnSize(event);

   wxString currpage = GetOpenedPage();
   if ( !currpage.IsEmpty() ) {
      DisplayFile(currpage);     // reload page
      Scroll(x, y);              // scroll to old position
   }
   
   // prevent wxHtmlWindow::OnSize being called again
   event.Skip(false);
}

// -----------------------------------------------------------------------------

void AlgoHelp::OnMouseDown(wxMouseEvent& event)
{
   // this failed to fix wxMac bug
   // SetFocus();
   
   // this also failed
   wxWindow* former = FindFocus();
   if (former == this) {
      //!!!printf("html win has focus\n");
   } else {
      //!!!printf("html win does NOT have focus\n");

      wxFocusEvent killevt(wxEVT_KILL_FOCUS, this->GetId());
      killevt.SetEventObject(this);
      this->GetEventHandler()->ProcessEvent(killevt);

      wxFocusEvent setevt(wxEVT_SET_FOCUS, this->GetId());
      setevt.SetEventObject(this);
      this->GetEventHandler()->ProcessEvent(setevt);
   }

   event.Skip();
}

// -----------------------------------------------------------------------------

void AlgoHelp::OnSetFocus(wxFocusEvent& event)
{
   //!!!printf("set focus\n");
   event.Skip();
}

// -----------------------------------------------------------------------------

void AlgoHelp::OnKillFocus(wxFocusEvent& event)
{
   //!!!printf("kill focus\n");
   event.Skip();
}

// -----------------------------------------------------------------------------

const wxString UNKNOWN = _("UNKNOWN");    // unknown algorithm
const wxString UNNAMED = _("UNNAMED");    // unnamed rule

const int HGAP = 12;
const int BIGVGAP = 12;

#if defined(__WXMAC__) && wxCHECK_VERSION(2,8,0)
   // fix wxALIGN_CENTER_VERTICAL bug in wxMac 2.8.0+;
   // only happens when a wxStaticText/wxButton box is next to a wxChoice box
   #define FIX_ALIGN_BUG wxBOTTOM,4
#else
   #define FIX_ALIGN_BUG wxALL,0
#endif

// -----------------------------------------------------------------------------

// define a modal dialog for changing the current rule

class RuleDialog : public wxDialog
{
public:
   RuleDialog(wxWindow* parent);
   virtual bool TransferDataFromWindow();    // called when user hits OK

private:
   enum {
      // control ids
      RULE_ALGO = wxID_HIGHEST + 1,
      RULE_NAME,
      RULE_TEXT,
      RULE_ADD_BUTT,
      RULE_ADD_TEXT,
      RULE_DEL_BUTT
   };

   void CreateControls();     // initialize all the controls
   void UpdateAlgo();         // update algochoice depending on rule
   void UpdateName();         // update namechoice depending on rule
   void UpdateHelp();         // update algo help

   AlgoHelp* htmlwin;         // html window for displaying algo help

   wxTextCtrl* ruletext;      // text box for user to type in rule
   wxTextCtrl* addtext;       // text box for user to type in name of rule

   wxChoice* algochoice;      // lists the known algorithms but can have one
                              // more item appended (UNKNOWN)
   wxChoice* namechoice;      // kept in sync with namedrules but can have one
                              // more item appended (UNNAMED)
   
   int algoindex;             // current algochoice selection
   int nameindex;             // current namechoice selection
   bool ignore_text_change;   // prevent OnRuleTextChanged doing anything?
   bool expanded;             // help button has expanded dialog?
   wxRect minrect;            // window rect for unexpanded dialog
   
   // event handlers
   void OnRuleTextChanged(wxCommandEvent& event);
   void OnChooseAlgo(wxCommandEvent& event);
   void OnChooseName(wxCommandEvent& event);
   void OnHelpButton(wxCommandEvent& event);
   void OnAddName(wxCommandEvent& event);
   void OnDeleteName(wxCommandEvent& event);
   void OnUpdateAdd(wxUpdateUIEvent& event);
   void OnUpdateDelete(wxUpdateUIEvent& event);
   void OnSize(wxSizeEvent& event);
   void OnMove(wxMoveEvent& event);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(RuleDialog, wxDialog)
   EVT_CHOICE     (RULE_ALGO,       RuleDialog::OnChooseAlgo)
   EVT_CHOICE     (RULE_NAME,       RuleDialog::OnChooseName)
   EVT_TEXT       (RULE_TEXT,       RuleDialog::OnRuleTextChanged)
   EVT_BUTTON     (wxID_HELP,       RuleDialog::OnHelpButton)
   EVT_BUTTON     (RULE_ADD_BUTT,   RuleDialog::OnAddName)
   EVT_BUTTON     (RULE_DEL_BUTT,   RuleDialog::OnDeleteName)
   EVT_UPDATE_UI  (RULE_ADD_BUTT,   RuleDialog::OnUpdateAdd)
   EVT_UPDATE_UI  (RULE_DEL_BUTT,   RuleDialog::OnUpdateDelete)
   EVT_SIZE       (                 RuleDialog::OnSize)
   EVT_MOVE       (                 RuleDialog::OnMove)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

RuleDialog::RuleDialog(wxWindow* parent)
{
   Create(parent, wxID_ANY, _("Set Rule"), wxPoint(rulex,ruley), wxDefaultSize,
          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
   
   expanded = false;    // tested in RuleDialog::OnSize

   ignore_text_change = true;
   CreateControls();
   ignore_text_change = false;
   
   // dialog location is now set to rulex,ruley
   // Centre();

   minrect = GetRect();
   // don't allow resizing when dialog isn't expanded
   SetMaxSize( wxSize(minrect.width,minrect.height) );

   htmlwin = new AlgoHelp(this, wxID_ANY,
                          // specify small size to avoid clipping scroll bar on resize
                          wxDefaultPosition, wxSize(30,30),
                          wxHW_DEFAULT_STYLE | wxSUNKEN_BORDER);
   if (!htmlwin) Fatal(_("Could not create algo help window!"));
   htmlwin->SetBorders(4);
   htmlwin->Show(false);

   // select all of rule text
   ruletext->SetFocus();
   ruletext->SetSelection(0,999);   // wxMac bug: -1,-1 doesn't work here
}

// -----------------------------------------------------------------------------

void RuleDialog::CreateControls()
{
   wxStaticText* textlabel = new wxStaticText(this, wxID_STATIC, _("Enter a new rule:"));
   wxStaticText* namelabel = new wxStaticText(this, wxID_STATIC, _("Or select a named rule:"));

   wxButton* helpbutt = new wxButton(this, wxID_HELP, wxEmptyString);
   wxButton* delbutt = new wxButton(this, RULE_DEL_BUTT, _("Delete"));
   wxButton* addbutt = new wxButton(this, RULE_ADD_BUTT, _("Add"));
   
   // create a choice menu to select algo
   wxArrayString algoarray;
   for (int i = 0; i < NumAlgos(); i++) {
      algoarray.Add( wxString(GetAlgoName(i),wxConvLocal) );
   }
   algochoice = new wxChoice(this, RULE_ALGO, wxDefaultPosition, wxDefaultSize, algoarray);
   algoindex = currlayer->algtype;
   algochoice->SetSelection(algoindex);

   wxBoxSizer* hbox0 = new wxBoxSizer(wxHORIZONTAL);
   wxBoxSizer* algolabel = new wxBoxSizer(wxHORIZONTAL);
   algolabel->Add(new wxStaticText(this, wxID_STATIC, _("Algorithm:")), 0, FIX_ALIGN_BUG);
   hbox0->Add(algolabel, 0, wxALIGN_CENTER_VERTICAL, 0);
   hbox0->Add(algochoice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
   hbox0->AddSpacer(HGAP);
   wxBoxSizer* helpbox = new wxBoxSizer(wxHORIZONTAL);
   helpbox->Add(helpbutt, 0, FIX_ALIGN_BUG);
   hbox0->Add(helpbox, 0, wxALIGN_CENTER_VERTICAL, 0);
   
   int minwidth = 250;
   int algowd = hbox0->GetMinSize().GetWidth();
   if (algowd > minwidth) minwidth = algowd;
   
   // create text box for entering new rule
   ruletext = new wxTextCtrl(this, RULE_TEXT,
                             wxString(currlayer->algo->getrule(),wxConvLocal),
                             wxDefaultPosition, wxSize(minwidth,wxDefaultCoord));
   
   // create a choice menu to select named rule
   wxArrayString namearray;
   for (size_t i=0; i<namedrules.GetCount(); i++) {
      // remove "|..." part
      wxString name = namedrules[i].BeforeFirst('|');
      namearray.Add(name);
   }
   namechoice = new wxChoice(this, RULE_NAME,
                             wxDefaultPosition, wxSize(160,wxDefaultCoord), namearray);
   nameindex = -1;
   UpdateName();        // careful -- this uses ruletext

   addtext = new wxTextCtrl(this, RULE_ADD_TEXT, wxEmptyString,
                            wxDefaultPosition, wxSize(160,wxDefaultCoord));

   wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
   hbox1->Add(namechoice, 0, wxALIGN_CENTER_VERTICAL, 0);
   hbox1->AddSpacer(HGAP);
   wxBoxSizer* delbox = new wxBoxSizer(wxHORIZONTAL);
   delbox->Add(delbutt, 0, FIX_ALIGN_BUG);
   hbox1->Add(delbox, 0, wxALIGN_CENTER_VERTICAL, 0);

   wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
   hbox2->Add(addtext, 0, wxALIGN_CENTER_VERTICAL, 0);
   hbox2->AddSpacer(HGAP);
   hbox2->Add(addbutt, 0, wxALIGN_CENTER_VERTICAL, 0);

   wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);
   wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
   // can we avoid these fudges???
   #ifdef __WXMAC__
      minwidth += 24;
   #elif defined(__WXMSW__)
      minwidth += 16;
   #else
      minwidth += 12;
   #endif
   vbox->Add(minwidth, 0, 0);
   vbox->Add(stdbutts, 1, wxALIGN_RIGHT, 0);

   wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(hbox0, 0, wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(textlabel, 0, wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(10);
   topSizer->Add(ruletext, 0, wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(namelabel, 0, wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(6);
   topSizer->Add(hbox1, 0, wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(hbox2, 0, wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(BIGVGAP);
   topSizer->Add(vbox, 0, wxTOP | wxBOTTOM, 10);
   SetSizer(topSizer);
   topSizer->SetSizeHints(this);    // calls Fit
}

// -----------------------------------------------------------------------------

void RuleDialog::UpdateAlgo()
{
   // may need to change algochoice depending on current rule text
   wxString newrule = ruletext->GetValue();
   if (newrule.IsEmpty()) newrule = wxT("B3/S23");
   
   lifealgo* tempalgo;
   const char* err;
   
   if (algoindex < NumAlgos()) {
      // try new rule in currently selected algo
      tempalgo = CreateNewUniverse(algoindex);
      err = tempalgo->setrule( newrule.mb_str(wxConvLocal) );
      delete tempalgo;
      if (!err) return;
   }
   
   // try new rule in all algos
   int newindex;
   for (newindex = 0; newindex < NumAlgos(); newindex++) {
      tempalgo = CreateNewUniverse(newindex);
      err = tempalgo->setrule( newrule.mb_str(wxConvLocal) );
      delete tempalgo;
      if (!err) {
         if (newindex != algoindex) {
            if (algoindex >= NumAlgos()) {
               // remove UNKNOWN item from end of algochoice
               algochoice->Delete( algochoice->GetCount() - 1 );
            }
            algoindex = newindex;
            algochoice->SetSelection(algoindex);
            UpdateHelp();
         }
         return;
      }
   }

   // new rule is not valid in any algo
   if (algoindex < NumAlgos()) {
      // append UNKNOWN item and select it
      algochoice->Append(UNKNOWN);
      algoindex = NumAlgos();
      algochoice->SetSelection(algoindex);
      UpdateHelp();
   }
}

// -----------------------------------------------------------------------------

void RuleDialog::UpdateHelp()
{
   if (expanded) {
      if (algoindex < NumAlgos()) {
         // display Help/Algorithms/algoname.html
         wxString filepath = gollydir + wxT("Help");
         filepath += wxFILE_SEP_PATH;
         filepath += wxT("Algorithms");
         filepath += wxFILE_SEP_PATH;
         filepath += wxString(GetAlgoName(algoindex),wxConvLocal);
         filepath += wxT(".html");
         htmlwin->DisplayFile(filepath);
      } else {
         // UNKNOWN algo
         htmlwin->DisplayFile(wxEmptyString);
      }
   }
}

// -----------------------------------------------------------------------------

void RuleDialog::UpdateName()
{
   // may need to change named rule depending on current rule text
   int newindex;
   wxString newrule = ruletext->GetValue();
   if ( newrule.IsEmpty() ) {
      // empty string is a quick way to restore normal Life
      newindex = 0;
   } else {
      // search namedrules array for matching rule
      size_t i;
      newindex = -1;
      for (i=0; i<namedrules.GetCount(); i++) {
         // extract rule after '|'
         wxString thisrule = namedrules[i].AfterFirst('|');
         if ( MatchingRules(newrule, thisrule) ) {
            newindex = i;
            break;
         }
      }
   }
   if (newindex >= 0) {
      // matching rule found so remove UNNAMED item if it exists
      // use (int) twice to avoid warnings on wx 2.6.x/2.7
      if ( (int) namechoice->GetCount() > (int) namedrules.GetCount() ) {
         namechoice->Delete( namechoice->GetCount() - 1 );
      }
   } else {
      // no match found so use index of UNNAMED item,
      // appending it if it doesn't exist;
      // use (int) twice to avoid warnings on wx 2.6.x/2.7
      if ( (int) namechoice->GetCount() == (int) namedrules.GetCount() ) {
         namechoice->Append(UNNAMED);
      }
      newindex = namechoice->GetCount() - 1;
   }
   if (nameindex != newindex) {
      nameindex = newindex;
      namechoice->SetSelection(nameindex);
   }
}

// -----------------------------------------------------------------------------

void RuleDialog::OnRuleTextChanged(wxCommandEvent& WXUNUSED(event))
{
   if (ignore_text_change) return;
   UpdateName();
   UpdateAlgo();
}

// -----------------------------------------------------------------------------

void RuleDialog::OnChooseAlgo(wxCommandEvent& event)
{
   int i = event.GetSelection();
   if (i >= 0 && i < NumAlgos() && i != algoindex) {
      int oldindex = algoindex;
      algoindex = i;
      
      // check if the current rule is valid in newly selected algo
      wxString thisrule = ruletext->GetValue();
      if (thisrule.IsEmpty()) thisrule = wxT("B3/S23");
      lifealgo* tempalgo = CreateNewUniverse(algoindex);
      const char* err = tempalgo->setrule( thisrule.mb_str(wxConvLocal) );
      if (err) {
         // rule is not valid so change rule text to selected algo's default rule
         ignore_text_change = true;
         ruletext->SetValue( wxString(tempalgo->DefaultRule(),wxConvLocal) );
         ruletext->SetFocus();
         ruletext->SetSelection(-1,-1);
         ignore_text_change = false;
         if (oldindex >= NumAlgos()) {
            // remove UNKNOWN item from end of algochoice
            algochoice->Delete( algochoice->GetCount() - 1 );
         }
         UpdateName();
      } else {
         // rule is valid
         if (oldindex >= NumAlgos()) {
            Warning(_("Bug detected in OnChooseAlgo!"));
         }
      }
      delete tempalgo;
      
      UpdateHelp();
   }
}

// -----------------------------------------------------------------------------

void RuleDialog::OnChooseName(wxCommandEvent& event)
{
   int i = event.GetSelection();
   if (i == nameindex) return;
   
   // update rule text based on chosen name
   nameindex = i;
   if ( nameindex == (int) namedrules.GetCount() ) {
      Warning(_("Bug detected in OnChooseName!"));
      UpdateAlgo();
      return;
   }
   
   // remove UNNAMED item if it exists;
   // use (int) twice to avoid warnings in wx 2.6.x/2.7
   if ( (int) namechoice->GetCount() > (int) namedrules.GetCount() ) {
      namechoice->Delete( namechoice->GetCount() - 1 );
   }

   wxString rule = namedrules[nameindex].AfterFirst('|');
   ignore_text_change = true;
   ruletext->SetValue(rule);
   ruletext->SetFocus();
   ruletext->SetSelection(-1,-1);
   ignore_text_change = false;

   UpdateAlgo();
}

// -----------------------------------------------------------------------------

void RuleDialog::OnAddName(wxCommandEvent& WXUNUSED(event))
{
   if ( nameindex < (int) namedrules.GetCount() ) {
      // OnUpdateAdd should prevent this but play safe
      wxBell();
      return;
   }
   
   // validate new rule and convert to canonical form
   wxString newrule = ruletext->GetValue();
   if (!ValidRule(newrule)) {
      Warning(_("The new rule is not valid in any algorithm."));
      ruletext->SetFocus();
      ruletext->SetSelection(-1,-1);
      return;
   }
      
   // validate new name
   wxString newname = addtext->GetValue();
   if ( newname.IsEmpty() ) {
      Warning(_("Type in a name for the new rule."));
      addtext->SetFocus();
      return;
   } else if ( newname.Find('|') >= 0 ) {
      Warning(_("Sorry, but rule names must not contain \"|\"."));
      addtext->SetFocus();
      addtext->SetSelection(-1,-1);
      return;
   } else if ( newname == UNNAMED ) {
      Warning(_("You can't use that name smarty pants."));
      addtext->SetFocus();
      addtext->SetSelection(-1,-1);
      return;
   } else if ( namechoice->FindString(newname) != wxNOT_FOUND ) {
      Warning(_("That name is already used for another rule."));
      addtext->SetFocus();
      addtext->SetSelection(-1,-1);
      return;
   }
   
   // replace UNNAMED with new name
   namechoice->Delete( namechoice->GetCount() - 1 );
   namechoice->Append( newname );
   
   // append new name and rule to namedrules
   newname += '|';
   newname += newrule;
   namedrules.Add(newname);
   
   // force a change to newly appended item
   nameindex = -1;
   UpdateName();
}

// -----------------------------------------------------------------------------

void RuleDialog::OnDeleteName(wxCommandEvent& WXUNUSED(event))
{
   if ( nameindex <= 0 || nameindex >= (int) namedrules.GetCount() ) {
      // OnUpdateDelete should prevent this but play safe
      wxBell();
      return;
   }
   
   // remove current name
   namechoice->Delete(nameindex);
   namedrules.RemoveAt((size_t) nameindex);
   
   // force a change to UNNAMED item
   nameindex = -1;
   UpdateName();
}

// -----------------------------------------------------------------------------

void RuleDialog::OnUpdateAdd(wxUpdateUIEvent& event)
{
   // Add button is only enabled if UNNAMED item is selected
   event.Enable( nameindex == (int) namedrules.GetCount() );
}

// -----------------------------------------------------------------------------

void RuleDialog::OnUpdateDelete(wxUpdateUIEvent& event)
{
   // Delete button is only enabled if a non-Life named rule is selected
   event.Enable( nameindex > 0 && nameindex < (int) namedrules.GetCount() );
}

// -----------------------------------------------------------------------------

void RuleDialog::OnHelpButton(wxCommandEvent& WXUNUSED(event))
{
   wxRect r = GetRect();
   expanded = !expanded;
   if (expanded) {
      int wd, ht;
      GetClientSize(&wd, &ht);
      wxRect htmlrect(minrect.width, 10, ruleexwd - 10, ht + ruleexht - 20);
      htmlwin->SetSize(htmlrect);
      r.width = minrect.width + ruleexwd;
      r.height = minrect.height + ruleexht;
      // call SetMinSize below (AFTER SetSize call)
      SetMaxSize( wxSize(-1,-1) );
   } else {
      wxWindow* focus = FindFocus();
      if (focus != ruletext && focus != addtext) {
         ruletext->SetFocus();
      }
      r.width = minrect.width;
      r.height = minrect.height;
      SetMinSize( wxSize(minrect.width,minrect.height) );
      SetMaxSize( wxSize(minrect.width,minrect.height) );
   }
   
   UpdateHelp();
   htmlwin->Show(expanded);
   SetSize(r);
   
   if (expanded) {
      SetMinSize( wxSize(minrect.width+100,minrect.height) );
   }
}

// -----------------------------------------------------------------------------

void RuleDialog::OnSize(wxSizeEvent& event)
{
   if (expanded) {
      // resize html window
      int wd, ht;
      GetClientSize(&wd, &ht);
      wxRect r = GetRect();
      ruleexwd = r.width - minrect.width;
      ruleexht = r.height - minrect.height;
      wxRect htmlrect(minrect.width, 10, wd - minrect.width - 10, ht - 20);
      htmlwin->SetSize(htmlrect);
   }
   event.Skip();
}

// -----------------------------------------------------------------------------

void RuleDialog::OnMove(wxMoveEvent& event)
{
   // save current location for later use in SavePrefs
   /*  sigh... wxMac doesn't return correct position
   wxPoint pt = event.GetPosition();
   rulex = pt.x;
   ruley = pt.y;
   */
   wxRect r = GetRect();
   rulex = r.x;
   ruley = r.y;
   
   event.Skip();
}

// -----------------------------------------------------------------------------

bool RuleDialog::TransferDataFromWindow()
{
   // get and validate new rule
   wxString newrule = ruletext->GetValue();
   if (newrule.IsEmpty()) newrule = wxT("B3/S23");
   
   if (algoindex >= NumAlgos()) {
      Warning(_("The new rule is not valid in any algorithm."));
      ruletext->SetFocus();
      ruletext->SetSelection(-1,-1);
      return false;
   }
   
   if (algoindex == currlayer->algtype) {
      // new rule should be valid in current algorithm
      const char* err = currlayer->algo->setrule( newrule.mb_str(wxConvLocal) );
      if (!err) {
         // cell colors depend on current algo and rule
         UpdateCellColors();
         return true;
      }
      Warning(_("Bug detected in TransferDataFromWindow!"));
      return false;
   } else {
      // change the current algorithm and switch to the new rule
      mainptr->ChangeAlgorithm(algoindex, newrule);
      return true;
   }
}

// -----------------------------------------------------------------------------

bool ChangeRule()
{
   wxArrayString oldnames = namedrules;
   wxString oldrule = wxString(currlayer->algo->getrule(),wxConvLocal);

   RuleDialog dialog( wxGetApp().GetTopWindow() );
   if ( dialog.ShowModal() == wxID_OK ) {
      // TransferDataFromWindow has changed the current rule,
      // and possibly the current algorithm as well
      return true;
   } else {
      // user hit Cancel so restore rule and name array
      currlayer->algo->setrule( oldrule.mb_str(wxConvLocal) );
      namedrules.Clear();
      namedrules = oldnames;
      return false;
   }
}

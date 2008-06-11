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
#ifndef _WXUNDO_H_
#define _WXUNDO_H_

#include "bigint.h"     // for bigint class
#include "wxedit.h"     // for Selection class
#include "wxalgos.h"    // for algo_type

// This module implements unlimited undo/redo:

typedef struct {
   int x;               // cell's x position
   int y;               // cell's y position
   int oldstate;        // old state
   int newstate;        // new state
} cell_change;          // stores a single cell change

class UndoRedo {
public:
   UndoRedo();
   ~UndoRedo();
   
   void SaveCellChange(int x, int y, int oldstate, int newstate);
   // cell at x,y has changed state
   
   void ForgetCellChanges();
   // ignore cell changes made by any previous SaveCellChange calls
   
   bool RememberCellChanges(const wxString& action, bool olddirty);
   // remember cell changes made by any previous SaveCellChange calls,
   // and the state of the layer's dirty flag BEFORE the change;
   // the given action string will be appended to the Undo/Redo items;
   // return true if one or more cells changed state, false otherwise
   
   void RememberFlip(bool topbot, bool olddirty);
   // remember flip's direction

   void RememberRotation(bool clockwise, bool olddirty);
   // remember simple rotation (selection includes entire pattern)
   
   void RememberRotation(bool clockwise, Selection& oldsel, Selection& newsel,
                         bool olddirty);
   // remember rotation's direction and old and new selections;
   // this variant assumes SaveCellChange may have been called
   
   void RememberSelection(const wxString& action);
   // remember selection change (no-op if selection hasn't changed)

   void RememberGenStart();
   // remember info before generating the current pattern

   void RememberGenFinish();
   // remember generating change after pattern has finished generating
   
   void AddGenChange();
   // in some situations the undo list is empty but ResetPattern can still
   // be called because the gen count is > startgen, so this routine adds
   // a generating change to the undo list so the user can Undo or Reset
   // (and then Redo if they wish)

   void SyncUndoHistory();
   // called by ResetPattern to synchronize the undo history

   void RememberSetGen(bigint& oldgen, bigint& newgen,
                       bigint& oldstartgen, bool oldsave);
   // remember change of generation count
   
   void RememberNameChange(const wxString& oldname, const wxString& oldcurrfile,
                           bool oldsave, bool olddirty);
   // remember change to current layer's name
   
   void DeletingClone(int index);
   // the given cloned layer is about to be deleted, so we must ignore
   // any later name changes involving this layer
   
   void RememberRuleChange(const wxString& oldrule);
   // remember rule change
   
   void RememberAlgoChange(algo_type oldalgo);
   // remember algorithm change
   
   void RememberScriptStart();
   // remember that script is about to start; this allows us to undo/redo
   // any changes made by the script all at once

   void RememberScriptFinish();
   // remember that script has ended
   
   void Duplicate(UndoRedo* history, const wxString& tempstart);
   // duplicate given undo/redo history
   
   bool savecellchanges;         // script's cell changes need to be remembered?
   bool savegenchanges;          // script's gen changes need to be remembered?
   bool doingscriptchanges;      // are script's changes being undone/redone?

   bool CanUndo();               // can a change be undone?
   bool CanRedo();               // can an undone change be redone?
   
   void UndoChange();            // undo a change
   void RedoChange();            // redo an undone change

   void UpdateUndoRedoItems();   // update Undo/Redo items in Edit menu
   void ClearUndoRedo();         // clear all undo/redo history

private:
   wxList undolist;              // list of undoable changes
   wxList redolist;              // list of redoable changes

   cell_change* cellarray;       // dynamic array of cell changes
   unsigned int numchanges;      // number of cell changes
   unsigned int maxchanges;      // number allocated
   bool badalloc;                // malloc/realloc failed?
   
   wxString prevfile;            // for saving pattern at start of gen change
   bigint prevgen;               // generation count at start of gen change
   bigint prevx, prevy;          // viewport position at start of gen change
   int prevmag;                  // scale at start of gen change
   int prevwarp;                 // speed at start of gen change
   Selection prevsel;            // selection at start of gen change
   int startcount;               // unfinished RememberGenStart calls
   bool fixsetgen;               // setgen node needs to be updated?
   
   void SaveCurrentPattern(const wxString& tempfile);
   // save current pattern to given temporary file
   
   void UpdateUndoItem(const wxString& action);
   void UpdateRedoItem(const wxString& action);
   // update the Undo/Redo items in the Edit menu
};

#endif

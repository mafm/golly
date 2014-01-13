/*** /

 This file is part of Golly, a Game of Life Simulator.
 Copyright (C) 2013 Andrew Trevorrow and Tomas Rokicki.

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

#ifndef _WEBCALLS_H_
#define _WEBCALLS_H_

#include <string>   // for std::string

// Web-specific routines (called mainly from gui-common code):

extern bool refresh_pattern;    // DoFrame in main.cpp should call DrawPattern?

void UpdatePattern();
// Redraw the current pattern (actually just sets refresh_pattern to true).

void UpdateStatus();
// Redraw the status bar info.

void PauseGenerating();
// If pattern is generating then temporarily pause.

void ResumeGenerating();
// Resume generating pattern if it was paused.

std::string GetRuleName(const std::string& rule);
// Return name of given rule (empty string if rule is unnamed).

void UpdateEditBar();
// Update Undo and Redo buttons, show current drawing state and touch mode.

void BeginProgress(const char* title);
bool AbortProgress(double fraction_done, const char* message);
void EndProgress();
// These calls display a progress bar while a lengthy task is carried out.

void SwitchToPatternTab();
// Switch to main screen for displaying/editing/generating patterns.

void ShowTextFile(const char* filepath);
// Display contents of given text file in a modal view.

void ShowHelp(const char* filepath);
// Display given HTML file in Help screen.

void WebWarning(const char* msg);
// Beep and display message in a modal dialog.

bool WebYesNo(const char* msg);
// Similar to Warning, but there are 2 buttons: Yes and No.
// Returns true if Yes button is hit.

void WebFatal(const char* msg);
// Beep, display message in a modal dialog, then exit app.

void WebBeep();
// Play beep sound, depending on user setting.

void WebRemoveFile(const std::string& filepath);
// Delete given file.

bool WebMoveFile(const std::string& inpath, const std::string& outpath);
// Return true if input file is successfully moved to output file.
// If the output file existed it is replaced.

void WebFixURLPath(std::string& path);
// Replace "%..." with suitable chars for a file path (eg. %20 is changed to space).

void WebCheckEvents();
// Run main UI thread for a short time so app remains responsive while doing a
// lengthy computation.  Note that event_checker is > 0 while in this routine.

bool WebCopyTextToClipboard(const char* text);
// Copy given text to the clipboard.

bool WebGetTextFromClipboard(std::string& text);
// Get text from the clipboard.

bool WebDownloadFile(const std::string& url, const std::string& filepath);
// Download given url and create given file.

#endif
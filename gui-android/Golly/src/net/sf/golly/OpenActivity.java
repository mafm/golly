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

package net.sf.golly;

import java.io.File;
import java.util.Arrays;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.NavUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class OpenActivity extends Activity {

    // see jnicalls.cpp for these native routines:
    private native String nativeGetRecentPatterns();
    private native String nativeGetSavedPatterns(String paths);
    private native String nativeGetDownloadedPatterns(String paths);
    private native String nativeGetSuppliedPatterns(String paths);
    private native void nativeToggleDir(String path);
    
    private enum PATTERNS {
        SUPPLIED, RECENT, SAVED, DOWNLOADED;
    }
    
    private static PATTERNS currpatterns = PATTERNS.SUPPLIED;
    
    // remember scroll positions for each type of patterns
    private static int supplied_pos = 0;
    private static int recent_pos = 0;
    private static int saved_pos = 0;
    private static int downloaded_pos = 0;
    
    private WebView gwebview;   // for displaying html data
    
    // -----------------------------------------------------------------------------
    
    // this class lets us intercept link taps and restore the scroll position
    private class MyWebViewClient extends WebViewClient {
        @Override
        public boolean shouldOverrideUrlLoading(WebView webview, String url) {
            if (url.startsWith("open:")) {
                openFile(url.substring(5));
                return true;
            }
            if (url.startsWith("toggledir:")) {
                nativeToggleDir(url.substring(10));
                saveScrollPosition();
                showSuppliedPatterns();
                return true;
            }
            if (url.startsWith("delete:")) {
                removeFile(url.substring(7));
                return true;
            }
            if (url.startsWith("edit:")) {
                editFile(url.substring(5));
                return true;
            }
            return false;
        }
        
        @Override  
        public void onPageFinished(WebView webview, String url) {
            super.onPageFinished(webview, url);
            // webview.scrollTo doesn't always work here;
            // we need to delay until webview.getContentHeight() > 0
            final int scrollpos = restoreScrollPosition();
            final Handler handler = new Handler();
            Runnable runnable = new Runnable() {
                public void run() {
                    if (gwebview.getContentHeight() > 0) {
                        gwebview.scrollTo(0, scrollpos);
                    } else {
                        // try again a bit later
                        handler.postDelayed(this, 100);
                    }
                }
            };
            handler.postDelayed(runnable, 100);
        }  
    }

    // -----------------------------------------------------------------------------

    @Override
    protected void onPause() {
        super.onPause();
        saveScrollPosition();
    }
    
    // -----------------------------------------------------------------------------
    
    private void saveScrollPosition() {
        switch (currpatterns) {
            case SUPPLIED:   supplied_pos = gwebview.getScrollY(); break;
            case RECENT:     recent_pos = gwebview.getScrollY(); break;
            case SAVED:      saved_pos = gwebview.getScrollY(); break;
            case DOWNLOADED: downloaded_pos = gwebview.getScrollY(); break;
        }
    }
    
    // -----------------------------------------------------------------------------
    
    private int restoreScrollPosition() {
        switch (currpatterns) {
            case SUPPLIED:   return supplied_pos;
            case RECENT:     return recent_pos;
            case SAVED:      return saved_pos;
            case DOWNLOADED: return downloaded_pos;
        }
        return 0;   // should never get here
    }

    // -----------------------------------------------------------------------------
    
    public final static String OPENFILE_MESSAGE = "net.sf.golly.OPENFILE";
    
    private void openFile(String filepath) {
        // switch to main screen and open given file
        Intent intent = new Intent(this, MainActivity.class);
        intent.putExtra(OPENFILE_MESSAGE, filepath);
        startActivity(intent);
    }
    
    // -----------------------------------------------------------------------------
    
    private void removeFile(String filepath) {
        final String fullpath = getFilesDir().getAbsolutePath() + "/" + filepath;
        final File file = new File(fullpath);
        
        // ask user if it's okay to delete given file
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle("Delete file?");
        alert.setMessage("Do you really want to delete " + file.getName() + "?");
        alert.setPositiveButton("DELETE",
            new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    if (file.delete()) {
                        // file has been deleted so refresh gwebview
                        saveScrollPosition();
                        switch (currpatterns) {
                            case SUPPLIED:   break; // should never happen
                            case RECENT:     break; // should never happen
                            case SAVED:      showSavedPatterns(); break;
                            case DOWNLOADED: showDownloadedPatterns(); break;
                        }
                    } else {
                        // should never happen
                        Log.e("removeFile", "Failed to delete file: " + fullpath);
                    }
                }
            });
        alert.setNegativeButton("CANCEL", null);
        alert.show();
    }
    
    // -----------------------------------------------------------------------------
    
    private void editFile(String filepath) {
        // let user read/edit given file
        //!!! start a TextActivity, passing filepath and read/edit mode???
    }
   
    // -----------------------------------------------------------------------------

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.open_layout);
        
        gwebview = (WebView) findViewById(R.id.webview);
        // no need for JavaScript???
        // gwebview.getSettings().setJavaScriptEnabled(true);
        gwebview.setWebViewClient(new MyWebViewClient());

        // show the Up button in the action bar
        getActionBar().setDisplayHomeAsUpEnabled(true);
        
        switch (currpatterns) {
            case SUPPLIED:   showSuppliedPatterns(); break;
            case RECENT:     showRecentPatterns(); break;
            case SAVED:      showSavedPatterns(); break;
            case DOWNLOADED: showDownloadedPatterns(); break;
        }
    }

    // -----------------------------------------------------------------------------

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // add main.xml items to the action bar
        getMenuInflater().inflate(R.menu.main, menu);
        
        // disable the item for this activity
        MenuItem item = menu.findItem(R.id.open);
        item.setEnabled(false);
        
        return true;
    }

    // -----------------------------------------------------------------------------

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // action bar item has been tapped
        Intent intent;
        switch (item.getItemId()) {
            case android.R.id.home:
                // the Home or Up button will start MainActivity
                NavUtils.navigateUpFromSameTask(this);
                return true;
            case R.id.open:
                // do nothing
                break;
            case R.id.settings:
                intent = new Intent(this, SettingsActivity.class);
                startActivity(intent);
                return true;
            case R.id.help:
                intent = new Intent(this, HelpActivity.class);
                startActivity(intent);
                return true;
        }
        return super.onOptionsItemSelected(item);
    }
    
    // -----------------------------------------------------------------------------
    
    // called when the Supplied button is tapped
    public void doSupplied(View view) {
        if (currpatterns != PATTERNS.SUPPLIED) {
            saveScrollPosition();
            currpatterns = PATTERNS.SUPPLIED;
            showSuppliedPatterns();
        }
    }

    // -----------------------------------------------------------------------------
    
    // called when the Recent button is tapped
    public void doRecent(View view) {
        if (currpatterns != PATTERNS.RECENT) {
            saveScrollPosition();
            currpatterns = PATTERNS.RECENT;
            showRecentPatterns();
        }
    }

    // -----------------------------------------------------------------------------
    
    // called when the Saved button is tapped
    public void doSaved(View view) {
        if (currpatterns != PATTERNS.SAVED) {
            saveScrollPosition();
            currpatterns = PATTERNS.SAVED;
            showSavedPatterns();
        }
    }

    // -----------------------------------------------------------------------------
    
    // called when the Downloaded button is tapped
    public void doDownloaded(View view) {
        if (currpatterns != PATTERNS.DOWNLOADED) {
            saveScrollPosition();
            currpatterns = PATTERNS.DOWNLOADED;
            showDownloadedPatterns();
        }
    }

    // -----------------------------------------------------------------------------
    
    private String enumerateDirectory(File dir, String prefix) {
        // return the files and/or sub-directories in the given directory
        // as a string of paths where:
        // - paths are relative to to the initial directory
        // - directory paths end with '/'
        // - each path is terminated by \n
        String result = "";
        File[] files = dir.listFiles();
        if (files != null) {
            Arrays.sort(files);         // sort into alphabetical order
            for (File file : files) {
                if (file.isDirectory()) {
                    String dirname = prefix + file.getName() + "/";
                    result += dirname + "\n";
                    result += enumerateDirectory(file, dirname);
                } else {
                    result += prefix + file.getName() + "\n";
                }
            }
        }
        return result;
    }

    // -----------------------------------------------------------------------------
    
    private void showSuppliedPatterns() {
        String paths = enumerateDirectory(new File(getFilesDir(), "Supplied/Patterns"), "");
        String htmldata = nativeGetSuppliedPatterns(paths);
        // we use a special base URL here so that <img src="foo.png"/> can find
        // foo.png stored in the assets folder
        gwebview.loadDataWithBaseURL("file:///android_asset/", htmldata, "text/html", "utf-8", null);
    }

    // -----------------------------------------------------------------------------
    
    private void showRecentPatterns() {
        String htmldata = nativeGetRecentPatterns();
        gwebview.loadDataWithBaseURL(null, htmldata, "text/html", "utf-8", null);
    }

    // -----------------------------------------------------------------------------
    
    private void showSavedPatterns() {
        String paths = enumerateDirectory(new File(getFilesDir(), "Saved"), "");
        String htmldata = nativeGetSavedPatterns(paths);
        gwebview.loadDataWithBaseURL(null, htmldata, "text/html", "utf-8", null);
    }

    // -----------------------------------------------------------------------------
    
    private void showDownloadedPatterns() {
        String paths = enumerateDirectory(new File(getFilesDir(), "Downloads"), "");
        String htmldata = nativeGetDownloadedPatterns(paths);
        gwebview.loadDataWithBaseURL(null, htmldata, "text/html", "utf-8", null);
    }

} // OpenActivity class

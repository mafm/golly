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

#include <jni.h>
#include <GLES/gl.h>
#include <unistd.h>     // for usleep

#include "utils.h"      // for Warning, etc
#include "algos.h"      // for InitAlgorithms
#include "prefs.h"      // for GetPrefs, SavePrefs
#include "layer.h"      // for AddLayer, ResizeLayers, currlayer
#include "control.h"    // for SetMinimumStepExponent
#include "file.h"       // for NewPattern
#include "render.h"     // for DrawPattern
#include "view.h"       // for TouchBegan, etc
#include "status.h"     // for UpdateStatusLines, ClearMessage, etc
#include "undo.h"       // for ClearUndoRedo
#include "jnicalls.h"

// -----------------------------------------------------------------------------

// these globals cache info that doesn't change during execution
static JavaVM* javavm;
static jobject mainobj;
static jmethodID id_RefreshPattern;
static jmethodID id_ShowStatusLines;
static jmethodID id_UpdateEditBar;
static jmethodID id_CheckMessageQueue;
static jmethodID id_PlayBeepSound;
static jmethodID id_Warning;
static jmethodID id_Fatal;
static jmethodID id_YesNo;
static jmethodID id_RemoveFile;
static jmethodID id_MoveFile;
static jmethodID id_CopyTextToClipboard;
static jmethodID id_GetTextFromClipboard;

static bool rendering = false;        // in DrawPattern?
static bool paused = false;           // generating has been paused?

// -----------------------------------------------------------------------------

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env;
    javavm = vm;    // save in global

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed!");
        return -1;
    }

    return JNI_VERSION_1_6;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeClassInit(JNIEnv* env, jclass klass)
{
    // save IDs for Java methods in MainActivity
    id_RefreshPattern = env->GetMethodID(klass, "RefreshPattern", "()V");
    id_ShowStatusLines = env->GetMethodID(klass, "ShowStatusLines", "()V");
    id_UpdateEditBar = env->GetMethodID(klass, "UpdateEditBar", "()V");
    id_CheckMessageQueue = env->GetMethodID(klass, "CheckMessageQueue", "()V");
    id_PlayBeepSound = env->GetMethodID(klass, "PlayBeepSound", "()V");
    id_Warning = env->GetMethodID(klass, "Warning", "(Ljava/lang/String;)V");
    id_Fatal = env->GetMethodID(klass, "Fatal", "(Ljava/lang/String;)V");
    id_YesNo = env->GetMethodID(klass, "YesNo", "(Ljava/lang/String;)Ljava/lang/String;");
    id_RemoveFile = env->GetMethodID(klass, "RemoveFile", "(Ljava/lang/String;)V");
    id_MoveFile = env->GetMethodID(klass, "MoveFile", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    id_CopyTextToClipboard = env->GetMethodID(klass, "CopyTextToClipboard", "(Ljava/lang/String;)V");
    id_GetTextFromClipboard = env->GetMethodID(klass, "GetTextFromClipboard", "()Ljava/lang/String;");
}

// -----------------------------------------------------------------------------

static JNIEnv* getJNIenv(bool* attached)
{
    // get the JNI environment for the current thread
    JNIEnv* env;
    *attached = false;

    int result = javavm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        if (javavm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("AttachCurrentThread failed!");
            return NULL;
        }
        *attached = true;
    } else if (result == JNI_EVERSION) {
        LOGE("GetEnv: JNI_VERSION_1_6 is not supported!");
        return NULL;
    }

    return env;
}

// -----------------------------------------------------------------------------

static std::string ConvertJString(JNIEnv* env, jstring str)
{
   const jsize len = env->GetStringUTFLength(str);
   const char* strChars = env->GetStringUTFChars(str, 0);
   std::string result(strChars, len);
   env->ReleaseStringUTFChars(str, strChars);
   return result;
}

// -----------------------------------------------------------------------------

static void CheckIfRendering()
{
    int msecs = 0;
    while (rendering) {
        // wait for DrawPattern to finish
        usleep(1000);    // 1 millisec
        msecs++;
    }
    // if (msecs > 0) LOGI("waited for rendering: %d msecs", msecs);
}

// -----------------------------------------------------------------------------

void UpdatePattern()
{
    // call a Java method that calls GLSurfaceView.requestRender() which calls nativeRender
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) env->CallVoidMethod(mainobj, id_RefreshPattern);
    if (attached) javavm->DetachCurrentThread();
}

// -----------------------------------------------------------------------------

void UpdateStatus()
{
    UpdateStatusLines();    // sets status1, status2, status3

    // call Java method in MainActivity class to update the status bar info
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) env->CallVoidMethod(mainobj, id_ShowStatusLines);
    if (attached) javavm->DetachCurrentThread();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT jstring JNICALL Java_net_sf_golly_MainActivity_nativeGetStatusLine(JNIEnv* env, jobject obj, jint line)
{
    switch (line) {
        case 1: return env->NewStringUTF(status1.c_str());
        case 2: return env->NewStringUTF(status2.c_str());
        case 3: return env->NewStringUTF(status3.c_str());
    }
    return env->NewStringUTF("Fix bug in nativeGetStatusLine!");
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT jstring JNICALL Java_net_sf_golly_MainActivity_nativeGetPasteMode(JNIEnv* env)
{
    return env->NewStringUTF(GetPasteMode());
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT jstring JNICALL Java_net_sf_golly_MainActivity_nativeGetRandomFill(JNIEnv* env)
{
    char s[4];    // room for 0..100
    sprintf(s, "%d", randomfill);
    return env->NewStringUTF(s);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeCreate(JNIEnv* env, jobject obj)
{
    // LOGI("nativeCreate");

    // save obj for calling Java methods in this instance of MainActivity
    mainobj = env->NewGlobalRef(obj);

    static bool firstcall = true;
    if (firstcall) {
        firstcall = false;
        std::string msg = "This is Golly ";
        msg += GOLLY_VERSION;
        msg += " for Android.  Copyright 2013 The Golly Gang.";
        SetMessage(msg.c_str());
        MAX_MAG = 5;                // maximum cell size = 32x32
        InitAlgorithms();           // must initialize algoinfo first
        GetPrefs();                 // load user's preferences
        SetMinimumStepExponent();   // for slowest speed
        AddLayer();                 // create initial layer (sets currlayer)
        NewPattern();               // create new, empty universe
        UpdateStatus();             // show initial message
        showtiming = true; //!!! test
        showicons = true; //!!! test
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeDestroy(JNIEnv* env)
{
    // LOGI("nativeDestroy");

/* Android apps aren't really meant to quit so best not to do this stuff:

    // avoid nativeResume restarting pattern generation
    paused = false;
    generating = false;

    if (currlayer->dirty) {
        // ask if changes should be saved
        //!!!
    }

    SavePrefs();

    // clear all undo/redo history for this layer
    currlayer->undoredo->ClearUndoRedo();

    if (numlayers > 1) DeleteOtherLayers();
    delete currlayer;

    // reset some globals so we can call AddLayer again in nativeCreate
    numlayers = 0;
    numclones = 0;
    currlayer = NULL;

    // delete stuff created by InitAlgorithms()
    DeleteAlgorithms();

    // reset some static info so we can call InitAlgorithms again
    staticAlgoInfo::nextAlgoId = 0;
    staticAlgoInfo::head = NULL;

*/

    // the current instance of MainActivity is being destroyed
    env->DeleteGlobalRef(mainobj);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeSetGollyDir(JNIEnv* env, jobject obj, jstring path)
{
    gollydir = ConvertJString(env, path) + "/";
    // LOGI("gollydir = %s", gollydir.c_str());
    // gollydir = /data/data/pkg.name/files/

    // set paths to various subdirs (created by caller)
    userrules = gollydir + "Rules/";
    datadir = gollydir + "Saved/";
    downloaddir = gollydir + "Downloads/";
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeSetTempDir(JNIEnv* env, jobject obj, jstring path)
{
    tempdir = ConvertJString(env, path) + "/";
    // LOGI("tempdir = %s", tempdir.c_str());
    // tempdir = /data/data/pkg.name/cache/
    clipfile = tempdir + "golly_clipboard";
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeSetSuppliedDirs(JNIEnv* env, jobject obj, jstring path)
{
    std::string prefix = ConvertJString(env, path);
    // LOGI("prefix = %s", prefix.c_str());
    // prefix = /data/data/pkg.name/app_
    helpdir = prefix + "Help/";
    rulesdir = prefix + "Rules/";
    patternsdir = prefix + "Patterns/";
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT bool JNICALL Java_net_sf_golly_MainActivity_nativeAllowUndo(JNIEnv* env)
{
    return allowundo;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT bool JNICALL Java_net_sf_golly_MainActivity_nativeCanUndo(JNIEnv* env)
{
    return currlayer->undoredo->CanUndo();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT bool JNICALL Java_net_sf_golly_MainActivity_nativeCanRedo(JNIEnv* env)
{
    return currlayer->undoredo->CanRedo();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeUndo(JNIEnv* env)
{
    if (generating) Warning("Bug: generating is true in nativeUndo!");
    ClearMessage();
    CheckIfRendering();
    currlayer->undoredo->UndoChange();
    UpdateEverything();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeRedo(JNIEnv* env)
{
    if (generating) Warning("Bug: generating is true in nativeRedo!");
    ClearMessage();
    CheckIfRendering();
    currlayer->undoredo->RedoChange();
    UpdateEverything();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT bool JNICALL Java_net_sf_golly_MainActivity_nativeCanReset(JNIEnv* env)
{
    return currlayer->algo->getGeneration() > currlayer->startgen;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeResetPattern(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    ResetPattern();
    UpdateEverything();
}

// -----------------------------------------------------------------------------

void PauseGenerating()
{
    if (generating) {
        StopGenerating();
        // generating is now false
        paused = true;
    }
}

// -----------------------------------------------------------------------------

void ResumeGenerating()
{
    if (paused) {
        StartGenerating();
        // generating is probably true (false if pattern is empty)
        paused = false;
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativePauseGenerating(JNIEnv* env)
{
    PauseGenerating();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeResumeGenerating(JNIEnv* env)
{
    ResumeGenerating();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeStartGenerating(JNIEnv* env)
{
    if (!generating) {
        ClearMessage();
        StartGenerating();
        // generating might still be false (eg. if pattern is empty)

        // best to reset paused flag in case a PauseGenerating call
        // wasn't followed by a matching ResumeGenerating call
        paused = false;
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeStopGenerating(JNIEnv* env)
{
    if (generating) {
        StopGenerating();
        // generating is now false
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT bool JNICALL Java_net_sf_golly_MainActivity_nativeIsGenerating(JNIEnv* env)
{
    return generating;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeGenerate(JNIEnv* env)
{
    if (paused) return;     // PauseGenerating has been called

    // play safe and avoid re-entering currlayer->algo->step()
    if (event_checker > 0) return;

    // avoid calling NextGeneration while DrawPattern is executing on different thread
    if (rendering) return;

    NextGeneration(true);   // calls currlayer->algo->step() using current gen increment
    UpdatePattern();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeStep(JNIEnv* env)
{
    ClearMessage();
    NextGeneration(true);
    UpdatePattern();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT int JNICALL Java_net_sf_golly_MainActivity_nativeCalculateSpeed(JNIEnv* env)
{
    // calculate the interval (in millisecs) between nativeGenerate calls

    int interval = 1000/60;        // max speed is 60 fps

    // increase interval if user wants a delay
    if (currlayer->currexpo < 0) {
        interval = GetCurrentDelay();
    }

    return interval;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeStep1(JNIEnv* env)
{
    ClearMessage();
    // reset step exponent to 0
    currlayer->currexpo = 0;
    SetGenIncrement();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeFaster(JNIEnv* env)
{
    ClearMessage();
    // go faster by incrementing step exponent
    currlayer->currexpo++;
    SetGenIncrement();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeSlower(JNIEnv* env)
{
    ClearMessage();
    // go slower by decrementing step exponent
    if (currlayer->currexpo > minexpo) {
        currlayer->currexpo--;
        SetGenIncrement();
    } else {
        Beep();
    }
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeStopBeforeNew(JNIEnv* env)
{
    // NewPattern is about to clear all undo/redo history so there's no point
    // saving the current pattern (which might be very large)
    bool saveundo = allowundo;
    allowundo = false;                    // avoid calling RememberGenFinish
    if (generating) StopGenerating();
    allowundo = saveundo;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeNewPattern(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    NewPattern();
    UpdatePattern();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeFitPattern(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    currlayer->algo->fit(*currlayer->view, 1);
    UpdatePattern();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeScale1to1(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    // set scale to 1:1
    if (currlayer->view->getmag() != 0) {
        currlayer->view->setmag(0);
        UpdatePattern();
        UpdateStatus();
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeBigger(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    // zoom in
    if (currlayer->view->getmag() < MAX_MAG) {
        currlayer->view->zoom();
        UpdatePattern();
        UpdateStatus();
    } else {
        Beep();
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeSmaller(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    // zoom out
    currlayer->view->unzoom();
    UpdatePattern();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeMiddle(JNIEnv* env)
{
    ClearMessage();
    if (currlayer->originx == bigint::zero && currlayer->originy == bigint::zero) {
        currlayer->view->center();
    } else {
        // put cell saved by ChangeOrigin (not yet implemented!!!) in middle
        currlayer->view->setpositionmag(currlayer->originx, currlayer->originy, currlayer->view->getmag());
    }
    UpdatePattern();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT int JNICALL Java_net_sf_golly_MainActivity_nativeGetMode(JNIEnv* env)
{
    switch (currlayer->touchmode) {
        case drawmode:   return 0;
        case pickmode:   return 1;
        case selectmode: return 2;
        case movemode:   return 3;
    }
    Warning("Bug detected in nativeGetMode!");
    return 0;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeSetMode(JNIEnv* env, jobject obj, jint mode)
{
    ClearMessage();
    switch (mode) {
        case 0: currlayer->touchmode = drawmode;   return;
        case 1: currlayer->touchmode = pickmode;   return;
        case 2: currlayer->touchmode = selectmode; return;
        case 3: currlayer->touchmode = movemode;   return;
    }
    Warning("Bug detected in nativeSetMode!");
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT int JNICALL Java_net_sf_golly_MainActivity_nativeNumLayers(JNIEnv* env)
{
    return numlayers;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT bool JNICALL Java_net_sf_golly_MainActivity_nativePasteExists(JNIEnv* env)
{
    return waitingforpaste;
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT bool JNICALL Java_net_sf_golly_MainActivity_nativeSelectionExists(JNIEnv* env)
{
    return currlayer->currsel.Exists();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativePaste(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    PasteClipboard();
    UpdatePattern();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeSelectAll(JNIEnv* env)
{
    ClearMessage();
    SelectAll();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeRemoveSelection(JNIEnv* env)
{
    ClearMessage();
    RemoveSelection();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeCutSelection(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    CutSelection();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeCopySelection(JNIEnv* env)
{
    ClearMessage();
    CopySelection();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeClearSelection(JNIEnv* env, jobject obj, jint inside)
{
    ClearMessage();
    CheckIfRendering();
    if (inside) {
        ClearSelection();
    } else {
        ClearOutsideSelection();
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeShrinkSelection(JNIEnv* env)
{
    ClearMessage();
    ShrinkSelection(false);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeRandomFill(JNIEnv* env)
{
    ClearMessage();
    CheckIfRendering();
    RandomFill();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeFlipSelection(JNIEnv* env, jobject obj, jint y)
{
    ClearMessage();
    CheckIfRendering();
    FlipSelection(y);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeRotateSelection(JNIEnv* env, jobject obj, jint clockwise)
{
    ClearMessage();
    CheckIfRendering();
    RotateSelection(clockwise);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeAdvanceSelection(JNIEnv* env, jobject obj, jint inside)
{
    ClearMessage();
    CheckIfRendering();
    if (inside) {
        currlayer->currsel.Advance();
    } else {
        currlayer->currsel.AdvanceOutside();
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeAbortPaste(JNIEnv* env)
{
    ClearMessage();
    AbortPaste();
    UpdateEverything();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeDoPaste(JNIEnv* env, jobject obj, jint toselection)
{
    // assume caller has stopped generating
    ClearMessage();
    CheckIfRendering();
    DoPaste(toselection);
    UpdateEverything();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeFlipPaste(JNIEnv* env, jobject obj, jint y)
{
    ClearMessage();
    FlipPastePattern(y);
    UpdateEverything();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_MainActivity_nativeRotatePaste(JNIEnv* env, jobject obj, jint clockwise)
{
    ClearMessage();
    RotatePastePattern(clockwise);
    UpdateEverything();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternGLSurfaceView_nativePause(JNIEnv* env)
{
    PauseGenerating();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternGLSurfaceView_nativeResume(JNIEnv* env)
{
    ResumeGenerating();
    UpdatePattern();
    UpdateStatus();
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternGLSurfaceView_nativeTouchBegan(JNIEnv* env, jobject obj, jint x, jint y)
{
    // LOGI("touch began: x=%d y=%d", x, y);
    ClearMessage();
    TouchBegan(x, y);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternGLSurfaceView_nativeTouchMoved(JNIEnv* env, jobject obj, jint x, jint y)
{
    // LOGI("touch moved: x=%d y=%d", x, y);
    TouchMoved(x, y);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternGLSurfaceView_nativeTouchEnded(JNIEnv* env)
{
    // LOGI("touch ended");
    TouchEnded();
}

// -----------------------------------------------------------------------------

// called to initialize the graphics state
extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternRenderer_nativeInit(JNIEnv* env)
{
    // we only do 2D drawing
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_BLEND);
    // this blending function seems similar to the one used in desktop Golly
    // (ie. selected patterns look much the same)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternRenderer_nativeResize(JNIEnv* env, jobject obj, jint w, jint h)
{
    // LOGI("resize w=%d h=%d", w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(0, w, h, 0, -1, 1);     // origin is top left and y increases down
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);

    if (w != currlayer->view->getwidth() || h != currlayer->view->getheight()) {
        // update size of viewport
        ResizeLayers(w, h);
        UpdatePattern();
    }
}

// -----------------------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL Java_net_sf_golly_PatternRenderer_nativeRender(JNIEnv* env)
{
    // if NextGeneration is executing (on different thread) then don't call DrawPattern
    if (event_checker > 0) return;

    // render the current pattern; note that this occurs on a different thread
    // so we use rendering flag to prevent calls like NewPattern being called
    // before DrawPattern has finished
    rendering = true;
    DrawPattern(currindex);
    rendering = false;
}

// -----------------------------------------------------------------------------

std::string GetRuleName(const std::string& rule)
{
    std::string result = "";
    // not yet implemented!!!
    // maybe we should create rule.h and rule.cpp in gui-common???
    return result;
}

// -----------------------------------------------------------------------------

void UpdateEditBar()
{
    // call Java method in MainActivity class to update the buttons in the edit bar
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) env->CallVoidMethod(mainobj, id_UpdateEditBar);
    if (attached) javavm->DetachCurrentThread();
}

// -----------------------------------------------------------------------------

void BeginProgress(const char* title)
{
    /* not yet implemented!!!
    if (progresscount == 0) {
        // disable interaction with all views but don't show progress view just yet
        [globalController enableInteraction:NO];
        [globalTitle setText:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]];
        cancelProgress = false;
        progstart = TimeInSeconds();
    }
    progresscount++;    // handles nested calls
    */
}

// -----------------------------------------------------------------------------

bool AbortProgress(double fraction_done, const char* message)
{
    /* not yet implemented!!!
    if (progresscount <= 0) Fatal("Bug detected in AbortProgress!");
    double secs = TimeInSeconds() - progstart;
    if (!globalProgress.hidden) {
        if (secs < prognext) return false;
        prognext = secs + 0.1;     // update progress bar about 10 times per sec
        if (fraction_done < 0.0) {
            // show indeterminate progress gauge???
        } else {
            [globalController updateProgressBar:fraction_done];
        }
        return cancelProgress;
    } else {
        // note that fraction_done is not always an accurate estimator for how long
        // the task will take, especially when we use nextcell for cut/copy
        if ( (secs > 1.0 && fraction_done < 0.3) || secs > 2.0 ) {
            // task is probably going to take a while so show progress view
            globalProgress.hidden = NO;
            [globalController updateProgressBar:fraction_done];
        }
        prognext = secs + 0.01;     // short delay until 1st progress update
    }
    */
    return false;
}

// -----------------------------------------------------------------------------

void EndProgress()
{
    /* not yet implemented!!!
    if (progresscount <= 0) Fatal("Bug detected in EndProgress!");
    progresscount--;
    if (progresscount == 0) {
        // hide the progress view and enable interaction with other views
        globalProgress.hidden = YES;
        [globalController enableInteraction:YES];
    }
    */
}

// -----------------------------------------------------------------------------

void SwitchToPatternTab()
{
    // switch to pattern view (ie. MainActivity)
    // not yet implemented!!!
    LOGI("SwitchToPatternTab");
}

// -----------------------------------------------------------------------------

void ShowTextFile(const char* filepath)
{
    // display contents of text file in modal view (TextFileActivity!!!)
    // not yet implemented!!!
    LOGI("ShowTextFile: %s", filepath);
}

// -----------------------------------------------------------------------------

void ShowHelp(const char* filepath)
{
    // display html file in help view (HelpActivity!!!)
    // not yet implemented!!!
    LOGI("ShowHelp: %s", filepath);
}

// -----------------------------------------------------------------------------

void AndroidWarning(const char* msg)
{
    if (generating) paused = true;

    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) {
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(mainobj, id_Warning, jmsg);
        env->DeleteLocalRef(jmsg);
    }
    if (attached) javavm->DetachCurrentThread();

    if (generating) paused = false;
}

// -----------------------------------------------------------------------------

void AndroidFatal(const char* msg)
{
    paused = true;

    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) {
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(mainobj, id_Fatal, jmsg);
        env->DeleteLocalRef(jmsg);
    }
    if (attached) javavm->DetachCurrentThread();

    // id_Fatal calls System.exit(1)
    // exit(1);
}

// -----------------------------------------------------------------------------

bool AndroidYesNo(const char* query)
{
    std::string answer;
    if (generating) paused = true;

    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) {
        jstring jquery = env->NewStringUTF(query);
        jstring jresult = (jstring) env->CallObjectMethod(mainobj, id_YesNo, jquery);
        answer = ConvertJString(env, jresult);
        env->DeleteLocalRef(jquery);
        env->DeleteLocalRef(jresult);
    }
    if (attached) javavm->DetachCurrentThread();

    if (generating) paused = false;
    return answer == "yes";
}

// -----------------------------------------------------------------------------

void AndroidBeep()
{
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) env->CallVoidMethod(mainobj, id_PlayBeepSound);
    if (attached) javavm->DetachCurrentThread();
}

// -----------------------------------------------------------------------------

void AndroidRemoveFile(const std::string& filepath)
{
    // LOGI("AndroidRemoveFile: %s", filepath.c_str());
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) {
        jstring jpath = env->NewStringUTF(filepath.c_str());
        env->CallVoidMethod(mainobj, id_RemoveFile, jpath);
        env->DeleteLocalRef(jpath);
    }
    if (attached) javavm->DetachCurrentThread();
}

// -----------------------------------------------------------------------------

bool AndroidMoveFile(const std::string& inpath, const std::string& outpath)
{
    // LOGE("AndroidMoveFile: %s to %s", inpath.c_str(), outpath.c_str());
    std::string error = "env is null";

    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) {
        jstring joldpath = env->NewStringUTF(inpath.c_str());
        jstring jnewpath = env->NewStringUTF(outpath.c_str());
        jstring jresult = (jstring) env->CallObjectMethod(mainobj, id_MoveFile, joldpath, jnewpath);
        error = ConvertJString(env, jresult);
        env->DeleteLocalRef(jresult);
        env->DeleteLocalRef(joldpath);
        env->DeleteLocalRef(jnewpath);
    }
    if (attached) javavm->DetachCurrentThread();

    return error.length() == 0;
}

// -----------------------------------------------------------------------------

void AndroidFixURLPath(std::string& path)
{
    // replace "%..." with suitable chars for a file path (eg. %20 is changed to space)
    // not yet implemented!!!
    LOGE("AndroidFixURLPath: %s", path.c_str());
}

// -----------------------------------------------------------------------------

bool AndroidCopyTextToClipboard(const char* text)
{
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) {
        jstring jtext = env->NewStringUTF(text);
        env->CallVoidMethod(mainobj, id_CopyTextToClipboard, jtext);
        env->DeleteLocalRef(jtext);
    }
    if (attached) javavm->DetachCurrentThread();

    return env != NULL;
}

// -----------------------------------------------------------------------------

bool AndroidGetTextFromClipboard(std::string& text)
{
    text = "";
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) {
        jstring jtext = (jstring) env->CallObjectMethod(mainobj, id_GetTextFromClipboard);
        text = ConvertJString(env, jtext);
        env->DeleteLocalRef(jtext);
    }
    if (attached) javavm->DetachCurrentThread();

    if (text.length() == 0) {
        ErrorMessage("No text in clipboard.");
        return false;
    } else {
        return true;
    }
}

// -----------------------------------------------------------------------------

bool AndroidDownloadFile(const std::string& url, const std::string& filepath)
{
    // not yet implemented!!!
    LOGI("AndroidDownloadFile: url=%s file=%s", url.c_str(), filepath.c_str());
    return false;//!!!
}

// -----------------------------------------------------------------------------

void AndroidCheckEvents()
{
    // event_checker is > 0 in here (see gui-common/utils.cpp)
    if (rendering) {
        // best not to call CheckMessageQueue while DrawPattern is executing
        // (speeds up generating loop and might help avoid fatal SIGSEGV error)
        return;
    }
    bool attached;
    JNIEnv* env = getJNIenv(&attached);
    if (env) env->CallVoidMethod(mainobj, id_CheckMessageQueue);
    if (attached) javavm->DetachCurrentThread();
}

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Intent;
import android.graphics.Rect;
import android.os.Bundle;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.util.Log;

import org.chromium.base.CalledByNative;

import java.util.ArrayList;

/**
 * This class interfaces between the c++ SpeechInputManagerAndroid and the
 * system provided SpeechRecognizer API. It lets the c++ code call the speech
 * APIs and proxy back the responses.
 */
class SpeechInputManager implements RecognitionListener {

    private SpeechRecognizer mRecognizer;
    private int mSessionId = 0;

    @Override
    public void onError(int error) {
        // TODO(satish): Process the error code.
        nativeDidCompleteRecognition(mSessionId);
        mSessionId = 0;
    }

    @Override
    public void onResults(Bundle results) {
        ArrayList<String> items = results.getStringArrayList(
                SpeechRecognizer.RESULTS_RECOGNITION);
        nativeSetRecognitionResults(mSessionId,
                items.toArray(new String[items.size()]));
        nativeDidCompleteRecognition(mSessionId);
        mSessionId = 0;
    }

    @Override
    public void onPartialResults(Bundle partialResults) {
    }

    @Override
    public void onEvent(int eventType, Bundle params) {
    }

    @Override
    public void onReadyForSpeech(Bundle params) {
    }

    @Override
    public void onBeginningOfSpeech() {
    }

    @Override
    public void onRmsChanged(float rmsdB) {
    }

    @Override
    public void onBufferReceived(byte[] buffer) {
    }

    @Override
    public void onEndOfSpeech() {
        nativeDidCompleteRecording(mSessionId);
    }

    // The following methods are called by SpeechInputManagerAndroid using JNI.

    @SuppressWarnings("unused")
    @CalledByNative
    private void startSpeechRecognition(int sessionId, ChromeView chromeView,
            Rect elementRect, String language, String grammar) {
        if (mSessionId != 0) {
            Log.e("ChromeView", "startSpeechRecognition called when a session was already active.");
            return;
        }

        if (mRecognizer == null) {
            mRecognizer = SpeechRecognizer.createSpeechRecognizer(
                    chromeView.getContext().getApplicationContext());
            if (mRecognizer == null) {
                // TODO(satish): Show an error UI here.
                Log.e("ChromeView", "Could not get speech recognizer.");
                nativeDidCompleteRecognition(sessionId);
                return;
            }
            mRecognizer.setRecognitionListener(this);
        }

        mSessionId = sessionId;
        Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, language);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, grammar);
        intent.putExtra(RecognizerIntent.EXTRA_CALLING_PACKAGE,
                        "org.chromium.content.browser");
        // TODO(satish): Change this hardcoded value to a parameter once
        // maxresults is implemented as an attribute in webkit.
        intent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 5);
        mRecognizer.startListening(intent);

        // TODO(satish): Add a UI to indicate that recognition is in progress
        // and to show mic volume levels.
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void stopSpeechRecording() {
        mRecognizer.stopListening();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void cancelSpeechRecognition() {
        mRecognizer.cancel();
        mSessionId = 0;
    }

    // The following methods are implemented by SpeechInputManagerAndroid.
    private native void nativeSetRecognitionResults(
            int sessionId, String[] results);
    private native void nativeDidCompleteRecording(int sessionId);
    private native void nativeDidCompleteRecognition(int sessionId);
}

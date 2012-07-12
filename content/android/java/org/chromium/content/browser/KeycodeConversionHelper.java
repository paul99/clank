// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.KeyEvent;

/**
 * This is a helper class to handle the keycode conversion for key events that are sent to native.
 */
class KeycodeConversionHelper {

    /**
     * This is a helper class with no instance.
     */
    private KeycodeConversionHelper(){

    }

    /**
     * Converts the keycode for an Android {@link KeyEvent} to Windows keycode which is used by
     * WebKit.
     * @param keyCode Keycode to be converted.
     * @return The converted keycode.
     */
    static int convertAndroidKeyCodeToWebKit(int keyCode) {
        // This list is extended from the list in external/chrome
        // /third_party/WebKit/Source/WebCOre/platform/chromium/KeyCodeConversionAndroid.cpp.
        switch (keyCode) {
            case KeyEvent.KEYCODE_DEL:
                return 0x08;
            case KeyEvent.KEYCODE_TAB:
                return 0x09;
            case KeyEvent.KEYCODE_CLEAR:
                return 0x0C;
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_ENTER:
                return 0x0D;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT:
                return 0x10;
            case KeyEvent.KEYCODE_CTRL_LEFT:
            case KeyEvent.KEYCODE_CTRL_RIGHT:
                return 0x11;
            case KeyEvent.KEYCODE_ALT_LEFT:
            case KeyEvent.KEYCODE_ALT_RIGHT:
                return 0x12;
            // Back will serve as escape, although we may not have access to it.
            case KeyEvent.KEYCODE_BACK:
            case KeyEvent.KEYCODE_ESCAPE:
                return 0x1B;
            case KeyEvent.KEYCODE_SPACE:
                return 0x20;
            case KeyEvent.KEYCODE_PAGE_UP:
                return 0x21;
            case KeyEvent.KEYCODE_PAGE_DOWN:
                return 0x22;
            case KeyEvent.KEYCODE_MOVE_END:
                return 0x23;
            case KeyEvent.KEYCODE_MOVE_HOME:
                return 0x24;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                return 0x25;
            case KeyEvent.KEYCODE_DPAD_UP:
                return 0x26;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return 0x27;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                return 0x28;
            case KeyEvent.KEYCODE_INSERT:
                return 0x2D;
            case KeyEvent.KEYCODE_0:
                return 0x30;
            case KeyEvent.KEYCODE_1:
                return 0x31;
            case KeyEvent.KEYCODE_2:
                return 0x32;
            case KeyEvent.KEYCODE_3:
                return 0x33;
            case KeyEvent.KEYCODE_4:
                return 0x34;
            case KeyEvent.KEYCODE_5:
                return 0x35;
            case KeyEvent.KEYCODE_6:
                return 0x36;
            case KeyEvent.KEYCODE_7:
                return 0x37;
            case KeyEvent.KEYCODE_8:
                return 0x38;
            case KeyEvent.KEYCODE_9:
                return 0x39;
            case KeyEvent.KEYCODE_A:
                return 0x41;
            case KeyEvent.KEYCODE_B:
                return 0x42;
            case KeyEvent.KEYCODE_C:
                return 0x43;
            case KeyEvent.KEYCODE_D:
                return 0x44;
            case KeyEvent.KEYCODE_E:
                return 0x45;
            case KeyEvent.KEYCODE_F:
                return 0x46;
            case KeyEvent.KEYCODE_G:
                return 0x47;
            case KeyEvent.KEYCODE_H:
                return 0x48;
            case KeyEvent.KEYCODE_I:
                return 0x49;
            case KeyEvent.KEYCODE_J:
                return 0x4A;
            case KeyEvent.KEYCODE_K:
                return 0x4B;
            case KeyEvent.KEYCODE_L:
                return 0x4C;
            case KeyEvent.KEYCODE_M:
                return 0x4D;
            case KeyEvent.KEYCODE_N:
                return 0x4E;
            case KeyEvent.KEYCODE_O:
                return 0x4F;
            case KeyEvent.KEYCODE_P:
                return 0x50;
            case KeyEvent.KEYCODE_Q:
                return 0x51;
            case KeyEvent.KEYCODE_R:
                return 0x52;
            case KeyEvent.KEYCODE_S:
                return 0x53;
            case KeyEvent.KEYCODE_T:
                return 0x54;
            case KeyEvent.KEYCODE_U:
                return 0x55;
            case KeyEvent.KEYCODE_V:
                return 0x56;
            case KeyEvent.KEYCODE_W:
                return 0x57;
            case KeyEvent.KEYCODE_X:
                return 0x58;
            case KeyEvent.KEYCODE_Y:
                return 0x59;
            case KeyEvent.KEYCODE_Z:
                return 0x5A;
            case KeyEvent.KEYCODE_NUMPAD_0:
                return 0x60;
            case KeyEvent.KEYCODE_NUMPAD_1:
                return 0x61;
            case KeyEvent.KEYCODE_NUMPAD_2:
                return 0x62;
            case KeyEvent.KEYCODE_NUMPAD_3:
                return 0x63;
            case KeyEvent.KEYCODE_NUMPAD_4:
                return 0x64;
            case KeyEvent.KEYCODE_NUMPAD_5:
                return 0x65;
            case KeyEvent.KEYCODE_NUMPAD_6:
                return 0x66;
            case KeyEvent.KEYCODE_NUMPAD_7:
                return 0x67;
            case KeyEvent.KEYCODE_NUMPAD_8:
                return 0x68;
            case KeyEvent.KEYCODE_NUMPAD_9:
                return 0x69;
            case KeyEvent.KEYCODE_NUMPAD_MULTIPLY:
                return 0x6A;
            case KeyEvent.KEYCODE_NUMPAD_ADD:
                return 0x6B;
            case KeyEvent.KEYCODE_NUMPAD_SUBTRACT:
                return 0x6D;
            case KeyEvent.KEYCODE_NUMPAD_DIVIDE:
                return 0x6F;
            case KeyEvent.KEYCODE_F1:
                return 0x70;
            case KeyEvent.KEYCODE_F2:
                return 0x71;
            case KeyEvent.KEYCODE_F3:
                return 0x72;
            case KeyEvent.KEYCODE_F4:
                return 0x73;
            case KeyEvent.KEYCODE_F5:
                return 0x74;
            case KeyEvent.KEYCODE_F6:
                return 0x75;
            case KeyEvent.KEYCODE_F7:
                return 0x76;
            case KeyEvent.KEYCODE_F8:
                return 0x77;
            case KeyEvent.KEYCODE_F9:
                return 0x78;
            case KeyEvent.KEYCODE_F10:
                return 0x79;
            case KeyEvent.KEYCODE_F11:
                return 0x7A;
            case KeyEvent.KEYCODE_F12:
                return 0x7B;
            case KeyEvent.KEYCODE_NUM_LOCK:
                return 0x90;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                return 0xAE;
            case KeyEvent.KEYCODE_VOLUME_UP:
                return 0xAF;
            case KeyEvent.KEYCODE_MEDIA_NEXT:
                return 0xB0;
            case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
                return 0xB1;
            case KeyEvent.KEYCODE_MEDIA_STOP:
                return 0xB2;
            case KeyEvent.KEYCODE_MEDIA_PAUSE:
                return 0xB3;
            case KeyEvent.KEYCODE_SEMICOLON:
                return 0xBA;
            case KeyEvent.KEYCODE_NUMPAD_COMMA:
            case KeyEvent.KEYCODE_COMMA:
                return 0xBC;
            case KeyEvent.KEYCODE_MINUS:
                return 0xBD;
            case KeyEvent.KEYCODE_NUMPAD_EQUALS:
            case KeyEvent.KEYCODE_EQUALS:
                return 0xBB;
            case KeyEvent.KEYCODE_NUMPAD_DOT:
            case KeyEvent.KEYCODE_PERIOD:
                return 0xBE;
            case KeyEvent.KEYCODE_SLASH:
                return 0xBF;
            case KeyEvent.KEYCODE_LEFT_BRACKET:
                return 0xDB;
            case KeyEvent.KEYCODE_BACKSLASH:
                return 0xDC;
            case KeyEvent.KEYCODE_RIGHT_BRACKET:
                return 0xDD;
            case KeyEvent.KEYCODE_MUTE:
            case KeyEvent.KEYCODE_VOLUME_MUTE:
                return 0xAD;
            default:
                return 0;
        }
    }
}

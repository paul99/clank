// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_MEDIA_PLAYER_CONSTANTS_ANDROID_H_
#define WEBKIT_GLUE_MEDIA_PLAYER_CONSTANTS_ANDROID_H_
#pragma once

typedef enum {
  MEDIA_PREPARED = 1,
  MEDIA_PLAYBACK_COMPLETE = 2,
  MEDIA_BUFFERING_UPDATE = 3,
  MEDIA_SEEK_COMPLETE = 4,
  MEDIA_SET_VIDEO_SIZE = 5,
  MEDIA_ERROR = 100,
  MEDIA_INFO = 200,
} MediaEventType;

typedef enum {
    MEDIA_ERROR_UNKNOWN = 1,
    MEDIA_ERROR_SERVER_DIED = 100,
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
} MediaErrorType;

#endif  // WEBKIT_GLUE_MEDIA_PLAYER_CONSTANTS_ANDROID_H_

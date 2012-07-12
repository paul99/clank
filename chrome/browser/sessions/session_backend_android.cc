// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/session_backend.h"

#include "base/stl_util.h"

// TODO(joth): This is dirty hack to stop SessionBackend from touching the
// filesystem on Android as tab and session persistence is handled Java side.
// A better design would be to factor out just those bits of TabRestoreService
// that are required and not instantiate BaseSessionService or SessionBackend
// at all.

SessionBackend::SessionBackend(BaseSessionService::SessionType type,
                               const FilePath& path_to_dir) {
}

void SessionBackend::Init() {
}

void SessionBackend::AppendCommands(std::vector<SessionCommand*>* commands,
                                    bool reset_first) {
  STLDeleteElements(commands);
  delete commands;
}

void SessionBackend::ReadLastSessionCommands(
    scoped_refptr<BaseSessionService::InternalGetCommandsRequest> request) {
  if (request->canceled())
    return;
  Init();
  ReadLastSessionCommandsImpl(&(request->commands));
  request->ForwardResult(request->handle(), request);
}

bool SessionBackend::ReadLastSessionCommandsImpl(
    std::vector<SessionCommand*>* commands) {
  commands->clear();
  return false;
}

void SessionBackend::DeleteLastSession() {
}

void SessionBackend::MoveCurrentSessionToLastSession() {
}

void SessionBackend::ReadCurrentSessionCommands(
    scoped_refptr<BaseSessionService::InternalGetCommandsRequest> request) {
  if (request->canceled())
    return;
  Init();
  ReadCurrentSessionCommandsImpl(&(request->commands));
  request->ForwardResult(request->handle(), request);
}

bool SessionBackend::ReadCurrentSessionCommandsImpl(
    std::vector<SessionCommand*>* commands) {
  commands->clear();
  return false;
}

SessionBackend::~SessionBackend() {
}

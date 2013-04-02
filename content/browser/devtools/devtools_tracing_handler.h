// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TRACING_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TRACING_HANDLER_H_

#include "content/browser/devtools/devtools_browser_target.h"
#include "content/public/browser/trace_subscriber.h"

namespace content {

class DevToolsWebSocketSender;

// This class bridges DevTools remote debugging server with the trace
// infrastructure.
class DevToolsTracingHandler
    : public TraceSubscriber,
      public DevToolsBrowserTarget::DomainHandler {
 public:
  DevToolsTracingHandler();
  virtual ~DevToolsTracingHandler();

  // TraceSubscriber:
  virtual void OnEndTracingComplete() OVERRIDE;;
  virtual void OnTraceDataCollected(
      const scoped_refptr<base::RefCountedString>& trace_fragment) OVERRIDE;

 private:
  scoped_ptr<DevToolsProtocol::Response> OnStart(
      DevToolsProtocol::Command* command);
  scoped_ptr<DevToolsProtocol::Response> OnEnd(
      DevToolsProtocol::Command* command);

  bool is_running_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsTracingHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TRACING_HANDLER_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_BROWSER_TARGET_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_BROWSER_TARGET_H_

#include <map>
#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/devtools/devtools_protocol.h"

namespace base {

class DictionaryValue;
class MessageLoopProxy;
class Value;

}  // namespace base

namespace net {
class HttpServer;
}  // namespace net

namespace content {

// This class handles the "Browser" target for remote debugging.
class DevToolsBrowserTarget {
 public:
  typedef base::Callback<void(const std::string& method,
                              base::DictionaryValue* params)> Notifier;

  class DomainHandler {
   public:
    typedef base::Callback<scoped_ptr<DevToolsProtocol::Response>(
        DevToolsProtocol::Command* command)> CommandHandler;
    virtual ~DomainHandler();

    // Returns the domain name for this handler.
    std::string domain() { return domain_; }

    void RegisterCommandHandler(const std::string& command,
                                CommandHandler handler);

   protected:
    explicit DomainHandler(const std::string& domain);

    // |params| and |error_out| ownership is transferred to the
    // caller.
    virtual scoped_ptr<DevToolsProtocol::Response> HandleCommand(
        DevToolsProtocol::Command* command);

    // Sends notification to the client. Takes ownership of |params|.
    void SendNotification(const std::string& method,
                          base::DictionaryValue* params);

   private:
    friend class DevToolsBrowserTarget;
    void set_notifier(Notifier notifier) { notifier_ = notifier; }

    std::string domain_;
    Notifier notifier_;
    typedef std::map<std::string, CommandHandler> CommandHandlers;
    CommandHandlers command_handlers_;

    DISALLOW_COPY_AND_ASSIGN(DomainHandler);
  };

  DevToolsBrowserTarget(base::MessageLoopProxy* message_loop_proxy,
                        net::HttpServer* server,
                        int connection_id);
  ~DevToolsBrowserTarget();

  int connection_id() const { return connection_id_; }

  // Takes ownership of |handler|.
  void RegisterDomainHandler(DomainHandler* handler);

  std::string HandleMessage(const std::string& data);

 private:
  // Sends notification to the client. Passes ownership of |params|.
  void SendNotification(const std::string& method,
                        base::DictionaryValue* params);

  base::MessageLoopProxy* const message_loop_proxy_;
  net::HttpServer* const http_server_;
  const int connection_id_;

  typedef std::map<std::string, DomainHandler*> DomainHandlerMap;
  DomainHandlerMap handlers_;
  base::WeakPtrFactory<DevToolsBrowserTarget> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsBrowserTarget);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_BROWSER_TARGET_H_

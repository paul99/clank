// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_SHELL_BROWSER_CONTEXT_H_
#define CONTENT_SHELL_SHELL_BROWSER_CONTEXT_H_

#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_context.h"
#include "net/url_request/url_request_job_factory.h"

namespace content {

class DownloadManagerDelegate;
class ResourceContext;
class ShellDownloadManagerDelegate;
class ShellURLRequestContextGetter;

class ShellBrowserContext : public BrowserContext {
 public:
  explicit ShellBrowserContext(bool off_the_record);
  virtual ~ShellBrowserContext();

  // BrowserContext implementation.
  virtual base::FilePath GetPath() OVERRIDE;
  virtual bool IsOffTheRecord() const OVERRIDE;
  virtual DownloadManagerDelegate* GetDownloadManagerDelegate() OVERRIDE;
  virtual net::URLRequestContextGetter* GetRequestContext() OVERRIDE;
  virtual net::URLRequestContextGetter* GetRequestContextForRenderProcess(
      int renderer_child_id) OVERRIDE;
  virtual net::URLRequestContextGetter* GetMediaRequestContext() OVERRIDE;
  virtual net::URLRequestContextGetter* GetMediaRequestContextForRenderProcess(
      int renderer_child_id) OVERRIDE;
  virtual net::URLRequestContextGetter*
      GetMediaRequestContextForStoragePartition(
          const base::FilePath& partition_path,
          bool in_memory) OVERRIDE;
  virtual ResourceContext* GetResourceContext() OVERRIDE;
  virtual GeolocationPermissionContext*
      GetGeolocationPermissionContext() OVERRIDE;
  virtual SpeechRecognitionPreferences*
      GetSpeechRecognitionPreferences() OVERRIDE;
  virtual quota::SpecialStoragePolicy* GetSpecialStoragePolicy() OVERRIDE;

  net::URLRequestContextGetter* CreateRequestContext(
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          blob_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          file_system_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          developer_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          chrome_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          chrome_devtools_protocol_handler);
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const FilePath& partition_path,
      bool in_memory,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          blob_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          file_system_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          developer_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          chrome_protocol_handler,
      scoped_ptr<net::URLRequestJobFactory::ProtocolHandler>
          chrome_devtools_protocol_handler);

 private:
  class ShellResourceContext;

  // Performs initialization of the ShellBrowserContext while IO is still
  // allowed on the current thread.
  void InitWhileIOAllowed();

  bool off_the_record_;
  bool ignore_certificate_errors_;
  base::FilePath path_;
  scoped_ptr<ShellResourceContext> resource_context_;
  scoped_refptr<ShellDownloadManagerDelegate> download_manager_delegate_;
  scoped_refptr<ShellURLRequestContextGetter> url_request_getter_;

  DISALLOW_COPY_AND_ASSIGN(ShellBrowserContext);
};

}  // namespace content

#endif  // CONTENT_SHELL_SHELL_BROWSER_CONTEXT_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/render_view_devtools_agent_host.h"

#include "base/base64.h"
#include "base/basictypes.h"
#include "base/lazy_instance.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/devtools/devtools_manager_impl.h"
#include "content/browser/devtools/render_view_devtools_agent_host.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/site_instance_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/devtools_messages.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents_delegate.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDevToolsAgent.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/snapshot/snapshot.h"


namespace content {

typedef std::vector<RenderViewDevToolsAgentHost*> Instances;

namespace {
base::LazyInstance<Instances>::Leaky g_instances = LAZY_INSTANCE_INITIALIZER;

static RenderViewDevToolsAgentHost* FindAgentHost(RenderViewHost* rvh) {
  if (g_instances == NULL)
    return NULL;
  for (Instances::iterator it = g_instances.Get().begin();
       it != g_instances.Get().end(); ++it) {
    if (rvh == (*it)->render_view_host())
      return *it;
  }
  return NULL;
}

}  // namespace

using WebKit::WebDevToolsAgent;

class DevToolsAgentHostRvhObserver : public RenderViewHostObserver {
 public:
  DevToolsAgentHostRvhObserver(RenderViewHost* rvh,
                               RenderViewDevToolsAgentHost* agent_host)
      : RenderViewHostObserver(rvh),
        agent_host_(agent_host) {
  }
  virtual ~DevToolsAgentHostRvhObserver() {}

  // RenderViewHostObserver overrides.
  virtual void RenderViewHostDestroyed(RenderViewHost* rvh) OVERRIDE {
    agent_host_->RenderViewHostDestroyed(rvh);
  }
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE {
    return agent_host_->OnRvhMessageReceived(message);
  }
 private:
  RenderViewDevToolsAgentHost* agent_host_;
  DISALLOW_COPY_AND_ASSIGN(DevToolsAgentHostRvhObserver);
};

// static
scoped_refptr<DevToolsAgentHost>
DevToolsAgentHost::GetFor(RenderViewHost* rvh) {
  RenderViewDevToolsAgentHost* result = FindAgentHost(rvh);
  if (!result)
    result = new RenderViewDevToolsAgentHost(rvh);
  return result;
}

// static
bool DevToolsAgentHost::HasFor(RenderViewHost* rvh) {
  return FindAgentHost(rvh) != NULL;
}

// static
bool DevToolsAgentHost::IsDebuggerAttached(WebContents* web_contents) {
  if (g_instances == NULL)
    return false;
  DevToolsManager* devtools_manager = DevToolsManager::GetInstance();
  if (!devtools_manager)
    return false;
  RenderViewHostDelegate* delegate =
      static_cast<WebContentsImpl*>(web_contents);
  for (Instances::iterator it = g_instances.Get().begin();
       it != g_instances.Get().end(); ++it) {
    RenderViewHost* rvh = (*it)->render_view_host_;
    if (rvh && rvh->GetDelegate() != delegate)
      continue;
    if (devtools_manager->GetDevToolsClientHostFor(*it))
      return true;
  }
  return false;
}

// static
int DevToolsAgentHost::DisconnectRenderViewHost(RenderViewHost* rvh) {
  RenderViewDevToolsAgentHost* agent_host = FindAgentHost(rvh);
  if (!agent_host)
    return -1;
  agent_host->DisconnectRenderViewHost();
  return agent_host->id();
}

// static
void DevToolsAgentHost::ConnectRenderViewHost(int cookie,
                                              RenderViewHost* rvh) {
  for (Instances::iterator it = g_instances.Get().begin();
       it != g_instances.Get().end(); ++it) {
    if (cookie == (*it)->id()) {
      (*it)->ConnectRenderViewHost(rvh, true);
      break;
    }
  }
}

// static
void RenderViewDevToolsAgentHost::OnCancelPendingNavigation(
    RenderViewHost* pending,
    RenderViewHost* current) {
  int cookie = DevToolsAgentHost::DisconnectRenderViewHost(pending);
  if (cookie != -1)
    DevToolsAgentHost::ConnectRenderViewHost(cookie, current);
}

RenderViewDevToolsAgentHost::RenderViewDevToolsAgentHost(
    RenderViewHost* rvh) {
  ConnectRenderViewHost(rvh, false);
  g_instances.Get().push_back(this);
  RenderViewHostDelegate* delegate = render_view_host_->GetDelegate();
  if (delegate && delegate->GetAsWebContents())
    Observe(delegate->GetAsWebContents());
}

RenderViewHost* RenderViewDevToolsAgentHost::GetRenderViewHost() {
  return render_view_host_;
}

void RenderViewDevToolsAgentHost::SendMessageToAgent(IPC::Message* msg) {
  msg->set_routing_id(render_view_host_->GetRoutingID());
  render_view_host_->Send(msg);
}

void RenderViewDevToolsAgentHost::NotifyClientAttaching() {
  if (!render_view_host_)
    return;

  ChildProcessSecurityPolicyImpl::GetInstance()->GrantReadRawCookies(
      render_view_host_->GetProcess()->GetID());

  NotificationService::current()->Notify(
      NOTIFICATION_DEVTOOLS_AGENT_ATTACHED,
      Source<BrowserContext>(
          render_view_host_->GetSiteInstance()->GetProcess()->
              GetBrowserContext()),
      Details<RenderViewHost>(render_view_host_));
}

void RenderViewDevToolsAgentHost::NotifyClientDetaching() {
  if (!render_view_host_)
    return;

  DevToolsManager* devtools_manager = DevToolsManager::GetInstance();
  bool process_has_agents = false;
  RenderProcessHost* render_process_host = render_view_host_->GetProcess();
  for (Instances::iterator it = g_instances.Get().begin();
       it != g_instances.Get().end(); ++it) {
    if (*it == this || !devtools_manager->GetDevToolsClientHostFor(*it))
      continue;
    RenderViewHost* rvh = (*it)->render_view_host();
    if (rvh && rvh->GetProcess() == render_process_host)
      process_has_agents = true;
  }

  // We are the last to disconnect from the renderer -> revoke permissions.
  if (!process_has_agents) {
    ChildProcessSecurityPolicyImpl::GetInstance()->RevokeReadRawCookies(
        render_process_host->GetID());
  }

  NotificationService::current()->Notify(
      NOTIFICATION_DEVTOOLS_AGENT_DETACHED,
      Source<BrowserContext>(
          render_view_host_->GetSiteInstance()->GetProcess()->
              GetBrowserContext()),
      Details<RenderViewHost>(render_view_host_));
}

RenderViewDevToolsAgentHost::~RenderViewDevToolsAgentHost() {
  Instances::iterator it = std::find(g_instances.Get().begin(),
                                     g_instances.Get().end(),
                                     this);
  if (it != g_instances.Get().end())
    g_instances.Get().erase(it);
}

void RenderViewDevToolsAgentHost::AboutToNavigateRenderView(
    RenderViewHost* dest_rvh) {
  if (!render_view_host_)
    return;

  if (render_view_host_ == dest_rvh && static_cast<RenderViewHostImpl*>(
          render_view_host_)->render_view_termination_status() ==
              base::TERMINATION_STATUS_STILL_RUNNING)
    return;
  DisconnectRenderViewHost();
  ConnectRenderViewHost(dest_rvh, true);
}

void RenderViewDevToolsAgentHost::ConnectRenderViewHost(RenderViewHost* rvh,
                                                        bool reattach) {
  render_view_host_ = rvh;
  rvh_observer_.reset(new DevToolsAgentHostRvhObserver(rvh, this));
  if (reattach)
    Reattach(state_);
}

void RenderViewDevToolsAgentHost::DisconnectRenderViewHost() {
  NotifyClientDetaching();
  rvh_observer_.reset();
  render_view_host_ = NULL;
}

void RenderViewDevToolsAgentHost::RenderViewHostDestroyed(
    RenderViewHost* rvh) {
  DCHECK(render_view_host_);
  scoped_refptr<RenderViewDevToolsAgentHost> protect(this);
  NotifyCloseListener();
  render_view_host_ = NULL;
}

bool RenderViewDevToolsAgentHost::OnRvhMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderViewDevToolsAgentHost, message)
    IPC_MESSAGE_HANDLER(DevToolsClientMsg_DispatchOnInspectorFrontend,
                        OnDispatchOnInspectorFrontend)
    IPC_MESSAGE_HANDLER(DevToolsHostMsg_SaveAgentRuntimeState,
                        OnSaveAgentRuntimeState)
    IPC_MESSAGE_HANDLER(DevToolsHostMsg_ClearBrowserCache, OnClearBrowserCache)
    IPC_MESSAGE_HANDLER(DevToolsHostMsg_ClearBrowserCookies,
                        OnClearBrowserCookies)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderViewDevToolsAgentHost::OnSaveAgentRuntimeState(
    const std::string& state) {
  if (!render_view_host_)
    return;
  state_ = state;
}

void RenderViewDevToolsAgentHost::OnDispatchOnInspectorFrontend(
    const std::string& message) {
  if (!render_view_host_)
    return;

  WebDevToolsAgent::BrowserDataHint dataHint =
      WebDevToolsAgent::shouldPatchWithBrowserData(message.data(),
                                                   message.length());
  if (dataHint == WebDevToolsAgent::BrowserDataHintNone) {
    DevToolsManagerImpl::GetInstance()->DispatchOnInspectorFrontend(
        this, message);
    return;
  }

  // Prepare the data and patch message with it.
  std::string overriden_message;
  switch (dataHint) {
    case WebDevToolsAgent::BrowserDataHintScreenshot:
      {
        std::string base_64_data;
        if (CaptureScreenshot(&base_64_data)) {
          overriden_message = WebDevToolsAgent::patchWithBrowserData(
              WebKit::WebString::fromUTF8(message),
              dataHint,
              WebKit::WebString::fromUTF8(base_64_data)).utf8();
        }
        break;
      }
    case WebDevToolsAgent::BrowserDataHintAcceptJavaScriptDialog:
    case WebDevToolsAgent::BrowserDataHintDismissJavaScriptDialog:
      {
        bool accept =
            dataHint == WebDevToolsAgent::BrowserDataHintAcceptJavaScriptDialog;
        if (HandleJavaScriptDialog(accept)) {
          overriden_message = WebDevToolsAgent::patchWithBrowserData(
              WebKit::WebString::fromUTF8(message), dataHint, "").utf8();
        }
        break;
      }
    case WebDevToolsAgent::BrowserDataHintNone:
      // Fall through.
    default:
      overriden_message = message;
  }

  DevToolsManagerImpl::GetInstance()->DispatchOnInspectorFrontend(
      this, overriden_message);
}

void RenderViewDevToolsAgentHost::OnClearBrowserCache() {
  if (render_view_host_)
    GetContentClient()->browser()->ClearCache(render_view_host_);
}

void RenderViewDevToolsAgentHost::OnClearBrowserCookies() {
  if (render_view_host_)
    GetContentClient()->browser()->ClearCookies(render_view_host_);
}

bool RenderViewDevToolsAgentHost::CaptureScreenshot(std::string* base_64_data) {
  DCHECK(render_view_host_);
  gfx::Rect view_bounds = render_view_host_->GetView()->GetViewBounds();
  gfx::Rect snapshot_bounds(view_bounds.size());
  gfx::Size snapshot_size = snapshot_bounds.size();
  std::vector<unsigned char> png;
  if (!ui::GrabViewSnapshot(render_view_host_->GetView()->GetNativeView(),
                            &png,
                            snapshot_bounds))
    return false;

  return base::Base64Encode(base::StringPiece(
                                reinterpret_cast<char*>(&*png.begin()),
                                png.size()),
                            base_64_data);
}

bool RenderViewDevToolsAgentHost::HandleJavaScriptDialog(bool accept) {
  DCHECK(render_view_host_);
  WebContentsImpl* web_contents = static_cast<WebContentsImpl*>(
      render_view_host_->GetDelegate()->GetAsWebContents());
  if (web_contents) {
    JavaScriptDialogManager* manager =
        web_contents->GetDelegate()->GetJavaScriptDialogManager();
    if (!manager)
      return false;
    return manager->HandleJavaScriptDialog(web_contents, accept);
  }
  return false;
}

}  // namespace content

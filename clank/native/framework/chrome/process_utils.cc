// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/process_utils.h"

#include <set>

#include "base/file_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/render_messages.h"
#include "content/browser/android/chrome_view.h"
#include "content/public/browser/render_process_host.h"
#include "jni/process_utils_jni.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

using content::BrowserThread;

namespace {

void CloseIdleConnectionsForProfile(
    scoped_refptr<net::URLRequestContextGetter> context_getter) {
  DCHECK(context_getter.get());
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                           base::Bind(&CloseIdleConnectionsForProfile,
                                      context_getter));
    return;
  }

  net::URLRequestContext* context = context_getter->GetURLRequestContext();
  DCHECK(context->http_transaction_factory());
  // TODO(satish): Remove this early return after verifying the above doesn't
  // fail.
  if (!context->http_transaction_factory())
    return;
  net::HttpNetworkSession* session =
      context->http_transaction_factory()->GetSession();
  DCHECK(session);
  // TODO(satish): Remove this check after verifying the above doesn't fail.
  if (session)
    session->CloseIdleConnections();
}

}  // namespace

static void ToggleWebKitSharedTimers(JNIEnv* env,
                                     jclass obj,
                                     jboolean suspend) {
  typedef std::set<int> ProcessIdSet;
  static ProcessIdSet suspended_ids;

  if (suspend) {
    // An earlier suspend should have been balanced with a resume.
    DCHECK(suspended_ids.empty());

    // Suspend all current render processes.
    ProcessIdSet ids;
    for (content::RenderProcessHost::iterator i(
            content::RenderProcessHost::AllHostsIterator());
         !i.IsAtEnd(); i.Advance()) {
      content::RenderProcessHost* host = i.GetCurrentValue();
      int id = host->GetID();
      // TODO(satish): If we don't hit the above DCHECK for a long time then we
      // are sure that the suspended set is empty, so this check can be removed.
      if (suspended_ids.find(id) == suspended_ids.end())
        host->Send(new ChromeViewMsg_ToggleWebKitSharedTimer(true));
      ids.insert(id);
    }
    suspended_ids = ids;
  } else {
    // Only resume timers in the processes that were suspended earlier. If there
    // were any new processes created since the last time suspend was called, we
    // won't call resume on them because that is an unsupported call sequence
    // and timers freeze in that scenario.
    for (ProcessIdSet::iterator i = suspended_ids.begin();
         i != suspended_ids.end(); ++i) {
      content::RenderProcessHost* host = content::RenderProcessHost::FromID(*i);
      if (host)
        host->Send(new ChromeViewMsg_ToggleWebKitSharedTimer(false));
    }
    suspended_ids.clear();
  }
}

static void CloseIdleConnections(JNIEnv* env, jclass obj,
                                 jobject view_for_profile) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env,
                                                            view_for_profile);

  // Close idle connections that are linked to both the normal and incognito
  // profile. The ChromeView parameter could be either a normal or incognito
  // view so we take care of both cases here.
  Profile* profile = chrome_view->GetProfile();
  DCHECK(profile);
  CloseIdleConnectionsForProfile(profile->GetRequestContext());
  if (profile->IsOffTheRecord()) {
    CloseIdleConnectionsForProfile(
        profile->GetOriginalProfile()->GetRequestContext());
  } else {
    // Profile::GetOffTheRecordProfile creates one if not present already, so
    // check before making the call.
    if (profile->HasOffTheRecordProfile()) {
      CloseIdleConnectionsForProfile(
          profile->GetOffTheRecordProfile()->GetRequestContext());
    }
  }
}

bool RegisterProcessUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

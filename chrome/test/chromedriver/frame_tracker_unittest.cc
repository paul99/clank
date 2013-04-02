// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "chrome/test/chromedriver/frame_tracker.h"
#include "chrome/test/chromedriver/status.h"
#include "chrome/test/chromedriver/stub_devtools_client.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(FrameTracker, GetContextIdForFrame) {
  StubDevToolsClient client;
  FrameTracker tracker(&client);
  int context_id = -1;
  ASSERT_TRUE(tracker.GetContextIdForFrame("f", &context_id).IsError());
  ASSERT_EQ(-1, context_id);

  const char context[] = "{\"id\":100,\"frameId\":\"f\"}";
  base::DictionaryValue params;
  params.Set("context", base::JSONReader::Read(context));
  tracker.OnEvent("Runtime.executionContextCreated", params);
  ASSERT_TRUE(tracker.GetContextIdForFrame("foo", &context_id).IsError());
  ASSERT_EQ(-1, context_id);
  ASSERT_TRUE(tracker.GetContextIdForFrame("f", &context_id).IsOk());
  ASSERT_EQ(100, context_id);

  tracker.OnEvent("DOM.documentUpdated", params);
  ASSERT_TRUE(tracker.GetContextIdForFrame("f", &context_id).IsError());
}

TEST(FrameTracker, GetFrameForContextId) {
  StubDevToolsClient client;
  FrameTracker tracker(&client);
  std::string frame_id = "NONE";
  ASSERT_TRUE(tracker.GetFrameForContextId(0, &frame_id).IsError());
  ASSERT_EQ("NONE", frame_id);

  const char context[] = "{\"id\":100,\"frameId\":\"f\"}";
  base::DictionaryValue params;
  params.Set("context", base::JSONReader::Read(context));
  tracker.OnEvent("Runtime.executionContextCreated", params);
  ASSERT_TRUE(tracker.GetFrameForContextId(0, &frame_id).IsError());
  ASSERT_EQ("NONE", frame_id);
  ASSERT_TRUE(tracker.GetFrameForContextId(100, &frame_id).IsOk());
  ASSERT_EQ("f", frame_id);

  tracker.OnEvent("DOM.documentUpdated", params);
  ASSERT_TRUE(tracker.GetFrameForContextId(100, &frame_id).IsError());
}

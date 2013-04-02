// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/dom_tracker.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/test/chromedriver/devtools_client.h"
#include "chrome/test/chromedriver/status.h"

DomTracker::DomTracker(DevToolsClient* client) : client_(client) {
  DCHECK(client_);
  client_->AddListener(this);
}

DomTracker::~DomTracker() {}

Status DomTracker::GetFrameIdForNode(
    int node_id, std::string* frame_id) {
  if (node_to_frame_map_.count(node_id) == 0)
    return Status(kUnknownError, "element is not a frame");
  *frame_id = node_to_frame_map_[node_id];
  return Status(kOk);
}

Status DomTracker::OnConnected() {
  // Fetch the root document node so that Inspector will push DOM node
  // information to the client.
  base::DictionaryValue params;
  return client_->SendCommand("DOM.getDocument", params);
}

void DomTracker::OnEvent(const std::string& method,
                         const base::DictionaryValue& params) {
  if (method == "DOM.setChildNodes") {
    const base::Value* nodes;
    if (!params.Get("nodes", &nodes)) {
      LOG(ERROR) << "DOM.setChildNodes missing 'nodes'";
      return;
    }
    if (!ProcessNodeList(nodes)) {
      std::string json;
      base::JSONWriter::Write(nodes, &json);
      LOG(ERROR) << "DOM.setChildNodes has invalid 'nodes': " << json;
    }
  } else if (method == "DOM.documentUpdated") {
    node_to_frame_map_.clear();
    base::DictionaryValue params;
    client_->SendCommand("DOM.getDocument", params);
  }
}

bool DomTracker::ProcessNodeList(const base::Value* nodes) {
  const base::ListValue* nodes_list;
  if (!nodes->GetAsList(&nodes_list))
    return false;
  for (size_t i = 0; i < nodes_list->GetSize(); ++i) {
    const base::Value* node;
    if (!nodes_list->Get(i, &node))
      return false;
    if (!ProcessNode(node))
      return false;
  }
  return true;
}

bool DomTracker::ProcessNode(const base::Value* node) {
  const base::DictionaryValue* dict;
  if (!node->GetAsDictionary(&dict))
    return false;
  int node_id;
  if (!dict->GetInteger("nodeId", &node_id))
    return false;
  std::string frame_id;
  if (dict->GetString("frameId", &frame_id))
    node_to_frame_map_.insert(std::make_pair(node_id, frame_id));

  const base::Value* children;
  if (dict->Get("children", &children))
    return ProcessNodeList(children);
  return true;
}

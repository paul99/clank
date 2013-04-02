// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/renderer/media/mock_web_rtc_peer_connection_handler_client.h"

#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebMediaStream.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebString.h"

namespace content {

MockWebRTCPeerConnectionHandlerClient::
MockWebRTCPeerConnectionHandlerClient()
    : renegotiate_(false),
      signaling_state_(SignalingStateStable),
      ice_connection_state_(ICEConnectionStateStarting),
      ice_gathering_state_(ICEGatheringStateNew),
      candidate_mline_index_(-1) {
}

MockWebRTCPeerConnectionHandlerClient::
~MockWebRTCPeerConnectionHandlerClient() {}

void MockWebRTCPeerConnectionHandlerClient::negotiationNeeded() {
  renegotiate_ = true;
}

void MockWebRTCPeerConnectionHandlerClient::didGenerateICECandidate(
    const WebKit::WebRTCICECandidate& candidate) {
  if (!candidate.isNull()) {
    candidate_sdp_ = UTF16ToUTF8(candidate.candidate());
    candidate_mline_index_ = candidate.sdpMLineIndex();
    candidate_mid_ = UTF16ToUTF8(candidate.sdpMid());
  } else {
    candidate_sdp_ = "";
    candidate_mline_index_ = -1;
    candidate_mid_ = "";
  }
}

void MockWebRTCPeerConnectionHandlerClient::didChangeSignalingState(
    SignalingState state) {
  signaling_state_ = state;
}


void MockWebRTCPeerConnectionHandlerClient::didChangeICEConnectionState(
    ICEConnectionState state) {
  ice_connection_state_ = state;
}

void MockWebRTCPeerConnectionHandlerClient::didChangeICEGatheringState(
    ICEGatheringState state) {
  ice_gathering_state_ = state;
}

void MockWebRTCPeerConnectionHandlerClient::didAddRemoteStream(
    const WebKit::WebMediaStream& stream_descriptor) {
  stream_label_ = UTF16ToUTF8(stream_descriptor.label());
}

void MockWebRTCPeerConnectionHandlerClient::didRemoveRemoteStream(
    const WebKit::WebMediaStream& stream_descriptor) {
  DCHECK(stream_label_ == UTF16ToUTF8(stream_descriptor.label()));
  stream_label_.clear();
}

}  // namespace content

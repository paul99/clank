// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_FAKE_LAYER_TREE_HOST_IMPL_H_
#define CC_TEST_FAKE_LAYER_TREE_HOST_IMPL_H_

#include "cc/layer_tree_host_impl.h"
#include "cc/single_thread_proxy.h"
#include "cc/test/fake_layer_tree_host_impl_client.h"

namespace cc {

class FakeLayerTreeHostImpl : public LayerTreeHostImpl {
 public:
  FakeLayerTreeHostImpl(Proxy* proxy);
  virtual ~FakeLayerTreeHostImpl();

  using LayerTreeHostImpl::resetNeedsUpdateDrawPropertiesForTesting;

 private:
  FakeLayerTreeHostImplClient client_;
  LayerTreeSettings settings_;
};

}  // namespace cc

#endif  // CC_TEST_FAKE_LAYER_TREE_HOST_IMPL_H_

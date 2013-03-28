// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WIN8_METRO_DRIVER_CHROME_APP_VIEW_ASH_H_
#define WIN8_METRO_DRIVER_CHROME_APP_VIEW_ASH_H_

#include <windows.applicationmodel.core.h>
#include <windows.ui.core.h>
#include <windows.ui.input.h>
#include <windows.ui.viewmanagement.h>

#include "base/memory/scoped_ptr.h"
#include "ui/base/events/event_constants.h"
#include "win8/metro_driver/direct3d_helper.h"

namespace IPC {
  class Listener;
  class ChannelProxy;
}

class ChromeAppViewAsh
    : public mswr::RuntimeClass<winapp::Core::IFrameworkView> {
 public:
  ChromeAppViewAsh();
  ~ChromeAppViewAsh();

  // IViewProvider overrides.
  IFACEMETHOD(Initialize)(winapp::Core::ICoreApplicationView* view);
  IFACEMETHOD(SetWindow)(winui::Core::ICoreWindow* window);
  IFACEMETHOD(Load)(HSTRING entryPoint);
  IFACEMETHOD(Run)();
  IFACEMETHOD(Uninitialize)();

  void OnSetCursor(HCURSOR cursor);

 private:
  HRESULT OnActivate(winapp::Core::ICoreApplicationView* view,
                     winapp::Activation::IActivatedEventArgs* args);

  HRESULT OnPointerMoved(winui::Core::ICoreWindow* sender,
                         winui::Core::IPointerEventArgs* args);

  HRESULT OnPointerPressed(winui::Core::ICoreWindow* sender,
                           winui::Core::IPointerEventArgs* args);

  HRESULT OnPointerReleased(winui::Core::ICoreWindow* sender,
                            winui::Core::IPointerEventArgs* args);

  HRESULT OnWheel(winui::Core::ICoreWindow* sender,
                  winui::Core::IPointerEventArgs* args);

  HRESULT OnKeyDown(winui::Core::ICoreWindow* sender,
                    winui::Core::IKeyEventArgs* args);

  HRESULT OnKeyUp(winui::Core::ICoreWindow* sender,
                  winui::Core::IKeyEventArgs* args);

  HRESULT OnCharacterReceived(winui::Core::ICoreWindow* sender,
                              winui::Core::ICharacterReceivedEventArgs* args);

  HRESULT OnVisibilityChanged(winui::Core::ICoreWindow* sender,
                              winui::Core::IVisibilityChangedEventArgs* args);

  mswr::ComPtr<winui::Core::ICoreWindow> window_;
  mswr::ComPtr<winapp::Core::ICoreApplicationView> view_;
  EventRegistrationToken activated_token_;
  EventRegistrationToken pointermoved_token_;
  EventRegistrationToken pointerpressed_token_;
  EventRegistrationToken pointerreleased_token_;
  EventRegistrationToken wheel_token_;
  EventRegistrationToken keydown_token_;
  EventRegistrationToken keyup_token_;
  EventRegistrationToken character_received_token_;
  EventRegistrationToken visibility_changed_token_;

  // Keep state about which button is currently down, if any, as PointerMoved
  // events do not contain that state, but Ash's MouseEvents need it.
  ui::EventFlags mouse_down_flags_;

  metro_driver::Direct3DHelper direct3d_helper_;

  IPC::ChannelProxy* ui_channel_;
};

#endif  // WIN8_METRO_DRIVER_CHROME_APP_VIEW_ASH_H_

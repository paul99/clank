// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/webkit_test_runner.h"

#include <cmath>

#include "base/base64.h"
#include "base/md5.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/stringprintf.h"
#include "base/sys_string_conversions.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "content/public/renderer/render_view.h"
#include "content/public/test/layouttest_support.h"
#include "content/shell/shell_messages.h"
#include "content/shell/shell_render_process_observer.h"
#include "content/shell/webkit_test_helpers.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/Source/Platform/chromium/public/Platform.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCString.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebRect.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebSize.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebString.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebURL.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebURLError.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebContextMenuData.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDevToolsAgent.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebElement.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "third_party/WebKit/Tools/DumpRenderTree/chromium/TestRunner/public/WebTask.h"
#include "third_party/WebKit/Tools/DumpRenderTree/chromium/TestRunner/public/WebTestProxy.h"
#include "webkit/base/file_path_string_conversions.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/glue/webpreferences.h"

using WebKit::Platform;
using WebKit::WebContextMenuData;
using WebKit::WebDevToolsAgent;
using WebKit::WebElement;
using WebKit::WebFrame;
using WebKit::WebGamepads;
using WebKit::WebRect;
using WebKit::WebSize;
using WebKit::WebString;
using WebKit::WebURL;
using WebKit::WebURLError;
using WebKit::WebVector;
using WebKit::WebView;
using WebTestRunner::WebPreferences;
using WebTestRunner::WebTask;

namespace content {

namespace {

void InvokeTaskHelper(void* context) {
  WebTask* task = reinterpret_cast<WebTask*>(context);
  task->run();
  delete task;
}

std::string DumpDocumentText(WebFrame* frame) {
  // We use the document element's text instead of the body text here because
  // not all documents have a body, such as XML documents.
  WebElement documentElement = frame->document().documentElement();
  if (documentElement.isNull())
    return std::string();
  return documentElement.innerText().utf8();
}

std::string DumpDocumentPrintedText(WebFrame* frame) {
  return frame->renderTreeAsText(WebFrame::RenderAsTextPrinting).utf8();
}

std::string DumpFramesAsText(WebFrame* frame, bool printing, bool recursive) {
  std::string result;

  // Cannot do printed format for anything other than HTML.
  if (printing && !frame->document().isHTMLDocument())
    return std::string();

  // Add header for all but the main frame. Skip emtpy frames.
  if (frame->parent() && !frame->document().documentElement().isNull()) {
    result.append("\n--------\nFrame: '");
    result.append(frame->uniqueName().utf8().data());
    result.append("'\n--------\n");
  }

  result.append(
      printing ? DumpDocumentPrintedText(frame) : DumpDocumentText(frame));
  result.append("\n");

  if (recursive) {
    for (WebFrame* child = frame->firstChild(); child;
         child = child->nextSibling()) {
      result.append(DumpFramesAsText(child, printing, recursive));
    }
  }
  return result;
}

std::string DumpFrameScrollPosition(WebFrame* frame, bool recursive) {
  std::string result;

  WebSize offset = frame->scrollOffset();
  if (offset.width > 0 || offset.height > 0) {
    if (frame->parent()) {
      result.append(
          base::StringPrintf("frame '%s' ", frame->uniqueName().utf8().data()));
    }
    result.append(
        base::StringPrintf("scrolled to %d,%d\n", offset.width, offset.height));
  }

  if (recursive) {
    for (WebFrame* child = frame->firstChild(); child;
         child = child->nextSibling()) {
      result.append(DumpFrameScrollPosition(child, recursive));
    }
  }
  return result;
}

#if !defined(OS_MACOSX)
void MakeBitmapOpaque(SkBitmap* bitmap) {
  SkAutoLockPixels lock(*bitmap);
  DCHECK(bitmap->config() == SkBitmap::kARGB_8888_Config);
  for (int y = 0; y < bitmap->height(); ++y) {
    uint32_t* row = bitmap->getAddr32(0, y);
    for (int x = 0; x < bitmap->width(); ++x)
      row[x] |= 0xFF000000;  // Set alpha bits to 1.
  }
}
#endif

void CopyCanvasToBitmap(SkCanvas* canvas,  SkBitmap* snapshot) {
  SkDevice* device = skia::GetTopDevice(*canvas);
  const SkBitmap& bitmap = device->accessBitmap(false);
  bitmap.copyTo(snapshot, SkBitmap::kARGB_8888_Config);

#if !defined(OS_MACOSX)
  // Only the expected PNGs for Mac have a valid alpha channel.
  MakeBitmapOpaque(snapshot);
#endif

}

}  // namespace

WebKitTestRunner::WebKitTestRunner(RenderView* render_view)
    : RenderViewObserver(render_view) {
  Reset();
}

WebKitTestRunner::~WebKitTestRunner() {
}

// WebTestDelegate  -----------------------------------------------------------

void WebKitTestRunner::clearContextMenuData() {
  last_context_menu_data_.reset();
}

WebContextMenuData* WebKitTestRunner::lastContextMenuData() const {
  return last_context_menu_data_.get();
}

void WebKitTestRunner::clearEditCommand() {
  render_view()->ClearEditCommands();
}

void WebKitTestRunner::setEditCommand(const std::string& name,
                                      const std::string& value) {
  render_view()->SetEditCommandForNextKeyEvent(name, value);
}

void WebKitTestRunner::fillSpellingSuggestionList(
    const WebString& word, WebVector<WebString>* suggestions) {
  if (word == WebString::fromUTF8("wellcome")) {
      WebVector<WebString> result(suggestions->size() + 1);
      for (size_t i = 0; i < suggestions->size(); ++i)
        result[i] = (*suggestions)[i];
      result[suggestions->size()] = WebString::fromUTF8("welcome");
      suggestions->swap(result);
  }
}

void WebKitTestRunner::setGamepadData(const WebGamepads& gamepads) {
  SetMockGamepads(gamepads);
}

void WebKitTestRunner::printMessage(const std::string& message) {
  Send(new ShellViewHostMsg_PrintMessage(routing_id(), message));
}

void WebKitTestRunner::postTask(WebTask* task) {
  Platform::current()->callOnMainThread(InvokeTaskHelper, task);
}

void WebKitTestRunner::postDelayedTask(WebTask* task, long long ms) {
  MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&WebTask::run, base::Owned(task)),
      base::TimeDelta::FromMilliseconds(ms));
}

WebString WebKitTestRunner::registerIsolatedFileSystem(
    const WebKit::WebVector<WebKit::WebString>& absolute_filenames) {
  std::vector<base::FilePath> files;
  for (size_t i = 0; i < absolute_filenames.size(); ++i)
    files.push_back(webkit_base::WebStringToFilePath(absolute_filenames[i]));
  std::string filesystem_id;
  Send(new ShellViewHostMsg_RegisterIsolatedFileSystem(
      routing_id(), files, &filesystem_id));
  return WebString::fromUTF8(filesystem_id);
}

long long WebKitTestRunner::getCurrentTimeInMillisecond() {
    return base::TimeTicks::Now().ToInternalValue() /
        base::Time::kMicrosecondsPerMillisecond;
}

WebString WebKitTestRunner::getAbsoluteWebStringFromUTF8Path(
    const std::string& utf8_path) {
#if defined(OS_WIN)
  base::FilePath path(UTF8ToWide(utf8_path));
#else
  base::FilePath path(base::SysWideToNativeMB(base::SysUTF8ToWide(utf8_path)));
#endif
  if (!path.IsAbsolute()) {
    GURL base_url =
        net::FilePathToFileURL(current_working_directory_.Append(
            FILE_PATH_LITERAL("foo")));
    net::FileURLToFilePath(base_url.Resolve(utf8_path), &path);
  }
  return webkit_base::FilePathToWebString(path);
}

WebURL WebKitTestRunner::localFileToDataURL(const WebURL& file_url) {
  base::FilePath local_path;
  if (!net::FileURLToFilePath(file_url, &local_path))
    return WebURL();

  std::string contents;
  Send(new ShellViewHostMsg_ReadFileToString(
        routing_id(), local_path, &contents));

  std::string contents_base64;
  if (!base::Base64Encode(contents, &contents_base64))
    return WebURL();

  const char data_url_prefix[] = "data:text/css:charset=utf-8;base64,";
  return WebURL(GURL(data_url_prefix + contents_base64));
}

WebURL WebKitTestRunner::rewriteLayoutTestsURL(const std::string& utf8_url) {
  const char kPrefix[] = "file:///tmp/LayoutTests/";
  const int kPrefixLen = arraysize(kPrefix) - 1;

  if (utf8_url.compare(0, kPrefixLen, kPrefix, kPrefixLen))
    return WebURL(GURL(utf8_url));

  base::FilePath replace_path =
      ShellRenderProcessObserver::GetInstance()->webkit_source_dir().Append(
          FILE_PATH_LITERAL("LayoutTests/"));
#if defined(OS_WIN)
  std::string utf8_path = WideToUTF8(replace_path.value());
#else
  std::string utf8_path =
      WideToUTF8(base::SysNativeMBToWide(replace_path.value()));
#endif
  std::string new_url =
      std::string("file://") + utf8_path + utf8_url.substr(kPrefixLen);
  return WebURL(GURL(new_url));
}

WebPreferences* WebKitTestRunner::preferences() {
  return &prefs_;
}

void WebKitTestRunner::applyPreferences() {
  webkit_glue::WebPreferences prefs = render_view()->GetWebkitPreferences();
  ExportLayoutTestSpecificPreferences(prefs_, &prefs);
  render_view()->SetWebkitPreferences(prefs);
  Send(new ShellViewHostMsg_OverridePreferences(routing_id(), prefs));
}

std::string WebKitTestRunner::makeURLErrorDescription(
    const WebURLError& error) {
  std::string domain = error.domain.utf8();
  int code = error.reason;

  if (domain == net::kErrorDomain) {
    domain = "NSURLErrorDomain";
    switch (error.reason) {
    case net::ERR_ABORTED:
      code = -999;  // NSURLErrorCancelled
      break;
    case net::ERR_UNSAFE_PORT:
      // Our unsafe port checking happens at the network stack level, but we
      // make this translation here to match the behavior of stock WebKit.
      domain = "WebKitErrorDomain";
      code = 103;
      break;
    case net::ERR_ADDRESS_INVALID:
    case net::ERR_ADDRESS_UNREACHABLE:
    case net::ERR_NETWORK_ACCESS_DENIED:
      code = -1004;  // NSURLErrorCannotConnectToHost
      break;
    }
  } else {
    DLOG(WARNING) << "Unknown error domain";
  }

  return base::StringPrintf("<NSError domain %s, code %d, failing URL \"%s\">",
      domain.c_str(), code, error.unreachableURL.spec().data());
}

// RenderViewObserver  --------------------------------------------------------

void WebKitTestRunner::DidClearWindowObject(WebFrame* frame) {
  ShellRenderProcessObserver::GetInstance()->BindTestRunnersToWindow(frame);
}

void WebKitTestRunner::DidFinishLoad(WebFrame* frame) {
  if (!frame->parent()) {
    if (!wait_until_done_)
      test_is_running_ = false;
    load_finished_ = true;
    Send(new ShellViewHostMsg_DidFinishLoad(routing_id()));
  }
}

void WebKitTestRunner::DidRequestShowContextMenu(
    WebFrame* frame,
    const WebContextMenuData& data) {
  last_context_menu_data_.reset(new WebContextMenuData(data));
}

bool WebKitTestRunner::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebKitTestRunner, message)
    IPC_MESSAGE_HANDLER(ShellViewMsg_CaptureTextDump, OnCaptureTextDump)
    IPC_MESSAGE_HANDLER(ShellViewMsg_CaptureImageDump, OnCaptureImageDump)
    IPC_MESSAGE_HANDLER(ShellViewMsg_SetCurrentWorkingDirectory,
                        OnSetCurrentWorkingDirectory)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

// Public methods - -----------------------------------------------------------

void WebKitTestRunner::NotifyDone() {
  if (load_finished_)
    test_is_running_ = false;
  else
    wait_until_done_ = false;
  Send(new ShellViewHostMsg_NotifyDone(routing_id()));
}

void WebKitTestRunner::DumpAsText() {
  Send(new ShellViewHostMsg_DumpAsText(routing_id()));
}

void WebKitTestRunner::DumpChildFramesAsText() {
  Send(new ShellViewHostMsg_DumpChildFramesAsText(routing_id()));
}

void WebKitTestRunner::WaitUntilDone() {
  wait_until_done_ = true;
  Send(new ShellViewHostMsg_WaitUntilDone(routing_id()));
}

void WebKitTestRunner::OverridePreference(const std::string& key,
                                          v8::Local<v8::Value> value) {
  if (key == "WebKitDefaultFontSize") {
    prefs_.defaultFontSize = value->Int32Value();
  } else if (key == "WebKitMinimumFontSize") {
    prefs_.minimumFontSize = value->Int32Value();
  } else if (key == "WebKitDefaultTextEncodingName") {
    prefs_.defaultTextEncodingName =
        WebString::fromUTF8(std::string(*v8::String::AsciiValue(value)));
  } else if (key == "WebKitJavaScriptEnabled") {
    prefs_.javaScriptEnabled = value->BooleanValue();
  } else if (key == "WebKitSupportsMultipleWindows") {
    prefs_.supportsMultipleWindows = value->BooleanValue();
  } else if (key == "WebKitDisplayImagesKey") {
    prefs_.loadsImagesAutomatically = value->BooleanValue();
  } else if (key == "WebKitPluginsEnabled") {
    prefs_.pluginsEnabled = value->BooleanValue();
  } else if (key == "WebKitJavaEnabled") {
    prefs_.javaEnabled = value->BooleanValue();
  } else if (key == "WebKitUsesPageCachePreferenceKey") {
    prefs_.usesPageCache = value->BooleanValue();
  } else if (key == "WebKitPageCacheSupportsPluginsPreferenceKey") {
    prefs_.pageCacheSupportsPlugins = value->BooleanValue();
  } else if (key == "WebKitOfflineWebApplicationCacheEnabled") {
    prefs_.offlineWebApplicationCacheEnabled = value->BooleanValue();
  } else if (key == "WebKitTabToLinksPreferenceKey") {
    prefs_.tabsToLinks = value->BooleanValue();
  } else if (key == "WebKitWebGLEnabled") {
    prefs_.experimentalWebGLEnabled = value->BooleanValue();
  } else if (key == "WebKitCSSRegionsEnabled") {
    prefs_.experimentalCSSRegionsEnabled = value->BooleanValue();
  } else if (key == "WebKitCSSGridLayoutEnabled") {
    prefs_.experimentalCSSGridLayoutEnabled = value->BooleanValue();
  } else if (key == "WebKitHyperlinkAuditingEnabled") {
    prefs_.hyperlinkAuditingEnabled = value->BooleanValue();
  } else if (key == "WebKitEnableCaretBrowsing") {
    prefs_.caretBrowsingEnabled = value->BooleanValue();
  } else if (key == "WebKitAllowDisplayingInsecureContent") {
    prefs_.allowDisplayOfInsecureContent = value->BooleanValue();
  } else if (key == "WebKitAllowRunningInsecureContent") {
    prefs_.allowRunningOfInsecureContent = value->BooleanValue();
  } else if (key == "WebKitCSSCustomFilterEnabled") {
    prefs_.cssCustomFilterEnabled = value->BooleanValue();
  } else if (key == "WebKitShouldRespectImageOrientation") {
    prefs_.shouldRespectImageOrientation = value->BooleanValue();
  } else if (key == "WebKitWebAudioEnabled") {
    DCHECK(value->BooleanValue());
  } else {
    std::string message("CONSOLE MESSAGE: Invalid name for preference: ");
    printMessage(message + key + "\n");
  }
  applyPreferences();
}

void WebKitTestRunner::NotImplemented(const std::string& object,
                                      const std::string& method) {
  Send(new ShellViewHostMsg_NotImplemented(routing_id(), object, method));
}

void WebKitTestRunner::Reset() {
  prefs_.reset();
  webkit_glue::WebPreferences prefs = render_view()->GetWebkitPreferences();
  ExportLayoutTestSpecificPreferences(prefs_, &prefs);
  render_view()->SetWebkitPreferences(prefs);
  dump_editing_callbacks_ = false;
  dump_frame_load_callbacks_ = false;
  dump_user_gesture_in_frame_load_callbacks_ = false;
  stop_provisional_frame_loads_ = false;
  dump_title_changes_ = false;
  dump_resource_load_callbacks_ = false;
  dump_resource_request_callbacks_ = false;
  dump_resource_response_mime_types_ = false;
  dump_create_view_ = false;
  can_open_windows_ = false;
  test_is_running_ = true;
  load_finished_ = false;
  wait_until_done_ = false;
}

// Private methods  -----------------------------------------------------------

void WebKitTestRunner::OnCaptureTextDump(bool as_text,
                                         bool printing,
                                         bool recursive) {
  WebFrame* frame = render_view()->GetWebView()->mainFrame();
  std::string dump;
  if (as_text) {
    dump = DumpFramesAsText(frame, printing, recursive);
  } else {
    WebFrame::RenderAsTextControls render_text_behavior =
        WebFrame::RenderAsTextNormal;
    if (printing)
      render_text_behavior |= WebFrame::RenderAsTextPrinting;
    dump = frame->renderTreeAsText(render_text_behavior).utf8();
    dump.append(DumpFrameScrollPosition(frame, recursive));
  }
  Send(new ShellViewHostMsg_TextDump(routing_id(), dump));
}

void WebKitTestRunner::OnCaptureImageDump(
    const std::string& expected_pixel_hash) {
  SkBitmap snapshot;
  PaintInvalidatedRegion();
  CopyCanvasToBitmap(GetCanvas(), &snapshot);

  SkAutoLockPixels snapshot_lock(snapshot);
  base::MD5Digest digest;
#if defined(OS_ANDROID)
  // On Android, pixel layout is RGBA, however, other Chrome platforms use BGRA.
  const uint8_t* raw_pixels =
      reinterpret_cast<const uint8_t*>(snapshot.getPixels());
  size_t snapshot_size = snapshot.getSize();
  scoped_array<uint8_t> reordered_pixels(new uint8_t[snapshot_size]);
  for (size_t i = 0; i < snapshot_size; i += 4) {
    reordered_pixels[i] = raw_pixels[i + 2];
    reordered_pixels[i + 1] = raw_pixels[i + 1];
    reordered_pixels[i + 2] = raw_pixels[i];
    reordered_pixels[i + 3] = raw_pixels[i + 3];
  }
  base::MD5Sum(reordered_pixels.get(), snapshot_size, &digest);
#else
  base::MD5Sum(snapshot.getPixels(), snapshot.getSize(), &digest);
#endif
  std::string actual_pixel_hash = base::MD5DigestToBase16(digest);

  if (actual_pixel_hash == expected_pixel_hash) {
    SkBitmap empty_image;
    Send(new ShellViewHostMsg_ImageDump(
        routing_id(), actual_pixel_hash, empty_image));
    return;
  }
  Send(new ShellViewHostMsg_ImageDump(
      routing_id(), actual_pixel_hash, snapshot));
}

void WebKitTestRunner::OnSetCurrentWorkingDirectory(
    const base::FilePath& current_working_directory) {
  current_working_directory_ = current_working_directory;
  std::vector<base::FilePath::StringType> components;
  current_working_directory_.GetComponents(&components);
  for (unsigned i = 0; i < components.size(); ++i) {
    if (components[i] == FILE_PATH_LITERAL("loading"))
      dump_frame_load_callbacks_ = true;
  }
}

SkCanvas* WebKitTestRunner::GetCanvas() {
  WebView* view = render_view()->GetWebView();
  const WebSize& size = view->size();
  float device_scale_factor = view->deviceScaleFactor();
  int width = std::ceil(device_scale_factor * size.width);
  int height = std::ceil(device_scale_factor * size.height);

  if (canvas_ &&
      canvas_->getDeviceSize().width() == width &&
      canvas_->getDeviceSize().height() == height) {
    return canvas_.get();
  }
  canvas_.reset(skia::CreatePlatformCanvas(
      size.width, size.height, true, 0, skia::RETURN_NULL_ON_FAILURE));
  return canvas_.get();
}

void WebKitTestRunner::PaintRect(const WebRect& rect) {
  WebView* view = render_view()->GetWebView();
  float device_scale_factor = view->deviceScaleFactor();
  int scaled_x = device_scale_factor * rect.x;
  int scaled_y = device_scale_factor * rect.y;
  int scaled_width = std::ceil(device_scale_factor * rect.width);
  int scaled_height = std::ceil(device_scale_factor * rect.height);
  // TODO(jochen): Verify that the scaling is correct once the HiDPI tests
  // actually work.
  WebRect device_rect(scaled_x, scaled_y, scaled_width, scaled_height);
  view->paint(webkit_glue::ToWebCanvas(GetCanvas()), device_rect);
}

void WebKitTestRunner::PaintInvalidatedRegion() {
  WebView* view = render_view()->GetWebView();
  view->animate(0.0);
  view->layout();
  const WebSize& widget_size = view->size();
  WebRect client_rect(0, 0, widget_size.width, widget_size.height);

  // Paint the canvas if necessary. Allow painting to generate extra rects
  // for the first two calls. This is necessary because some WebCore rendering
  // objects update their layout only when painted.
  for (int i = 0; i < 3; ++i) {
    // Make sure that paint_rect is always inside the RenderView's visible
    // area.
    WebRect paint_rect = proxy_->paintRect();
    int left = std::max(paint_rect.x, client_rect.x);
    int top = std::max(paint_rect.y, client_rect.y);
    int right = std::min(paint_rect.x + paint_rect.width,
                         client_rect.x + client_rect.width);
    int bottom = std::min(paint_rect.y + paint_rect.height,
                          client_rect.y + client_rect.height);
    WebRect rect;
    if (left < right && top < bottom)
      rect = WebRect(left, top, right - left, bottom - top);
    proxy_->setPaintRect(WebRect());
    if (rect.isEmpty())
      continue;
    PaintRect(rect);
  }
  CHECK(proxy_->paintRect().isEmpty());
}

void WebKitTestRunner::DisplayRepaintMask() {
  GetCanvas()->drawARGB(167, 0, 0, 0);
}

}  // namespace content

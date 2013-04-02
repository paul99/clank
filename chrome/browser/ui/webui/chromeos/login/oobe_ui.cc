// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"

#include <string>

#include "ash/ash_switches.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "chrome/browser/browser_about_handler.h"
#include "chrome/browser/chromeos/kiosk_mode/kiosk_mode_settings.h"
#include "chrome/browser/chromeos/login/base_login_display_host.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_screen_actor.h"
#include "chrome/browser/chromeos/login/screen_locker.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/about_ui.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/enterprise_oauth_enrollment_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/error_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/eula_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_dropdown_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_state_informer.h"
#include "chrome/browser/ui/webui/chromeos/login/reset_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/terms_of_service_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/update_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/user_image_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/wrong_hwid_screen_handler.h"
#include "chrome/browser/ui/webui/options/chromeos/user_image_source.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "grit/browser_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/webui/jstemplate_builder.h"
#include "ui/webui/web_ui_util.h"

using content::WebContents;

namespace {

// Path for a stripped down login page that does not have OOBE elements.
const char kLoginPath[] = "login";

// Path for the enterprise enrollment gaia page hosting.
const char kEnterpriseEnrollmentGaiaLoginPath[] = "gaialogin";

}  // namespace

namespace chromeos {

class OobeUIHTMLSource : public content::URLDataSource {
 public:
  explicit OobeUIHTMLSource(DictionaryValue* localized_strings);

  // content::URLDataSource implementation.
  virtual std::string GetSource() OVERRIDE;
  virtual void StartDataRequest(
      const std::string& path,
      bool is_incognito,
      const content::URLDataSource::GotDataCallback& callback) OVERRIDE;
  virtual std::string GetMimeType(const std::string&) const OVERRIDE {
    return "text/html";
  }
  virtual bool ShouldAddContentSecurityPolicy() const OVERRIDE {
    return false;
  }

 private:
  virtual ~OobeUIHTMLSource() {}

  std::string GetDataResource(int resource_id) const;

  scoped_ptr<DictionaryValue> localized_strings_;
  DISALLOW_COPY_AND_ASSIGN(OobeUIHTMLSource);
};

// OobeUIHTMLSource -------------------------------------------------------

OobeUIHTMLSource::OobeUIHTMLSource(DictionaryValue* localized_strings)
    : localized_strings_(localized_strings) {
}

std::string OobeUIHTMLSource::GetSource() {
  return chrome::kChromeUIOobeHost;
}

void OobeUIHTMLSource::StartDataRequest(
    const std::string& path,
    bool is_incognito,
    const content::URLDataSource::GotDataCallback& callback) {
  if (UserManager::Get()->IsUserLoggedIn() &&
      !UserManager::Get()->IsLoggedInAsStub() &&
      !ScreenLocker::default_screen_locker()) {
    scoped_refptr<base::RefCountedBytes> empty_bytes =
        new base::RefCountedBytes();
    callback.Run(empty_bytes);
    return;
  }

  std::string response;
  if (chromeos::KioskModeSettings::Get()->IsKioskModeEnabled())
    response = GetDataResource(IDR_DEMO_USER_LOGIN_HTML);
  else if (path.empty())
    response = GetDataResource(IDR_OOBE_HTML);
  else if (path == kLoginPath)
    response = GetDataResource(IDR_LOGIN_HTML);
  else if (path == kEnterpriseEnrollmentGaiaLoginPath)
    response = GetDataResource(IDR_GAIA_LOGIN_HTML);

  callback.Run(base::RefCountedString::TakeString(&response));
}

std::string OobeUIHTMLSource::GetDataResource(int resource_id) const {
  const base::StringPiece html(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          resource_id));
  return webui::GetI18nTemplateHtml(html, localized_strings_.get());
}

// OobeUI ----------------------------------------------------------------------

// static
const char OobeUI::kScreenOobeNetwork[]         = "connect";
const char OobeUI::kScreenOobeEula[]            = "eula";
const char OobeUI::kScreenOobeUpdate[]          = "update";
const char OobeUI::kScreenOobeEnrollment[]      = "oauth-enrollment";
const char OobeUI::kScreenGaiaSignin[]          = "gaia-signin";
const char OobeUI::kScreenAccountPicker[]       = "account-picker";
const char OobeUI::kScreenErrorMessage[]        = "error-message";
const char OobeUI::kScreenUserImagePicker[]     = "user-image";
const char OobeUI::kScreenTpmError[]            = "tpm-error-message";
const char OobeUI::kScreenPasswordChanged[]     = "password-changed";
const char OobeUI::kScreenManagedUserCreation[] = "managed-user-creation";
const char OobeUI::kScreenTermsOfService[]      = "terms-of-service";
const char OobeUI::kScreenWrongHWID[]           = "wrong-hwid";

OobeUI::OobeUI(content::WebUI* web_ui)
    : WebUIController(web_ui),
      update_screen_actor_(NULL),
      network_screen_actor_(NULL),
      eula_screen_actor_(NULL),
      reset_screen_actor_(NULL),
      wrong_hwid_screen_actor_(NULL),
      error_screen_handler_(NULL),
      signin_screen_handler_(NULL),
      terms_of_service_screen_actor_(NULL),
      user_image_screen_actor_(NULL),
      current_screen_(SCREEN_UNKNOWN) {
  InitializeScreenMaps();

  network_state_informer_ = new NetworkStateInformer();
  network_state_informer_->Init();

  core_handler_ = new CoreOobeHandler(this);
  AddScreenHandler(core_handler_);
  core_handler_->SetDelegate(this);

  AddScreenHandler(new NetworkDropdownHandler);

  NetworkScreenHandler* network_screen_handler = new NetworkScreenHandler();
  network_screen_actor_ = network_screen_handler;
  AddScreenHandler(network_screen_handler);

  EulaScreenHandler* eula_screen_handler = new EulaScreenHandler();
  eula_screen_actor_ = eula_screen_handler;
  AddScreenHandler(eula_screen_handler);

  ResetScreenHandler* reset_screen_handler = new ResetScreenHandler();
  reset_screen_actor_ = reset_screen_handler;
  AddScreenHandler(reset_screen_handler);

  WrongHWIDScreenHandler* wrong_hwid_screen_handler =
      new WrongHWIDScreenHandler();
  wrong_hwid_screen_actor_ = wrong_hwid_screen_handler;
  AddScreenHandler(wrong_hwid_screen_handler);

  UpdateScreenHandler* update_screen_handler = new UpdateScreenHandler();
  update_screen_actor_ = update_screen_handler;
  AddScreenHandler(update_screen_handler);

  EnterpriseOAuthEnrollmentScreenHandler*
      enterprise_oauth_enrollment_screen_handler =
          new EnterpriseOAuthEnrollmentScreenHandler;
  enterprise_enrollment_screen_actor_ =
      enterprise_oauth_enrollment_screen_handler;
  AddScreenHandler(enterprise_oauth_enrollment_screen_handler);

  TermsOfServiceScreenHandler* terms_of_service_screen_handler =
      new TermsOfServiceScreenHandler;
  terms_of_service_screen_actor_ = terms_of_service_screen_handler;
  AddScreenHandler(terms_of_service_screen_handler);

  UserImageScreenHandler* user_image_screen_handler =
      new UserImageScreenHandler();
  user_image_screen_actor_ = user_image_screen_handler;
  AddScreenHandler(user_image_screen_handler);

  error_screen_handler_ = new ErrorScreenHandler(network_state_informer_);
  AddScreenHandler(error_screen_handler_);

  signin_screen_handler_ = new SigninScreenHandler(network_state_informer_,
                                                   error_screen_handler_);
  AddScreenHandler(signin_screen_handler_);

  network_state_informer_->SetDelegate(signin_screen_handler_);

  DictionaryValue* localized_strings = new DictionaryValue();
  GetLocalizedStrings(localized_strings);

  Profile* profile = Profile::FromWebUI(web_ui);
  // Set up the chrome://theme/ source, for Chrome logo.
  ThemeSource* theme = new ThemeSource(profile);
  content::URLDataSource::Add(profile, theme);

  // Set up the chrome://terms/ data source, for EULA content.
  AboutUIHTMLSource* about_source =
      new AboutUIHTMLSource(chrome::kChromeUITermsHost, profile);
  content::URLDataSource::Add(profile, about_source);

  // Set up the chrome://oobe/ source.
  OobeUIHTMLSource* html_source = new OobeUIHTMLSource(localized_strings);
  content::URLDataSource::Add(profile, html_source);

  // Set up the chrome://userimage/ source.
  options::UserImageSource* user_image_source =
      new options::UserImageSource();
  content::URLDataSource::Add(profile, user_image_source);
}

OobeUI::~OobeUI() {
  core_handler_->SetDelegate(NULL);
}

void OobeUI::ShowScreen(WizardScreen* screen) {
  screen->Show();
}

void OobeUI::HideScreen(WizardScreen* screen) {
  screen->Hide();
}

UpdateScreenActor* OobeUI::GetUpdateScreenActor() {
  return update_screen_actor_;
}

NetworkScreenActor* OobeUI::GetNetworkScreenActor() {
  return network_screen_actor_;
}

EulaScreenActor* OobeUI::GetEulaScreenActor() {
  return eula_screen_actor_;
}

EnterpriseEnrollmentScreenActor* OobeUI::
    GetEnterpriseEnrollmentScreenActor() {
  return enterprise_enrollment_screen_actor_;
}

ResetScreenActor* OobeUI::GetResetScreenActor() {
  return reset_screen_actor_;
}

TermsOfServiceScreenActor* OobeUI::GetTermsOfServiceScreenActor() {
  return terms_of_service_screen_actor_;
}

WrongHWIDScreenActor* OobeUI::GetWrongHWIDScreenActor() {
  return wrong_hwid_screen_actor_;
}

UserImageScreenActor* OobeUI::GetUserImageScreenActor() {
  return user_image_screen_actor_;
}

ViewScreenDelegate* OobeUI::GetRegistrationScreenActor() {
  NOTIMPLEMENTED();
  return NULL;
}

ViewScreenDelegate* OobeUI::GetHTMLPageScreenActor() {
  // WebUI implementation of the LoginDisplayHost opens HTML page directly,
  // without opening OOBE page.
  NOTREACHED();
  return NULL;
}

void OobeUI::GetLocalizedStrings(base::DictionaryValue* localized_strings) {
  // Note, handlers_[0] is a GenericHandler used by the WebUI.
  for (size_t i = 0; i < handlers_.size(); ++i)
    handlers_[i]->GetLocalizedStrings(localized_strings);
  webui::SetFontAndTextDirection(localized_strings);

#if defined(GOOGLE_CHROME_BUILD)
  localized_strings->SetString("buildType", "chrome");
#else
  localized_strings->SetString("buildType", "chromium");
#endif

  if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kDisableNewOobe))
    localized_strings->SetString("oobeType", "new");
  else
    localized_strings->SetString("oobeType", "old");

  if (CommandLine::ForCurrentProcess()->
          HasSwitch(ash::switches::kAshDisableNewLockAnimations))
    localized_strings->SetString("lockAnimationsType", "old");
  else
    localized_strings->SetString("lockAnimationsType", "new");

  // If we're not doing boot animation then WebUI should trigger
  // wallpaper load on boot.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableBootAnimation)) {
    localized_strings->SetString("bootIntoWallpaper", "on");
  } else {
    localized_strings->SetString("bootIntoWallpaper", "off");
  }

  // OobeUI is used for OOBE/login and lock screen.
  if (BaseLoginDisplayHost::default_host())
    localized_strings->SetString("screenType", "login");
  else
    localized_strings->SetString("screenType", "lock");
}

void OobeUI::InitializeScreenMaps() {
  screen_names_.resize(SCREEN_UNKNOWN);
  screen_names_[SCREEN_OOBE_NETWORK] = kScreenOobeNetwork;
  screen_names_[SCREEN_OOBE_EULA] = kScreenOobeEula;
  screen_names_[SCREEN_OOBE_UPDATE] = kScreenOobeUpdate;
  screen_names_[SCREEN_OOBE_ENROLLMENT] = kScreenOobeEnrollment;
  screen_names_[SCREEN_GAIA_SIGNIN] = kScreenGaiaSignin;
  screen_names_[SCREEN_ACCOUNT_PICKER] = kScreenAccountPicker;
  screen_names_[SCREEN_ERROR_MESSAGE] = kScreenErrorMessage;
  screen_names_[SCREEN_USER_IMAGE_PICKER] = kScreenUserImagePicker;
  screen_names_[SCREEN_TPM_ERROR] = kScreenTpmError;
  screen_names_[SCREEN_PASSWORD_CHANGED] = kScreenPasswordChanged;
  screen_names_[SCREEN_CREATE_MANAGED_USER] = kScreenManagedUserCreation;
  screen_names_[SCREEN_TERMS_OF_SERVICE] = kScreenTermsOfService;
  screen_names_[SCREEN_WRONG_HWID] = kScreenWrongHWID;

  screen_ids_.clear();
  for (size_t i = 0; i < screen_names_.size(); ++i)
    screen_ids_[screen_names_[i]] = static_cast<Screen>(i);
}

void OobeUI::AddScreenHandler(BaseScreenHandler* handler) {
  web_ui()->AddMessageHandler(handler);
  handlers_.push_back(handler);
}

void OobeUI::InitializeHandlers() {
  for (size_t i = 0; i < handlers_.size(); ++i)
    handlers_[i]->InitializeBase();
}

void OobeUI::ShowOobeUI(bool show) {
  core_handler_->ShowOobeUI(show);
}

void OobeUI::ShowRetailModeLoginSpinner() {
  signin_screen_handler_->ShowRetailModeLoginSpinner();
}

void OobeUI::ShowSigninScreen(SigninScreenHandlerDelegate* delegate,
                              NativeWindowDelegate* native_window_delegate) {
  signin_screen_handler_->SetDelegate(delegate);
  signin_screen_handler_->SetNativeWindowDelegate(native_window_delegate);
  error_screen_handler_->SetNativeWindowDelegate(native_window_delegate);

  signin_screen_handler_->Show(core_handler_->show_oobe_ui());
}

void OobeUI::ResetSigninScreenHandlerDelegate() {
  signin_screen_handler_->SetDelegate(NULL);
  signin_screen_handler_->SetNativeWindowDelegate(NULL);
  error_screen_handler_->SetNativeWindowDelegate(NULL);
}

const std::string& OobeUI::GetScreenName(Screen screen) const {
  DCHECK(screen >= 0 && screen < SCREEN_UNKNOWN);
  return screen_names_[static_cast<size_t>(screen)];
}

void OobeUI::OnCurrentScreenChanged(const std::string& screen) {
  if (screen_ids_.count(screen)) {
    current_screen_ = screen_ids_[screen];
  } else {
    NOTREACHED() << "Screen should be registered in InitializeScreenMaps()";
    current_screen_ = SCREEN_UNKNOWN;
  }
}

}  // namespace chromeos

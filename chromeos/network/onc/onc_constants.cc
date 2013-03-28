// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_constants.h"

namespace chromeos {

// Constants for ONC properties.
namespace onc {

// Top Level ONC
const char kCertificates[] = "Certificates";
const char kEncryptedConfiguration[] = "EncryptedConfiguration";
const char kNetworkConfigurations[] = "NetworkConfigurations";
const char kUnencryptedConfiguration[] = "UnencryptedConfiguration";
const char kNetworkConfiguration[] = "NetworkConfiguration";

// Common keys/values.
const char kRecommended[] = "Recommended";
const char kRemove[] = "Remove";

// Network Configuration
const char kCellular[] = "Cellular";
const char kEthernet[] = "Ethernet";
const char kGUID[] = "GUID";
const char kIPConfigs[] = "IPConfigs";
const char kName[] = "Name";
const char kNameServers[] = "NameServers";
const char kProxySettings[] = "ProxySettings";
const char kSearchDomains[] = "SearchDomains";
const char kType[] = "Type";
const char kVPN[] = "VPN";
const char kWiFi[] = "WiFi";

namespace ethernet {
const char kAuthentication[] = "Authentication";
const char kEAP[] = "EAP";
const char kNone[] = "None";
const char k8021X[] = "8021X";
}  // namespace ethernet

namespace ipconfig {
const char kGateway[] = "Gateway";
const char kIPAddress[] = "IPAddress";
const char kIPv4[] = "IPv4";
const char kIPv6[] = "IPv6";
const char kRoutingPrefix[] = "RoutingPrefix";
const char kType[] = "Type";
}  // namespace ipconfig

namespace wifi {
const char kAutoConnect[] = "AutoConnect";
const char kEAP[] = "EAP";
const char kHiddenSSID[] = "HiddenSSID";
const char kNone[] = "None";
const char kPassphrase[] = "Passphrase";
const char kProxyURL[] = "ProxyURL";
const char kSSID[] = "SSID";
const char kSecurity[] = "Security";
const char kWEP_PSK[] = "WEP-PSK";
const char kWEP_8021X[] = "WEP-8021X";
const char kWPA_PSK[] = "WPA-PSK";
const char kWPA_EAP[] = "WPA-EAP";
}  // namespace wifi

namespace certificate {
const char kAuthority[] = "Authority";
const char kClient[] = "Client";
const char kCommonName[] = "CommonName";
const char kEmailAddress[] = "EmailAddress";
const char kEnrollmentURI[] = "EnrollmentURI";
const char kIssuerCARef[] = "IssuerCARef";
const char kIssuer[] = "Issuer";
const char kLocality[] = "Locality";
const char kNone[] = "None";
const char kOrganization[] = "Organization";
const char kOrganizationalUnit[] = "OrganizationalUnit";
const char kPKCS12[] = "PKCS12";
const char kPattern[] = "Pattern";
const char kRef[] = "Ref";
const char kServer[] = "Server";
const char kSubject[] = "Subject";
const char kTrust[] = "Trust";
const char kType[] = "Type";
const char kWeb[] = "Web";
const char kX509[] = "X509";
}  // namespace certificate

namespace encrypted {
const char kAES256[] = "AES256";
const char kCipher[] = "Cipher";
const char kCiphertext[] = "Ciphertext";
const char kHMACMethod[] = "HMACMethod";
const char kHMAC[] = "HMAC";
const char kIV[] = "IV";
const char kIterations[] = "Iterations";
const char kPBKDF2[] = "PBKDF2";
const char kSHA1[] = "SHA1";
const char kSalt[] = "Salt";
const char kStretch[] = "Stretch";
}  // namespace encrypted

namespace eap {
const char kAnonymousIdentity[] = "AnonymousIdentity";
const char kAutomatic[] = "Automatic";
const char kClientCertPattern[] = "ClientCertPattern";
const char kClientCertRef[] = "ClientCertRef";
const char kClientCertType[] = "ClientCertType";
const char kEAP_AKA[] = "EAP-AKA";
const char kEAP_FAST[] = "EAP-FAST";
const char kEAP_SIM[] = "EAP-SIM";
const char kEAP_TLS[] = "EAP-TLS";
const char kEAP_TTLS[] = "EAP-TTLS";
const char kIdentity[] = "Identity";
const char kInner[] = "Inner";
const char kLEAP[] = "LEAP";
const char kMD5[] = "MD5";
const char kMSCHAPv2[] = "MSCHAPv2";
const char kOuter[] = "Outer";
const char kPAP[] = "PAP";
const char kPEAP[] = "PEAP";
const char kPassword[] = "Password";
const char kSaveCredentials[] = "SaveCredentials";
const char kServerCARef[] = "ServerCARef";
const char kUseSystemCAs[] = "UseSystemCAs";
}  // namespace eap

namespace vpn {
const char kAuthNoCache[] = "AuthNoCache";
const char kAuthRetry[] = "AuthRetry";
const char kAuth[] = "Auth";
const char kAuthenticationType[] = "AuthenticationType";
const char kCert[] = "Cert";
const char kCipher[] = "Cipher";
const char kClientCertPattern[] = "ClientCertPattern";
const char kClientCertRef[] = "ClientCertRef";
const char kClientCertType[] = "ClientCertType";
const char kCompLZO[] = "CompLZO";
const char kCompNoAdapt[] = "CompNoAdapt";
const char kEAP[] = "EAP";
const char kGroup[] = "Group";
const char kHost[] = "Host";
const char kIKEVersion[] = "IKEVersion";
const char kIPsec[] = "IPsec";
const char kKeyDirection[] = "KeyDirection";
const char kL2TP[] = "L2TP";
const char kNsCertType[] = "NsCertType";
const char kOpenVPN[] = "OpenVPN";
const char kPSK[] = "PSK";
const char kPassword[] = "Password";
const char kPort[] = "Port";
const char kProto[] = "Proto";
const char kPushPeerInfo[] = "PushPeerInfo";
const char kRemoteCertEKU[] = "RemoteCertEKU";
const char kRemoteCertKU[] = "RemoteCertKU";
const char kRemoteCertTLS[] = "RemoteCertTLS";
const char kRenegSec[] = "RenegSec";
const char kSaveCredentials[] = "SaveCredentials";
const char kServerCARef[] = "ServerCARef";
const char kServerCertRef[] = "ServerCertRef";
const char kServerPollTimeout[] = "ServerPollTimeout";
const char kShaper[] = "Shaper";
const char kStaticChallenge[] = "StaticChallenge";
const char kTLSAuthContents[] = "TLSAuthContents";
const char kTLSRemote[] = "TLSRemote";
const char kTypeL2TP_IPsec[] = "L2TP-IPsec";
const char kType[] = "Type";
const char kUsername[] = "Username";
const char kVerb[] = "Verb";
const char kXAUTH[] = "XAUTH";
}  // namespace vpn

namespace openvpn {
const char kNone[] = "none";
const char kInteract[] = "interact";
const char kNoInteract[] = "nointeract";
const char kServer[] = "server";
}  // namespace openvpn

namespace proxy {
const char kDirect[] = "Direct";
const char kExcludeDomains[] = "ExcludeDomains";
const char kFtp[] = "FTPProxy";
const char kHost[] = "Host";
const char kHttp[] = "HTTPProxy";
const char kHttps[] = "SecureHTTPProxy";
const char kManual[] = "Manual";
const char kPAC[] = "PAC";
const char kPort[] = "Port";
const char kSocks[] = "SOCKS";
const char kType[] = "Type";
const char kWPAD[] = "WPAD";
}  // namespace proxy

namespace substitutes {
const char kLoginIDField[] = "${LOGIN_ID}";
const char kEmailField[] = "${LOGIN_EMAIL}";
}  // namespace substitutes

}  // namespace onc

}  // namespace chromeos

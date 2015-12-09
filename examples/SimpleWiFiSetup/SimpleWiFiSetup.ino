#include <ESP8266WiFiSetup.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

void setup() {
  Serial.begin(115200);

  //WiFiSetup
  ESP8266WiFiSetup setup;
  //setup.setDebugOutput(true);
  //setup.clearSettings(); //Ensures that we create an AP after each reset. Useful for testing.
  setup.begin(); //Automatically generate an SSID for the AP, NO password.
  //setup.begin("WiFiSetup"); //Set SSID for the AP, NO password.
  //setup.begin("WiFiSetup", "password"); //Set SSID and password for the AP. Note the password must be 8 or more chars!

  Serial.println("Connected, yay");
}

void loop() {
  // put your main code here, to run repeatedly:
}

/**************************************************************
 * ESP8266WiFiSetup is a library for the ESP8266/Arduino platform
 * (https://github.com/esp8266/Arduino) to enable easy
 * configuration and reconfiguration of WiFi credentials.
 * inspired by AlexT https://github.com/tzapu
 * Built by Peter Kristensen
 * Licensed under MIT license
 **************************************************************/

#ifndef ESP8266WiFiSetup_h
#define ESP8266WiFiSetup_h

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

const static char ICACHE_FLASH_ATTR WIFI_SETUP_HTML[] = R"xxx(
  <!DOCTYPE html>
  <html lang='en'>
    <head>
      <meta name='viewport' content='width=device-width, initial-scale=1'/>
      <title>ESP Web Manager</title>
      <style>
        html{
          width:100%;
          height:100%;
          overflow-y:auto;
        }
        body{
          border:1px solid;
          width: 80%;
          height: 80%;
          margin-left: auto;
          margin-right: auto;
          overflow-y:auto;
        }
        button{
          background-color:rgba(0,0,0,0);
          color:#FFFFFF;
          border:none;
          cursor:pointer;
          padding-left:10px;
        }
        #btnConnect{
          margin-left:15px;
        }
        div#header {
          height: 60px;
          background-color: #1fa3ec;
          display: flex;
        }
        div#headerWrapper{
          width:100%;
          height: 100%;
          display: flex;
          justify-content: center;
        }
        div#divInputs{
          max-width:50%;
          width:50%;
          height: 100%;
          display:flex;
          flex-direction: column;
        }
        input{
          width: 100%;
          height: 50%;
          margin: 3px;
          padding-left:5px;
          background-color: hsla(201,84%,45%,1);
          border: none;
          color: #FFFFFF;
        }
        #divNetworkContainer{
          width:100%;
          display: flex;
          flex-direction: column;
        }
        .divNetwork{
          width:100%;
          font-size: 2.3vw;
          border-bottom:1px solid black;
          display: flex;
          margin-top:5px;
          margin-bottom:5px;
        }
        .divNetwork.top{
          border-bottom:2px solid black;
        }
        .divNetworkSSID{
          width: 60%;
        }
        .divNetworkEnc{
          width: 10%;
        }
        .divNetworkRSSI{
          width: 30%;
          text-align: right;
        }
        .low{
          color:red;
        }
        .medium{
          color:black;
        }
        .high{
          color:green;
        }
        *:focus {
          outline-width: 0;
        }
        ::-webkit-input-placeholder { /* WebKit, Blink, Edge */
          color: #FFFFFF;
        }
        :-moz-placeholder { /* Mozilla Firefox 4 to 18 */
          color: #FFFFFF;
        }
        ::-moz-placeholder { /* Mozilla Firefox 19+ */
          color: #FFFFFF;
        }
        :-ms-input-placeholder { /* Internet Explorer 10-11 */
          color: #FFFFFF;
        }
      </style>
      <script>
        function onClickSSID(l){
          console.log(l);
          document.getElementById('inputSSID').value = l.children[0].innerText || l.children[0].textContent;
          document.getElementById('inputPass').focus();
        }
        function addNetworks(networks){
          var result = '';
          result += '<div class="divNetwork top"><div class="divNetworkSSID">&nbsp;SSID</div><div class="divNetworkEnc">Encrypted</div><div class="divNetworkRSSI">Strength&nbsp;</div></div>';
          for(i = 0; i < networks.length; i++) {
            var percent, quality;
            if(parseInt(networks[i].rssi) <= -100){
              percent = 0;
            }else if(parseInt(networks[i].rssi) >= -50){
              percent = 100;
            }else{
              percent = 2 * (parseInt(networks[i].rssi) + 100);
            }
            if(percent >= 0 && percent < 33) quality = 'low';
            else if(percent >= 33 && percent < 66) quality ='medium';
            else quality = 'high'
            result += '<div class="divNetwork" onclick="onClickSSID(this)"><div class="divNetworkSSID">' + networks[i].ssid + '</div><div class="divNetworkEnc">' + networks[i].encrypted + '</div><div class="divNetworkRSSI ' + quality + '">' + percent + '%&nbsp;</div></div>';
          }
          document.getElementById('divNetworkContainer').innerHTML = result;
        }
        function scan(){
          var dots = 0;
          var id = setInterval(function(){
            document.getElementById('divNetworkContainer').innerHTML = 'Scanning';
            for(var i = 0; i < dots; i++){
              document.getElementById('divNetworkContainer').innerHTML += '.';
            }
            dots = (dots + 1) % 5;
          },200);
          var xmlhttp = new XMLHttpRequest();
          xmlhttp.onreadystatechange = function() {
            if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
              clearInterval(id);
              console.log(JSON.parse(xmlhttp.responseText));
              addNetworks(JSON.parse(xmlhttp.responseText));
            }
          }
          xmlhttp.open('GET', 'getNetworks', true);
          xmlhttp.send();
        }
        function connect(){
          var xmlhttp = new XMLHttpRequest();
          xmlhttp.onreadystatechange = function() {
            if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
              console.log(JSON.parse(xmlhttp.responseText));
              var response = JSON.parse(xmlhttp.responseText).response;
              if(response === 'ok') alert('Connecting...');
              else alert('Something went wrong. Try again...');
            }
          }
          var ssid = document.getElementById('inputSSID').value;
          var pass = document.getElementById('inputPass').value;
          xmlhttp.open('GET', 'connect?s=' + ssid + '&p=' + pass, true);
          xmlhttp.send();
        }
      </script>
    </head>
    <body onload='scan()'>
      <div id='header'>
        <div id='headerWrapper'>
          <div id='divInputs'>
            <input id='inputSSID' placeholder='SSID'>
            <input id='inputPass' placeholder='Pass'>
          </div>
          <button id='btnConnect' onclick='connect()'>Connect</button>
          <button id='btnScan' onclick='scan()'>Scan</button>
        </div>
      </div>
      <div id='divNetworkContainer'></div>
    </body>
  </html>
)xxx";

class ESP8266WiFiSetup
{
public:
    ESP8266WiFiSetup();

    boolean begin();
    boolean begin(char const *ssid);
    boolean begin(char const *ssid, char const *pass); //Password must be atleast 8 characters!

    void clearSettings();

    void setTimeout(unsigned long seconds);
    void setDebugOutput(boolean debug);

    //sets a custom ip /gateway /subnet configuration
    void    setAPConfig(IPAddress ip, IPAddress gw, IPAddress sn);

private:
    DNSServer dnsServer;
    ESP8266WebServer server;

    // DNS server
    const byte DNS_PORT = 53;


    boolean tryToConnect;
    boolean _debug = false;
    String _ssid = "";
    String _pass = "";
    unsigned long timeout = 0;
    unsigned long start = 0;
    IPAddress _ip;
    IPAddress _gw;
    IPAddress _sn;

    void setupAP(const char*, const char*);

    // WebServer
    String getWifiNetworks();
    void handleRoot();
    void handleGetNetworks();
    void handleConnect();
    void handleNotFound();
    void handle204();
    boolean captivePortal();

    template <typename Generic>
    void DEBUG_PRINT(Generic text);
    String urldecode(const char*);
    boolean isIp(String str);

};
#endif

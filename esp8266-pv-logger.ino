#include <ESP8266WiFi.h>
#include <Soladin.h>

#define dsInterval 15000
#define SSID "wifissid" //put your wifi ssid here
#define PASS "wifipass" //put your wifi key here
#define APIKEY "abcdefgh" //put your thingspeak api key here

char targetHost[] = "api.thingspeak.com";
//char debugHost[] = "192.168.1.2"; //If you want debugging enable this line and any other debug line below. Make sure to put the ip of your debugging host here. On the debug host run(linux): nc -k -l 3000
#define debugPort 3000

Soladin sol;
boolean solConnect = false;
unsigned long dataSent = millis();
byte retries = 0;
boolean successQ = false;

WiFiClient wifiClient;

void setup() {
  Serial.begin(9600);
  
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);  
  }     
 // debug(wifiClient, debugHost, debugPort, "wifi connected");
  
  sol.begin(&Serial);  
 // debug(wifiClient, debugHost, debugPort, "Sol begin done");
}

void loop() {
  retries = 0;
  while ((! solConnect) && (retries <  6)){
    solConnect = sol.query(PRB);    
    delay(1000*retries);
    retries++;
  }
  
  if ((dataSent + dsInterval < millis()) && (WiFi.status() == WL_CONNECTED) && solConnect) {
//    debug(wifiClient, debugHost, debugPort, "Time to send data"); 
    successQ = false;
    retries = 0;
    
    while ((! successQ) && (retries < 6)) {      
      successQ = sol.query(DVS);
      if (! successQ) {
        delay(1000*retries);
        retries++;
      }            
    }
       
    if (! successQ) {
      solConnect = false;
      delay(15000);
    }
    
    if (solConnect) {
 //     debug(wifiClient, debugHost, debugPort, "Sol connected");
      String payload = thingSpeakPayload(float(sol.PVamp)/100, sol.DeviceTemp, float(sol.Totalpower)/100, sol.Gridpower, sol.Gridvolt, sol.PVvolt/10);
 //     debug(wifiClient, debugHost, debugPort, payload);      
      if (thingSpeakSend(wifiClient, targetHost, 80, APIKEY, payload)) {;
      dataSent = millis();
      }
    }
  }
  
  delay(1000*retries);
  
  while(wifiClient.available()){
      wifiClient.readStringUntil('\r');
  }
}

bool thingSpeakSend(WiFiClient& wfClient, char *hostname, unsigned int port, String apiKey, String payLoad)
{
  bool retValue = false;

  if (wfClient.connect(hostname, port))
  { 
     String postHead = "POST /update HTTP/1.1\nHost: " + String(hostname) + "\nConnection: close\nX-THINGSPEAKAPIKEY: " + apiKey + "\nContent-Type: application/x-www-form-urlencoded\nContent-Length: " + payLoad.length() + "\n\n";
     String httpPost=postHead + payLoad;
        
    wfClient.print(httpPost);
    yield();    
    retValue = true;
  }
  
  return retValue;
}

String thingSpeakPayload(float amp, float invTemp, float kwhTotal, float power, float volt, float pvVolt)
{
  return "&field1=" + String(amp) + "&field2=" + String(invTemp) + "&field3=" + String(kwhTotal) + "&field4=" + String(power) + "&field5=" + String(volt) + "&field6=" + String(pvVolt);
}

void debug(WiFiClient& wfClient, char *hostname, unsigned int port, String message) {
  if (wfClient.connect(hostname, port)) {
    wfClient.println(message);
    yield();
  }
}

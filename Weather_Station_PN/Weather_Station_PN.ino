#include <Wire.h>
#include "DHT.h"
#include "Adafruit_VEML6070.h"
#include <ESP8266WiFiMulti.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

unsigned long g_startMillis;
unsigned long g_switchTimeMillis;
boolean       g_heaterInHighPhase;

const char* host       = "pubsub.pubnub.com";
const char* pubKey     = "<your-pubnub-publish-key>";
const char* subKey     = "<your-pubnub-subscribe-key>";
const char* channel    = "my_channel";
String      timeToken    = "0";
DynamicJsonBuffer jsonBufPWR;
const char PWR_DATA[] = "{\"pwr\":\" \"}";
const char w_data[] = "{\"WS\":{\"battery_lvl\":\" \", \"charging\":\" \", \"charge_done\":\" \", \"water_lvl\":\" \", \"UV\":\" \", \"temp\":\" \", \"hum\":\" \"} }";

const char LEDON[] = "{\"data\":{\"SW_res\":\"on\"}}";
const char* ssid = "PWS_Wifi";
const char* passphrase = "1234";
String st;
String content;
int statusCode;
String url;
#define MY_SIZE 200
#define update_interval 100
struct pair {int x; double y;} arr[MY_SIZE];

bool DEMO = false;

int x[MY_SIZE];
double y[MY_SIZE];
const long get_temp_interval = 2000; 
unsigned long previousMillis = 0; 
float t = 0.00;
float h = 0.00;
float hic = 0.00;
WiFiClient client;
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);

#define DHTPIN D3     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

int charging = D6;
int chrg_done = D7;

Adafruit_VEML6070 uv = Adafruit_VEML6070();
DHT dht(DHTPIN, DHTTYPE);



bool testWifi(void) {
  int c = 0;
  String pleasewait = "Please wait";
  Serial.println("Waiting for Wifi to connect");


   
  while ( c < 20 ) {
    if (WiFiMulti.run() == WL_CONNECTED) { return true; } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
    pleasewait += ".";
  
  }
  //Mode = 0;
  Serial.println("");
  Serial.println("Unable to connect");
  Serial.println("Please go to 192.168.4.1");
  //display.drawString(0, 16, "192.168.4.1");
  Serial.println("to configure Network"); 
    
     
  //Serial.println("Connect timed out, opening AP");
  return false;
} 


void createWebServer(int webtype,String my_SSID)
{
  my_SSID = my_SSID; 
  if ( webtype == 1 ) {
    server.on("/", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html> <h1>Portable Weather Station</h1> ";
        content += "<h3>Configuration Panel</h3>";
        content += "<h5>Available Wifi networks</h5>";
        //content += ipStr;
        content += "<p>";
        content += st;
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' type='password' length=64><input type='submit'></form>";
        content += "</html>";
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");

          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              Serial.print("Wrote: ");
              Serial.print(qsid[i]); 
            }
            Serial.println("");
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              Serial.print("Wrote: ");
              Serial.println(qpass[i]); 
            }    
          EEPROM.commit();
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<h2>Configuration saved. Reset the device to connect to your local router.</h2></html>";
          statusCode = 200;
        } else {
          content = "Error\":\"404 not found\"";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        server.send(statusCode, "text/html", content);
        delay(2000);
        //ESP.reset();
        //system_restart();
    });
  }
}

void launchWeb(int webtype, String my_SSID) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  //setupAP();
  createWebServer(webtype, my_SSID);
  // Start the server
  server.begin();
  Serial.println("Server started"); 
}




void setupAP(int type) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase);
  Serial.println("softap");
  if (type == 1){
    launchWeb(1, ssid );
  }
  Serial.println("over");
}

void WIFI_finder_setup(){
  EEPROM.begin(512);
  delay(10);
  setupAP(0);
  
  for (int i=0; i< MY_SIZE; i++){
       x[i] = i;
       y[i] = 0;
  }

    for (int i=0; i < MY_SIZE; i++ ){
    arr[i].x = x[i];
    arr[i].y = y[i]; 
   
    }

  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid.c_str());
  Serial.println("Reading EEPROM pass");
  String epass = "";
  

  
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass.c_str());  
  if ( esid.length() > 1 ) {
      WiFiMulti.addAP(esid.c_str(), epass.c_str());
      //WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        //Mode = 1;
        IPAddress ip = WiFi.localIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        Serial.print("Connected to ");
        Serial.println(ipStr);        
        launchWeb(1, esid);
        return;
      }else {
        //Mode = 0;
        //delay(5000);
        //display.clear();
        //Mode = 1; 
      }
  }
  
  setupAP(1); 
   
}

void send_PN(WiFiClient c, String msg, String ch){
          if (!c.connect(host, 80)) {
            Serial.println("Pubnub Connection Failed");
            return;
          }//else{
            //Serial.println("Pubnub Connection Success");       
          //}
          String url = "/publish/";
          url += pubKey;
          url += "/";
          url += subKey;
          url += "/0/";
          url += ch;
          url += "/0/";
          url += msg;
          url +=  "?uuid=weather_station";
          url +=  "&auth=WS";
          //Serial.println(url);
          
          c.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" + 
                     "Connection: close\r\n\r\n");
          delay(50); 
          
          //while(c.available()){
          //  String line = c.readStringUntil('\r');
          //  Serial.print(line); 
          //}
          //Serial.print("Send:" + msg);  
}

int soil(){
  // read the sensor on analog A0:
  int sensorReading = analogRead(A0);
  // map the sensor range (four options):
  // ex: 'long int map(long int, long int, long int, long int, long int)'
  int range = map(sensorReading, 0, 1024, 0, 100);
  return range;
}

void setup() {
  Serial.begin(115200);
  WIFI_finder_setup();
  uv.begin(VEML6070_1_T);  // pass in the integration time constant
  pinMode(charging, INPUT);
  pinMode(chrg_done, INPUT); 
}


void loop() {
  server.handleClient();
  int soil_lvl = soil();
  unsigned long currentMillis = millis();  
  if (currentMillis - previousMillis >= get_temp_interval) {
        // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        h = dht.readHumidity();
        // Read temperature as Celsius (the default)
        t = dht.readTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        //float f = dht.readTemperature(true);
      
        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(t)) {
          Serial.println("Failed to read from DHT sensor!");
          //return 0.0;
        }

      // Compute heat index in Fahrenheit (the default)
     // float hif = dht.computeHeatIndex(f, h);
      // Compute heat index in Celsius (isFahreheit = false)
      //hic = dht.computeHeatIndex(t, h, false);
              
      previousMillis = currentMillis;
  }   

  //Serial.print("water level:");
  //Serial.println(water_lvl);
  //Serial.print("temp:");
  //Serial.println(t);    
  //Serial.print("UV level:");
  //Serial.println(uv.readUV());
  
  int chrging = digitalRead(charging);
    int charge_done = digitalRead(chrg_done);
  // print out the state of the button:
  //Serial.print("Charging:");
  //Serial.print(chrging);
  //Serial.print(" Done:");
  //Serial.print(charge_done);  
  //Serial.print("  Analog:");
  //int val = analogRead(A0);
  //float volts = (val/1024)*5.0;
  //Serial.print(val);
  //int percentage = map(val, 0, 1023, 0, 100);
  //Serial.print(" - ");
  //Serial.println(percentage);
  
  delay(200);
  DynamicJsonBuffer jsonBuf; 
  JsonObject& root = jsonBuf.parseObject(w_data);       
  root[String("WS")][String("battery_lvl")] = ""; 
  root[String("WS")][String("charging")] = String(chrging);
  root[String("WS")][String("charge_done")] = String(charge_done);    
  root[String("WS")][String("soil")] = String(soil_lvl); 
  root[String("WS")][String("UV")] = String(uv.readUV());   
  root[String("WS")][String("temp")] = String(t);   
  root[String("WS")][String("hum")] = String(h);   
      
  String output;
  root.printTo(output);
  Serial.println(output);

  send_PN(client, output , "channel_chart");
}





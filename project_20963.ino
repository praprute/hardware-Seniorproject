#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>

AsyncWebServer server(80);

#define SSID_AP_NAME "Maket1"
#define SSID_AP_PASS NULL

#define DHTPIN 4
#define DHTTYPE DHT11

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x3f, lcdColumns, lcdRows);
#define LED 2
//input รับค่าจาก web 3 ตัว
const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";
//ตัวแปรทั้งหมด
String wifiname,password ,serialid ;
String wifinamefs,passwordfs ,serialfs ;
String serialidjson;
String haswifi,haspass;
//
//ชุดข้อมูลเกษตกร
StaticJsonDocument<100> wifidetail;
char buffer[100];
//
DHT dht(DHTPIN, DHTTYPE);

String wifistate;
String statu(const String& var){
  if(var == "STATE"){
    if(WiFi.status() == WL_CONNECTED){
      wifistate = "Connected";
    }
    else{
      wifistate = "Disconnected";
    } 
    Serial.print("WiFi Status : ");
    Serial.println(wifistate);
    return wifistate;
  }
  if(var == "SERIAID"){  
     File serial2 = SPIFFS.open("/serial.txt");
     while(serial2.available()){serialidjson = serial2.readString();}
     serial2.close();
     return serialidjson;
  }
  return String();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
 wifidetail["temp"] = "";
 wifidetail["moisture"] = "";
 wifidetail["product"] = ""; 

 lcd.init();                      
 lcd.backlight();

 Serial.begin(115200);
 pinMode(LED,OUTPUT);
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  //wifi เริ่มทำงาน แบบ hotspot
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SSID_AP_NAME, SSID_AP_PASS);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  //อ่านค่า wifi ที่เคยเข้าใช้งานไว้
  File wifi2 = SPIFFS.open("/wifi.txt");
  while(wifi2.available()){haswifi = wifi2.readString();}
  wifi2.close();
  File pass2 = SPIFFS.open("/pass.txt");
  while(pass2.available()){haspass = pass2.readString();}
  pass2.close();
  File serial2 = SPIFFS.open("/serial.txt");
  while(serial2.available()){serialidjson = serial2.readString();}
  serial2.close();
  wifidetail["product"] = serialidjson;
  
  //กรณียังไม่มีการเชื่อมต่อ wifi จะเชื่อมต่ออัตโนมัติไปที่ wifi ที่เคยเชื่อมต่อไว้
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin((const char*) haswifi.c_str(),(const char*) haspass.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {     WiFi.disconnect();
          Serial.print("Can't connect to ");
          Serial.print(haswifi);    
    }
    else
    {     Serial.print("Connect to ");
          Serial.print(haswifi); 
    }
  }

  //เมื่อได้รับคำสั่งที่ paht นั้นๆ
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, statu);
    Serial.println("!Client conect to homepage!");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/seria", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/seriaform.html", "text/html");
  });
  
  server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/wifiform.html", "text/html");
  });

  server.on("/disconnect", HTTP_GET, [](AsyncWebServerRequest *request){
    WiFi.disconnect();
    Serial.println("!Disonnect page!");
    request->send(SPIFFS, "/index.html", String(), false, statu);
  });

  server.on("/getwifi", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
        wifiname = request->getParam(PARAM_INPUT_1)->value();
        password = request->getParam(PARAM_INPUT_2)->value();  
      if(wifiname != NULL && password != NULL){ 
        WiFi.begin((const char*) wifiname.c_str(),(const char*) password.c_str());
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);Serial.println("");
          Serial.print(".");
          };           
        if (WiFi.status() == WL_CONNECTED) {
            wifinamefs = wifiname;
            passwordfs = password;     
            File wifi = SPIFFS.open("/wifi.txt", FILE_WRITE);
            wifi.print(wifinamefs);
            wifi.close();         
            File pass = SPIFFS.open("/pass.txt", FILE_WRITE);
            pass.print(passwordfs);
            pass.close();                       
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            request->send(200, "text/html",
            "Connected <br><a href=\"/\"><button>Return to homepage</button></a>"
            );                                                                      
        }
      }
      else {
        request->send(200, "text/html", "Plase put your wifi");
      }
    }
  });

  server.on("/getseria", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_3)) {
        serialid = request->getParam(PARAM_INPUT_3)->value();       
      if(serialid != NULL){ 
         serialfs = serialid;
         File serial = SPIFFS.open("/serial.txt", FILE_WRITE);
         serial.print(serialfs);
         serial.close();           
         File serial2 = SPIFFS.open("/serial.txt");
         while(serial2.available()){serialidjson = serial2.readString();}
         serial2.close();
         wifidetail["product"] = serialidjson;
         request->send(200, "text/html", 
         "Receive your id <br><a href=\"/\"><button>Return to homepage</button></a>");                                                                      
            }
      }
      else {
        request->send(200, "text/html", "Plase put your seriaid");
      }
  });
  server.onNotFound(notFound);
  server.begin();
  dht.begin(); 
}

void loop() {
    int i, count;
    for(i = 1; i < 128 ; i++){
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      float f = dht.readTemperature(true);
      Serial.println(i);
      if (isnan(h) || isnan(t) || isnan(f)) {
          Serial.println(F("Failed to read from DHT sensor!"));
          return;
      }
      float hif = dht.computeHeatIndex(f, h);
      float hic = dht.computeHeatIndex(t, h, false);
    
      lcd.setCursor(0, 0);
      lcd.print("TEMP. = ");
      lcd.print(t);
      lcd.print(" C ");
      lcd.setCursor(0, 1);
      lcd.print("MOIS. = ");
      lcd.print(h);
      lcd.print(" % ");

      wifidetail["temp"] = t;
      wifidetail["moisture"] = h;
      if ((WiFi.status() == WL_CONNECTED)) { 
        if(i == 2){
          HTTPClient http;   
          serializeJsonPretty(wifidetail, buffer);
          http.begin("http://128.199.97.98:8000/api/dht11/tempsensor");
          http.addHeader("Content-Type" , "application/json");
          int httpCode = http.POST(buffer);
          Serial.println(httpCode);
            if (httpCode != 200) {
            i=i-1;
            Serial.println(i);
          }
        }
        else{
            for(count = 1; count < 15 ; count++)
              {delay(1000);
              Serial.println(count);
              }
        } 
      }
      else{
        i=0;
        }
}
}

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "SoftwareSerial.h"
#include <PubSubClient.h>
#include <Wire.h>


#define DEVICE_TYPE 10012
#define FIRMWARE_VERSION "1.0.1"

char ssid[] = "beyond";      // your network SSID (name)
char pass[] = "Saber12312";  // your network password

int status  = WL_IDLE_STATUS;    // the Wifi radio's status
WiFiClient wifiClient;
PubSubClient client(wifiClient);  

char MqttServer[]     = "iiot.ideaschain.com.tw";  // new ideaschain dashboard MQTT server
int  MqttPort          = 1883;
char ClientId[]       = "DSI5168Test";            // MQTT client ID. it's better to use unique id.
char Username[]       = "CpmmjsKKn8BsHSj1Ks8D";    // device access token
char Password[]       = "CpmmjsKKn8BsHSj1Ks8D";    // no need password
char SubscribeTopic[] = "v1/devices/me/telemetry";  // Fixed topic. ***DO NOT MODIFY***
char PublishTopic[]   = "v1/devices/me/telemetry";  // Fixed topic. ***DO NOT MODIFY***
char SubscribeTopic2[] = "v1/devices/me/attributes";
char PublishTopic2[]   = "v1/devices/me/attributes";  // Fixed topic. ***DO NOT MODIFY***
char PublishPayload[]="{\"weight\":\"0\"}";

///
/// RGB LED
///
#include "ws2812b.h"
#define DIGITALPINNUMBER  10
#define NUM_LEDS  8
ws2812b Ledstrip = ws2812b(DIGITALPINNUMBER , NUM_LEDS);

//
// OLED 128x128
//
#include <U8g2lib.h>
U8G2_SSD1327_MIDAS_128X128_F_HW_I2C u8g2(U8G2_R0,  U8X8_PIN_NONE);


///
/// HX711重量模組
///
#include "HX711.h"
const int DT_PIN = 6;
const int SCK_PIN = 2;
const int SCALE_FACTOR = -20; //the propotional parameter,different from each person
HX711 Scale;

void reconnect();

void setup(void) {
  Serial.begin(9600);
 
  u8g2.begin();  
  u8g2.clear();
  u8g2.setFont(u8g2_font_ncenB08_tr); //set the font
  u8g2.firstPage();
  do {
     u8g2.setCursor(30 , 64);
     u8g2.print("connecting...");    
  } while ( u8g2.nextPage() );
  
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 4 seconds for connection:
    delay(4000);
  }
  
  u8g2.clear();
  u8g2.firstPage();
  do {
    u8g2.setCursor(17, 70);
    u8g2.print("connect successed!");    
  } while ( u8g2.nextPage() );
  
  client.setServer(MqttServer, MqttPort);   
  reconnect();
  
  client.setCallback(callback);
  if (client.connect(Username)){
    Serial.println("Connection has been established, well done");
  } 
  
  Scale.begin(DT_PIN, SCK_PIN);
  Scale.set_scale(SCALE_FACTOR);//set the propotional parameter 設定比例參數
  Scale.tare();//  zero weight sensor 
   
  delay(1000);
  u8g2.clear();
  u8g2.firstPage();
  do {
    u8g2.setCursor(5, 64);
    u8g2.print("enter weight , number"); 
  } while ( u8g2.nextPage() );
  Ledstrip.begin(); //RGB begin
}

float Measure_Weight = 0;
float Weight = 0;
float Number = 0;
float SumWeight = 0 ;
bool WhetherContinue = false;
bool Next = false ;
String Str = "\0";
unsigned long TimeNow = 0;
unsigned long TimeNow1 = 0;
unsigned long TimeNow2 = 0;
unsigned long TimeNow3 = 0;

void loop(void) {
  client.loop();
  while(WhetherContinue){ 
    if(millis()- TimeNow > 5){
      TimeNow = millis(); 
      client.loop();
    }
    if(millis()- TimeNow1 > 3000 && millis() > TimeNow1 ){
      Measure_Weight = Scale.get_units(10); //measure the weight
   // Serial.println(Measure_Weight);
      if(Measure_Weight < 0) Measure_Weight = 0 ;
      String a ="/0";
      Str = "total weight : ";
      Str = Str + String(Measure_Weight);
      u8g2.clear();
      u8g2.firstPage();
      do {
        u8g2.setCursor(10, 64);
        u8g2.print(Str);   //display the measure_weight message
      } while ( u8g2.nextPage() );
      TimeNow1 = 12000.0 + millis() ; //avoid collison with another time
      TimeNow3 = 12000.0 + millis() ; //avoid collison with another time
      TimeNow2 = millis(); 
    }
    if(millis()- TimeNow2 > 3000 && millis() > TimeNow2 ){
      String c = "\0";
      String b = "\0";
      /*
      Serial.println(b);
      Serial.println(SumWeight - Measure_Weight);
      Serial.println(Measure_Weight*0.05  );
      */
      if( fabs(SumWeight - Measure_Weight)> SumWeight*0.05 ){ //if measure_weight smaller or bigger than 0.95 time total_weight
        int need_number = fabs(SumWeight - Measure_Weight)/Weight ;
        if(need_number == 0)need_number = 1;
        ///
        ///the payload format to IdeasChain and MQTT Dash to get the relevant value
        ///
        b="{\"Measure_Weight\":\""+ String(Measure_Weight) +"\",\"weight\":\"" + String(Weight) + "\",\"number\":\"" + String(Number) + "\",\"Y/N\":\"N\",\"color\":\"#ff0000\",\"need\":\""+String(need_number)+"\"}";
        Serial.println(b);
        client.publish(PublishTopic, b.c_str());
        // Serial.println(need_number);
        Str = "need  ";
        Str  = Str + String(need_number);
        u8g2.clear();
        if(SumWeight - Measure_Weight < 0){ //measure_weight smaller or bigger than 0.95 total_weigh
          u8g2.firstPage();
          do {
            u8g2.setCursor(50, 40);
            u8g2.print("fail!!");
            u8g2.setCursor(46, 60);
            u8g2.print(Str); //display the elements you need to increase message
            u8g2.setCursor(25, 80);
            u8g2.print("more elements");   
          } while ( u8g2.nextPage() );
        }
        else{
          do {
            u8g2.setCursor(50, 40);
            u8g2.print("fail!!");
            u8g2.setCursor(46, 60);
            u8g2.print(Str);  //display the elements you need decrease message
            u8g2.setCursor(25, 80);
            u8g2.print("less elements");   
          } while ( u8g2.nextPage() );
        }
        for(int i = 0 ; i < 8 ; i++){
          Ledstrip.setPixelColor(i,255,0,0); //All_RGB_to_Red
        }
        Ledstrip.show();
      }
    
      else{
        ///
        ///the payload format to IdeasChain and MQTT Dash to get the relevant value
        ///
        c="{\"Measure_Weight\":\""+ String(Measure_Weight) +"\",\"weight\":\"" + String(Weight) + "\",\"number\":\"" + String(Number) + "\",\"Y/N\":\"Y\",\"color\":\"#00ff11\"}";
        client.publish(PublishTopic, c.c_str());
        Serial.println(c);
        u8g2.firstPage();
        do {
          u8g2.setCursor(35, 64);
          u8g2.print("success!!");
        } while ( u8g2.nextPage() );
    
        for(int i = 0 ; i < 8 ; i++){
          Ledstrip.setPixelColor(i,0,255,0); //All_RGB_to_Green
        }
        Ledstrip.show();
      }
      TimeNow1 = 12000.0 + millis() ; //avoid collison with another time
      TimeNow2 = 12000.0 + millis() ; //avoid collison with another time
      TimeNow3 = millis();
    }
    if(millis()- TimeNow3 > 3000 && millis() > TimeNow3 ){
      u8g2.clear();
      u8g2.firstPage();
      do {
        u8g2.setCursor(10, 64);
        u8g2.print("continue measuring");    
      } while ( u8g2.nextPage() );
      for(int i = 0 ; i < 8 ; i++){
        Ledstrip.setPixelColor(i,0,0,0);
      }
      Ledstrip.show();
    Next = true;
    TimeNow2 = 12000.0 + millis() ;  //avoid collison with another time
    TimeNow3 = 12000.0 + millis() ;  //avoid collison with another time
    TimeNow1 = millis(); 
    }
  }    
  ///
  /// set up to next goods (weight and number)
  ///  
  if(Next){
   u8g2.clear();
   u8g2.firstPage();
   do {
     u8g2.setCursor(5, 64);
     u8g2.print("enter weight , number");    
   } while ( u8g2.nextPage() );
   for(int i = 0 ; i < 8 ; i++){
     Ledstrip.setPixelColor(i,0,0,0); //stop RGB  
   }
   Ledstrip.show();
   Next = false;
  }
  if(!client.connected()){
     reconnect();
     delay(2000);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  ///
  /// cuz pulish to the same portal,using first word of playoad to determine what to do  
  ///
  if(char(payload[2])!= 'w'){
    if (char(payload[2])== 'c')WhetherContinue = false ;
    for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    }
    return;
  }
  /*
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  */
  int cnt = 0 ; 
  String weight_str ="";
  String number_str ="";
  ///
  ///get thw weight and number yoy sended
  ///
  for (int i = 0; i < length; i++) {
    // Serial.print((char)payload[i]);
    if((char)payload[i]=='\"') cnt ++ ;
    else if(cnt==7) weight_str = weight_str + (char)payload[i]; // position of two parameter
    else if(cnt==15) number_str = number_str + (char)payload[i];
  }
  Serial.println();
  Weight = atof(weight_str.c_str());
  Number = atof(number_str.c_str());
  SumWeight =  Weight * Number; //count the total weight
  u8g2.clear();
  u8g2.firstPage();
  do {
    u8g2.setCursor(20, 64);
    u8g2.print("start measuring");    
  } while ( u8g2.nextPage() );
  WhetherContinue = true; //set up continue measure
  delay(3000);
  TimeNow = millis(); //initialize time
  TimeNow1 = millis(); //initialize time
  TimeNow2 = millis()+10000; //avoid collison with another time
  TimeNow3 = millis()+10000; //avoid collison with another time
}


 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("\nAttempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ClientId,Username,Password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(PublishTopic2, PublishPayload);
      // ... and resubscribe
      client.subscribe(SubscribeTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

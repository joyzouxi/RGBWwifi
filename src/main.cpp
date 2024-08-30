#include <Arduino.h>
#include "WiFi.h"
#include "PubSubClient.h"
#include "Ticker.h"
#include <iostream>
#include <string>
#include <U8g2lib.h>

// U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 13, /* data=*/ 15);   // ESP32 Thing, HW I2C with pin remapping
U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 13, /* data=*/ 15);   // ESP32 Thing, HW I2C with pin remapping

const char *ssid = "slow_office_optout_nomap";   //wifi name
const char *password = "kfcih6StqqerT4L";       //wifi password
const char *mqtt_server = "47.91.203.178";       //Onenet IP address
const int port = 6002;
#define mqtt_pubid "16001520"                    //Product ID
#define mqtt_devid "161198193"                  //Device ID 1
#define mqtt_password "rgbw"                 //Device Password 1

WiFiClient espClient;                            //Create a WiFi client
PubSubClient client(espClient);                  //Create a PubSub client and pass the WiFi client in it
char msgJson[100];                                
unsigned short json_len = 0;
char msg_buf[200];
#define ONENET_TOPIC_PROP_POST "$sys/" mqtt_pubid "/" mqtt_devid "/dp/post/json"
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10       /* Time ESP32 will go to sleep (in seconds) */

#define R_PIN 27
#define G_PIN 25
#define B_PIN 32
#define W_PIN 26
#define Button1 17
#define Button2 18

int R_Color = 0; // 0~255
int B_Color = 0; // 0~255
int G_Color = 0; // 0~255
int hue = 0; // 0~360
int saturation = 100; // 0~255

//char dataTemplate[] = "{\"temp\":%.2f,\"humidity\":%.2f}"; //Data template
char dataTemplate[] = "{\"Hue\":%i,\"Saturation\":%i}"; //Data template
Ticker tim1;                                     //Timer use to upload data
//Function use to connect WiFi
void setupWifi(){  
  delay(10);  
  Serial.println("Connect WIFI");  
  WiFi.begin(ssid, password);  
  while (!WiFi.isConnected())  {
    Serial.print(".");    
    delay(500);  
    }  
    Serial.println("OK");  
    Serial.println("Wifi connect success");
}
//Recieve feedback
void callback(char *topic, byte *payload, unsigned int length){ 
  Serial.println("message rev:");
  //Serial.println(topic);
  int hueInt = 0;
  int satInt = 0;
     for (size_t i = 0; i < length; i++)  {    
       Serial.print((char)payload[i]);
         }  
         Serial.println();   
    if((char)payload[0] == '%') {
      for(size_t i = 1; i<length; i++){
        char c =(char)payload[i];
        int digit = c - '0';
        hueInt = hueInt*10 + digit;
      }
      Serial.println(hueInt);
      hue = hueInt;
    }
    else if((char)payload[0] == '&') {
      for(size_t i = 1; i<length; i++){
        char c =(char)payload[i];
        int digit = c - '0';
        satInt = satInt*10 + digit;
      }
      Serial.println(satInt);
      saturation = satInt;
    }
}
//Send data to Onenet
void sendHSV(){
      if (client.connected())
        {
          snprintf(msgJson, 40, dataTemplate, hue, saturation); //Put the data in the template and pass to msgJson
          json_len = strlen(msgJson);                    //length of msgJson
          msg_buf[0] = char(0x03);                       //buffer of the data sent first part is 3
          msg_buf[1] = char(json_len >> 8);              //Second part is the 8 MSB of the data
          msg_buf[2] = char(json_len & 0xff);            //Third part is 8 LSB of the data
          memcpy(msg_buf + 3, msgJson, strlen(msgJson)); //Forth part is the data in msgJson
          msg_buf[3 + strlen(msgJson)] = 0;              //Add a 0 at last
          Serial.print("public message client:");
          Serial.println(msgJson);
          client.publish("$dp", (uint8_t *)msg_buf, 3 + strlen(msgJson)); //Send data
        }
      }

//reconnect the client once it disconnect
void clientReconnect(){  while (!client.connected())
{    Serial.println("reconnect MQTT...");
    if (client.connect(mqtt_devid, mqtt_pubid, mqtt_password))    {
            Serial.println("connected");      
            client.subscribe("$sys/16001520/RGBW/cmd/request/#");   
            }    else    {      Serial.println("failed");
                  Serial.println(client.state());      
                  Serial.println("try again in 1 sec");      
                  delay(1000);    }  
                  }
                  }
void clientconnect1(){
    //Send the server of the Client connect to 
    client.connect(mqtt_devid, mqtt_pubid, mqtt_password);
    //determine wether it is connected  
    if (client.connected())  {    Serial.println("\nOneNet is connected to " mqtt_devid "!\n");
    }  
}


void HSVtoRGB(int h, int s)      
{
    if(h >= 360) h = 0;
    
    float f = (float)s/255.0 * 100.0;
    s = int(f);
    if(s >= 100) s = 100;
    s = 100-s;

    int i;
    i = h / 60;
    int difs = h % 60; 
    float RGB_max = 255.0f;
    float RGB_min = RGB_max * (100 - s) / 100.0f;
    float RGB_Adj = (RGB_max - RGB_min) * difs / 60.0f; 
    switch(i)
    {
    case 0:
        R_Color = RGB_max;
        G_Color = RGB_min + RGB_Adj;
        B_Color = RGB_min;
        break;

    case 1:
        R_Color = RGB_max - RGB_Adj;
        G_Color = RGB_max;
        B_Color = RGB_min;
        break;

    case 2:
        R_Color = RGB_min;
        G_Color = RGB_max;
        B_Color = RGB_min + RGB_Adj;
        break;

    case 3:
        R_Color = RGB_min;
        G_Color = RGB_max - RGB_Adj;
        B_Color = RGB_max;
        break;

    case 4:
        R_Color = RGB_min + RGB_Adj;
        G_Color = RGB_min;
        B_Color = RGB_max;
        break;

    default:
        R_Color = RGB_max;
        G_Color = RGB_min;
        B_Color = RGB_max - RGB_Adj;
        break;
    }
}

void button_press(){
  if(digitalRead(Button1)==0){
    hue++;
    Serial.println("hue++");
    if(hue>=360){
      hue=0;
    }
  }
  if(digitalRead(Button2)==0){
    saturation++;
    Serial.println("saturation++");
    if(saturation>=255){
      saturation=0;
    }
  }
  
}
void setup() {
    Serial.begin(115200);
    Serial.println("start");

    u8g2.begin();

    pinMode(R_PIN,OUTPUT);
    pinMode(G_PIN,OUTPUT);
    pinMode(B_PIN,OUTPUT);
    pinMode(W_PIN,OUTPUT);
    pinMode(Button1,INPUT_PULLUP);
    pinMode(Button2,INPUT_PULLUP);

//Setting up WIFI and onenet
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr(2,20,"Setting up WIFI");
        u8g2.setFont(u8g2_font_helvR08_tr);
        u8g2.drawStr(2,40,ssid);
    } while ( u8g2.nextPage() );
    setupWifi(); 
    client.setServer(mqtt_server, port);
    char pid_str[20];
    sprintf(pid_str,"PID: %s",mqtt_pubid);
    char did_str[20];
    sprintf(did_str,"DID: %s",mqtt_devid);
    u8g2.firstPage();
    do {
        // u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.setFont(u8g2_font_helvR10_tr);
        u8g2.drawStr(2,20,"Connecting onenet");
        u8g2.drawStr(2,40,pid_str);
        u8g2.drawStr(2,60,did_str);

    } while ( u8g2.nextPage() );
    clientconnect1();
    client.setCallback(callback);
    tim1.attach(5, sendHSV);

}
 
void loop() {
          //  Check if WiFi connected  
    if (!WiFi.isConnected())  
      {    
        u8g2.firstPage();
        do {
            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.drawStr(2,20,"Setting up WIFI");
            u8g2.setFont(u8g2_font_helvR08_tr);
            u8g2.drawStr(2,40,ssid);
        } while ( u8g2.nextPage() );

        setupWifi(); 
         }  if (!client.connected()) //If client does not connect to Onenet reconnect
         {    
          char pid_str[20];
          sprintf(pid_str,"PID: %s",mqtt_pubid);
          char did_str[20];
          sprintf(did_str,"DID: %s",mqtt_devid);
          u8g2.firstPage();
          do {
              u8g2.setFont(u8g2_font_helvR10_tr);
              u8g2.drawStr(2,20,"Connecting onenet");
              u8g2.drawStr(2,40,pid_str);
              u8g2.drawStr(2,60,did_str);

          } while ( u8g2.nextPage() );
          clientReconnect();    
         //delay(500);  
         }
    client.loop();

    button_press();

    char hue_str[20];
    sprintf(hue_str,"Hue: %i",hue);
    char sat_str[20];
    sprintf(sat_str,"Saturation: %i",saturation);
    char Color_str[20];
    sprintf(Color_str,"R:%i|G:%i|B:%i",R_Color,G_Color,B_Color);
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr(2,20,hue_str);
        u8g2.drawStr(2,40,sat_str);
        u8g2.setFont(u8g2_font_helvR08_tf);
        u8g2.drawStr(2,60,Color_str);

    } while ( u8g2.nextPage() );

    HSVtoRGB(hue,saturation);
    analogWrite(G_PIN,G_Color-saturation/2);
    analogWrite(W_PIN,saturation);
    analogWrite(R_PIN,R_Color-saturation/2);
    analogWrite(B_PIN,B_Color-saturation/2);
    Serial.print(R_Color);
    Serial.print(", ");
    Serial.print(G_Color);
    Serial.print(", ");
    Serial.print(B_Color);
    Serial.print(", ");
    Serial.println(saturation);
}
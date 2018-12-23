#include <U8glib.h>
#include <Rtc_Pcf8563.h>
#include <ESP8266.h>
#include "audio.h"
#include <Wire.h>
#include <EEPROM.h>
#define INTERVAL_Time 1000
#define setFont_L u8g.setFont(u8g_font_timB18)
#define setFont_M u8g.setFont(u8g_font_timB14)
#define setFont_S u8g.setFont(u8g_font_timB10)
#define buzzer_pin 10
#include "userDef.h"
#include "oled.h"
#include <Microduino_Key.h>
DigitalKey KeyButton(keyPinn);
int micValue=0;
bool isRoar=false;

unsigned long Time_millis = millis();
unsigned long OLEDShowTime = millis();
int timeHH, timeMM, timeSS;
int year,month,day;
Rtc_Pcf8563 rtc;

#ifdef ESP32
#error "This code is not recommended to run on the ESP32 platform! Please check your Tools->Board setting."
#endif
/**
**CoreUSB UART Port: [Serial1] [D0,D1]
**Core+ UART Port: [Serial1] [D2,D3]
**/
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1284P__) || defined (__AVR_ATmega644P__) || defined(__AVR_ATmega128RFA1__)
#define EspSerial Serial1
#define UARTSPEED  115200
#endif
/**
**Core UART Port: [SoftSerial] [D2,D3]
**/
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
#include <SoftwareSerial.h>
//SoftwareSerial mySerial(2, 3); /* RX:D2, TX:D3 */
#define EspSerial mySerial
#define UARTSPEED  9600
#endif
#define SSID        "Honor 10"
#define PASSWORD    "zmf1124072183"
#define HOST_NAME   F("api.heclouds.com")
#define HOST_PORT   (80)
ESP8266 wifi(&EspSerial);

bool volChange=false,statusChange=false,idChange=false;
int music_status=0,temp_music_status=0;
int music_vol=10,temp_music_vol=10;
int music_num_MAX=14;
int current_music=1,temp_current_music=1;
static const byte  GETDATA[]  PROGMEM = {
  "GET https://api.heclouds.com/devices/503139172/datapoints?datastream_id=id,status,vol&limit=1 HTTP/1.1\r\napi-key: ZA0V=3BEajfyhX59diDNA=qHSfs=\r\nHost: api.heclouds.com\r\nConnection: close\r\n\r\n"
};
bool isConnected=false;
bool bottomBar=true;
bool canPlay=false;

void setup() {
  Serial.begin(115200);
  pinMode(buzzer_pin,OUTPUT);
  pinMode(micPin,INPUT);
  initTime();
  
  //wifi连接
  while (!Serial); // wait for Leonardo enumeration, others continue immediately
  Serial.println(F("WIFI start"));
  delay(100);
  WifiInit(EspSerial, UARTSPEED);
  wifi.setOprToStationSoftAP();
  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.println(F("WIFI Connect!"));
    isConnected=true;
  } else {
    isConnected=false;
    Serial.println(F("NO WIFI"));
  }
  wifi.disableMUX();
  
  //设置TF卡读取音乐
  audio_init(DEVICE_TF,MODE_One_END,music_vol);  
}

void loop() {
  updateMic();
  updateTime(); //更新时间
  updateAlarm();  //判断是否到闹钟时间
  db=getDB();//获得分贝数
  analyticDB(db);//分析分贝
  speakerDoing(isAlaram);//蜂鸣器处理
  updateButton();//按键检测
  updateOLED();//刷新OLED
  //wifi确认连接
  if(isConnected){
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
      Serial.print(F("TCP\n"));
      isConnected=true;
    } else {
      Serial.print(F("No TCP\n"));
      isConnected=true;
    }
    wifi.sendFromFlash(GETDATA, sizeof(GETDATA)); 
    //获取数据播放音乐
    networkHandle();
    mp3Handle();
    u8g.firstPage();
    do {
      drawAll();
    } while (u8g.nextPage());
  }
  else
  {
    u8g.firstPage();
    do{
      drawNotConnected();
    }while(u8g.nextPage());
    delay(5000);
    setup();
  }
}

void drawPlay(){
  u8g.drawBox(4,4,8,16);
  u8g.drawBox(20,4,8,16);
  u8g.setPrintPos(32,16);
  u8g.print("Playing");
}
void drawPause(){
  u8g.drawTriangle(4,4,30,8,4,16);
  u8g.setPrintPos(32,16);
  u8g.print("Paused");
}

void drawVol(){
  u8g.drawLine(4,35,4+30*4,35);
  u8g.drawLine(4+30*4,35,4+30*4,35-30*0.6);
  u8g.drawLine(4,35,4+30*4,35-30*0.6);
  u8g.drawTriangle(4,35,4+music_vol*4,35,4+music_vol*4,35-music_vol*0.6);
  u8g.setPrintPos(80,16);
  u8g.print(temp_current_music+1);
}
void drawNotConnected(){
  //it will draw like this:✡ (NO BORDER!!)
  u8g.drawTriangle(64,4,32,48,96,48);
  u8g.drawTriangle(64,60,32,16,96,16);
}
//处理接收数据
bool networkHandle() {
  //do something with net work ,include handle response message.
  canPlay=true;
  uint8_t buffer[415]={0};
  uint32_t len = wifi.recv(buffer, sizeof(buffer), 2000);
  if (len > 0) {
    for (uint32_t i = 0; i < len; i++) {
      Serial.print((char)buffer[i]);
    }
  }
//the ram of the device is too limited,so i enhered the length of response message,at specific index,there are nessicity value.
//272，273 vol
//344 id
//414 status
  temp_music_vol=((int)buffer[272]-48)*10+((int)buffer[273]-48)-10;
  if((int)buffer[345]-125==0)  temp_current_music=(int)buffer[344]-48;
  else {temp_current_music=((int)buffer[344]-48)*10+((int)buffer[345]-48);}
  temp_music_status=(int)buffer[414]-48;
  //Serial.println("***************");
  //Serial.println((int)buffer[345]);
  Serial.println(temp_music_vol);
  Serial.println(temp_current_music);
  Serial.println(temp_music_status);  
  Serial.println("");  
  Serial.println(""); 
  wifi.releaseTCP();
}
//播放音乐
void mp3Handle(){
  if(canPlay){
    if(current_music!=temp_current_music){
      idChange=true;
      current_music=temp_current_music;
    }
    if(music_vol!=temp_music_vol){
      volChange=true;
      music_vol=temp_music_vol;
    }
    if(music_status!=temp_music_status){
      statusChange=true;
      music_status=temp_music_status;
    }
    if(statusChange){
      if(music_status==1){
        audio_play();
      }else{
        audio_pause();
      }
    }
    if(volChange){
      audio_vol(music_vol);
    }
    if(idChange){
      audio_choose(current_music+1);
      audio_play();
    }
    volChange=false;
    idChange=false;
    statusChange=false;
  }
}
void drawAll(){
//包括分贝、播放状态、当前歌曲、音量大小
    u8g.setPrintPos(5,60);
    u8g.setFont(u8g_font_9x15);
    u8g.print("DB");
    u8g.setPrintPos(44, 60);
    //u8g.print(db);
    u8g.print(db);
    u8g.setFont(u8g_font_7x13);
    if(music_status==0){
      drawPause();
    }else{
      drawPlay();
    }
    drawVol();
}

void updateOLED() {
  //OLED display
  if (OLEDShowTime1 > millis()) OLEDShowTime1 = millis();
  if(millis()-OLEDShowTime1>INTERVAL_OLED) {
    //OLEDShow(); //调用显示库
    u8g.firstPage();
    do {
      //包括分贝、播放状态、当前歌曲、音量大小
      u8g.setPrintPos(5,60);
      u8g.setFont(u8g_font_9x15);
      u8g.print("DB");
      u8g.setPrintPos(44, 60);
      //u8g.print(db);
      u8g.print(db);
      u8g.setFont(u8g_font_7x13);
      if(music_status==0){
        drawPause();
      }else{
        drawPlay();
      }
      drawVol();
    } while( u8g.nextPage() );
    OLEDShowTime1 = millis();
  } 
}

void buzzer() {
  if (millis() - timer > 10) {
    if (!add) {
      i++;
      if (i >= 800)
        add = true;
    } else {
      i--;
      if (i <= 200) {
        add = false;
      }
    }
    tone(buzzerPin, i);
    timer = millis();
  }
}

//转换分贝值
double getDB() {
  int voice_data = analogRead(micPin);
  voice_data=map(voice_data,0,1023,0,20);
  db = (20. * log(10)) * (voice_data / 5.0);
  if(db>recodeDB) {
    recodeDB=db;
  }
  Serial.print("DB:");
  Serial.println(db);
  return db;
}

void analyticDB(double db) {
  if(db > voice&&!(timeHH==1&&timeMM<2)) {
    numNoise++; //记录一段时间内超过噪声阈值的次数
    //Serial.println(numNoise);
  }
  if (analytic_time > millis()) analytic_time = millis();
  if (millis() - analytic_time > INTERVALOLED) {
    if(numNoise>maxNoise) {
        i = 200;
        isAlaram= true;
    }
//     Serial.print(numNoise);
//    Serial.print("\t");
//    Serial.println(maxNoise);
    numNoise=0;
    analytic_time = millis();
  }
}

void updateButton() {
    Serial.println("****************************");
    if(KeyButton.readEvent()==SHORT_PRESS) {
      delay(15);
      recodeDB=0;
      isAlaram = false;
    }
}

void speakerDoing(boolean isAlaram) {
  if (isAlaram) {
    buzzer();
  } else {
    if(!(timeHH==1&&timeMM<2)) noTone(buzzerPin);
  }
}

void initTime() {
  //设置初始时间
  rtc.initClock();
  //set a time to start with.
  //day, weekday, month, century(1=1900, 0=2000), year(0-99)
  rtc.setDate(23, 7, 12, 0, 18);
  //hr, min, sec
  rtc.setTime(0, 59, 0);
}


void getCurrentTime() {

  rtc.formatDate();
  rtc.formatTime();

  timeHH=rtc.getHour();
  timeMM=rtc.getMinute();
  timeSS=rtc.getSecond();
  year=2000+rtc.getYear();
  month=rtc.getMonth();
  day=rtc.getDay();}

void updateTime() {
  //update GPS and UT during INTERVAL_GPS
  if (Time_millis > millis()) Time_millis = millis();
  if(millis()-Time_millis>INTERVAL_Time) {
    getCurrentTime();
    Time_millis = millis();
  } 
}


void updateAlarm() {

  if(timeHH==1&&timeMM<2) {
        Serial.println(timeHH);
        Serial.println(micValue);
   //     Serial.println(timeMM);  
      delay(1000);
      if(micValue>80) {
        if(!isAlaram)isRoar=true;  //是否吼叫
      }
      if(isRoar) {
        noTone(buzzer_pin);
      } else {
        tone(buzzer_pin,800); //在端口输出频率  
      }
  } 
  else {
      isRoar=false;
      if(!isAlaram)noTone(buzzer_pin);
  }
}

void updateMic() {
  //micValue = analogRead(micPin);
  //value = map(micValue, 0, 1023, 0, 255);
  micValue = map(analogRead(micPin),0,1023,0,20);
  micValue = (20. * log(10)) * (micValue / 5.0);
  Serial.println(micValue);
}

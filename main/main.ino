/*-----------------Version 1.3--------------*/
/*-----------------联网测试版本----------------*/
/**
 * 
 * 
 * Wifi库文件
 * 
 * 
 */
#include <E-ink.h>
#include "DHT.h"  
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>//解析json
#include <ESP8266WiFiMulti.h>   //  ESP8266WiFiMulti库
#include <ESP8266WebServer.h>   //  ESP8266WebServer库
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <EEPROM.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

/***********************************/


#define SSID_MAX_LENGTH 33
#define PASSWORD_MAX_LENGTH 16
#define DHTPIN D6
#define DHTTYPE DHT11
#define pushButton D3  
#define dat  9
#define clk  10
#define dustPin  D7
#define ledPower  D4


/******************************************全局变量*********************************/
DHT dht(DHTPIN, DHTTYPE);
ESP8266WiFiMulti wifiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 8*3600, 60000);
//端口及数据
WiFiServer server(80);
String ssid="";
String data = "";
String password = "";
WiFiClient client;
String str=""; 
DynamicJsonDocument doc(1024);
ESP8266WebServer esp8266_server(80);
String Location = "Fu zhou";
String htmlCode;
int updatetime = 60;
bool hour24 = true;
bool hasAlarm = false;
bool isCelsius = true;
float dustVal = 0;
int delayTime = 280;
int delayTime2 = 40;
float offTime = 9680;
byte dateFormat = 1;//1:yyyy-mm-dd, 2:dd-mm-yyyy, 3:mm-dd-yyyy

struct Time {
  int hour;
  int minute;
}currenttime = {0, 0}, Alarm = {7, 0};

struct Weather {
  int humanDetect;
  String weathernow;          /*暂不清楚天气共有多少种，分别叫什么名字，目前已知的类型有{“小雨”，“中雨”，“阴”，“多云”}。如果有确定的，可以联系电话13395699612来修改*/
  int temperature;
  float humidity;//湿度
  int airQuality;
}weather = {1,"晴", 25, 0, 89};

struct Date {
  
  int year;
  int month;
  int day;
  String week;
}date = {2021, 1, 1, "星期二"};
  
void setup()
{
  Serial.begin(115200);
  Serial.println();
  dht.begin(); //DHT开始工作
  /*****************************Hardware pin init************************/
  pinMode(pushButton, INPUT);
  pinMode(dat,OUTPUT);
  pinMode(clk,OUTPUT);
  digitalWrite(clk,LOW);
  pinMode(ledPower, OUTPUT);
  pinMode(dustPin, INPUT);//???
  
  Serial.println("setup");
  
  EEPROM.begin(1024);
  getHtmlCode();
  
  display.init(115200); //esp8266和水墨屏竞争端口，只有水墨屏输出。

  Serial.println("setup done");

  display.fillScreen(GxEPD_WHITE);
  display.update();
  welcomepage();
  display.update();
  delay(3000);
  
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold12pt7b);
  display.setCursor(40, 250);
  display.println("Connect to Internet ...");
  display.drawPic(wifi,140,80,120,96);
  display.update();
  
  /*Todo: 联网*/
 wifiConnect();
 timeInit();
  String wifiname=WiFi.SSID();
  
//  String ip=WiFi.localIP();
//  toaskii(wifiname.c_str(), strlen(wifiname));
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(10, 50);
  display.println("Connect Successful");
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 225);
  display.print("Name: Local");
  display.setCursor(10, 275);
  display.print("  IP: ");
  display.println(WiFi.localIP());
  display.drawPic(wifi,140,80,120,96);
  display.update();

  
  //网页配置相关
  esp8266_server.begin();                           // 启动网站服务
  esp8266_server.on("/", HTTP_GET, handleRoot);     // 设置服务器根目录即'/'的函数'handleRoot'
  esp8266_server.on("/BUTTON", HTTP_POST, handleSubmit);  // 设置处理按钮上传请求的函数'handleSubmit'
  esp8266_server.onNotFound(handleNotFound);  
  delay(3000);
  
}

unsigned long lastupdate = millis();
 int lastApiUpdate=-1;
void loop() {
  
  if(weather.humanDetect)
    updatetime = 10;
  else
    updatetime = 300;
    
   esp8266_server.handleClient();  
//  if(hasAlarm) {
//    if(Alarm.hour == currenttime.hour && Alarm.minute == currenttime.minute) {
//      /*Todo: 启动蜂鸣器*/
//    }
//  }

  /*Todo: 更新数据*/
  /*1.更新本地数据
   * 
   *       hour24               //true = 24小时制， false = 12小时制
   *       hasAlarm             //true = 闹铃开启， false = 闹铃关闭
   *       isCelsius            //true = 显示摄氏度， false = 显示华氏度
   *       dateFormat           //日期格式: 1 = yyyy-mm-dd, 2 = dd-mm-yyyy, 3 = mm-dd-yyyy
   *       Time结构的Alarm       //闹铃时间，包括Alarm.hour和Alarm.minute,注意: 闹铃的小时和分钟均为int类型
   */
     checksensor();
     timeUpdate();
     if(lastApiUpdate!=currenttime.hour)
      deserializeJSONData(getJsonWeatherData());
      
  /*2.更新联网时间数据，包括
   *       Time结构的currenttime//当前时间
   *       Date结构的date       //当前日期
   */

  show();
  delay(1000);
}
void checksensor(){
  weather.humanDetect=digitalRead(pushButton);
  weather.humidity=dht.readHumidity();
  weather.temperature = int(dht.readTemperature());
  Serial.print(weather.temperature);
  Serial.println("C");
  weather.airQuality=int(random(200));
  }
  
void show() {
  if(millis() - lastupdate < updatetime*1000) {
    return;
  }
  lastupdate = millis();
  display.fillScreen(GxEPD_WHITE);
  showTime();
  display.drawFastHLine(70-33, 153, 180, GxEPD_BLACK);
  showDate();
  display.fillRect(225, 50, 1, 200, GxEPD_BLACK);
  showWeather();
  showlabel();
  if(hasAlarm)
    showAlarm();
  display.update();
}
void showlabel() {
  display.setFont(&Org_01);
  display.setCursor(10, 298);
  display.print("EE297");
  display.setCursor(290, 298);
  display.print("Maynoonth University");
}
void welcomepage() {
  display.setTextColor(GxEPD_BLACK);
  display.drawPic(FZU, 0, 0, 120, 120);
  display.drawPic(MU, 280, 0, 120, 132);
  showlabel();
  display.setFont(&FreeMonoBold24pt7b);
  display.setCursor(125, 100);
  display.print("EE297");
  display.setCursor(75, 150);
  display.print("IOT E-ink");
  display.setCursor(0, 200);
  display.setFont(&FreeMono9pt7b);
  display.println("By: 19103387  Zhichen Wei");
  display.println("    19103557  Xiao Zheng");
  display.println("    19104278  Yanxiang Wang");
  display.println("    19104642  Tian Luo");
  display.println("    19105690  Zongtan Li");
}
void showTime() {
  display.setFont(&FreeSerifBoldItalic24pt7b);
  display.setCursor(70, 150);
  if(hour24) {
    if(currenttime.hour < 10) {
      display.print("0");
      display.print(currenttime.hour);
    }else
      display.print(currenttime.hour);
    display.print(" : ");
    if(currenttime.minute < 10) {
      display.print("0");
      display.print(currenttime.minute);
    }else
      display.print(currenttime.minute);
  }else {
    int hour = (currenttime.hour>12) ? (currenttime.hour - 12) : (currenttime.hour);
    if(hour < 10) {
      display.print(" ");
      display.print(hour);
    }else
      display.print(hour);
    display.print(" : ");
    if(currenttime.minute < 10) {
      display.print("0");
      display.print(currenttime.minute);
    }else
      display.print(currenttime.minute);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(70-33, 150);
    if(currenttime.hour>12)
      display.print("PM");
    else
      display.print("AM");
  }
}
void showDate() {
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(55, 170);
  if(date.day < 10)
    display.print(" ");
  if(date.month != 9)
    display.print(" ");
  switch(dateFormat) {
    case 1:           //yyyy-mm-dd
      display.print(date.year);
      display.print(", ");
      display.print(transfM(date.month));
      display.print(" ");
      display.print(date.day);
      display.println(transfD(date.day));
      break;
    case 2:           //dd-mm-yyyy
      display.print(date.day);
      display.print(transfD(date.day));
      display.print(" ");
      display.print(transfM(date.month));
      display.print(", ");
      display.println(date.year);
      break;
    case 3:           //mm-dd-yyyy
      display.print(transfM(date.month));
      display.print(" ");
      display.print(date.day);
      display.print(transfD(date.day));
      display.print(", ");
      display.println(date.year);
      break;
  }
  String week = transfW(date.week);
  int spacenumber = 20-week.length();
  for(int i = 0; i < spacenumber; i++) {
    display.print(" ");
  }
  display.print(week);
}
void showAlarm() {
  display.setFont(&FreeMonoBold9pt7b);
  display.drawPic(alarm, 150, 200, 16, 16);
  display.setCursor(166, 214);
  if(Alarm.hour < 10) {
      display.print("0");
      display.print(Alarm.hour);
    }else
      display.print(Alarm.hour);
    display.print(":");
    if(Alarm.minute < 10) {
      display.print("0");
      display.print(Alarm.minute);
    }else
      display.print(Alarm.minute);
}
void showWeather() {
  display.drawPic(location, 237, 55, 16, 20);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(237 + 16, 70);
  display.print(Location);
  display.setFont(&FreeSans18pt7b);
  display.setCursor(237 + 8, 120  + 104 - 15);
  int degree;
  if(isCelsius) {
    degree = weather.temperature;
    if(degree < 0){
      display.print("-");
      degree = -degree;
    }else
      display.print(" ");
    display.println(degree);
    display.drawPic(c, 237 + 72 - 8, 90 + 104 - 15, 32, 32, false);
  }else{
    degree = int(weather.temperature*1.8 + 32);
    if(degree >= 0 && degree < 100)
      display.print(" ");
    display.println(degree);
    display.setFont(&FreeSans9pt7b);
    display.drawPic(f, 237 + 72 - 8, 90 + 104 - 15, 32, 32, false);
  }
  display.setFont(&FreeSans9pt7b);
  display.setCursor(237 + 8, 90 + 104 + 32 + 13 - 15);
  display.print("Humidity: ");
  display.print(weather.humidity);
  display.print("%");
  display.setCursor(237 + 8, 90 + 104 + 32 + 13 + 2);
  display.print("AQI: ");
  display.print(weather.airQuality);
  display.print(" ");
  display.println(airlevel(weather.airQuality));
  displayWeatherIcon(weather.weathernow);

}
void updateinf() //测试函数
{
  currenttime.minute++;
  if(currenttime.minute >= 60) {
    currenttime.minute = 0;
    currenttime.hour++;
  }
  if(currenttime.hour >= 24) {
    currenttime.hour = 0;
    date.day++;
  }
  if(date.day > 31) {
    date.day = 1;
    date.month++;
  }
  if(date.month > 12) {
    date.month = 1;
    date.year++;
  }
}

void toaskii(char str[],int len) {
  for(int i = 0; i < len; i++) {
    if(*(str+i)>=0&&*(str+i)<=127)
      continue;
    else
      *(str+i) = '?';
  }
  *(str+29) = '\0';
}

String transfM(int Month){
  switch (Month) {
    case 1:
        return "Jan";
    case 2:
        return "Feb";
    case 3:
        return "Mar";
    case 4:
        return "Apr";
    case 5:
        return "May";
    case 6:
        return "Jun";
    case 7:
        return "Jul";
    case 8:
        return "Aug";
    case 9:
        return "Sept";
    case 10:
        return "Oct";
    case 11:
        return "Nov";
    case 12:
        return "Dec";
  }
}

String transfD(int Day) {
  String day;
  if(Day == 1 || Day == 21 || Day == 31){
    day = "st";
  }else if(Day == 2 || Day == 22){
    day = "nd";
  }else if(Day == 3 || Day == 23){
    day = "rd";
  }else
    day = "th";
  return day;
}
String transfW(String Week) {
  char week[10];
  strcpy(week, Week.c_str());
  if(strcmp(week, "周一") == 0) {
    return "Monday";
  }else if(strcmp(week, "周二") == 0) {
    return "Tuesday";
  }else if(strcmp(week, "周三") == 0) {
    return "Wednesday";
  }else if(strcmp(week, "周四") == 0) {
    return "Thursday";
  }else if(strcmp(week, "周五") == 0) {
    return "Friday";
  }else if(strcmp(week, "周六") == 0) {
    return "Saturday";
  }else {
    return "Sunday";
  }
}
void displayWeatherIcon(String WeatherNow) {
  char weatherNow[10];
  strcpy(weatherNow, WeatherNow.c_str());
  if(strcmp(weatherNow, "晴") == 0) {
    if(currenttime.hour >= 20 || currenttime.hour <= 5)
      display.drawPic(sunny1, 225 + 35, 122 - 32 - 15, 104, 104);
    else
      display.drawPic(sunny0, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "多云") == 0) {
    if(currenttime.hour >= 20 || currenttime.hour <= 5)
      display.drawPic(cloudy1, 225 + 35, 122 - 32 - 15, 104, 104);
    else
      display.drawPic(cloudy0, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "阴") == 0) {
      display.drawPic(overcast, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "小雨") == 0) {
    if(currenttime.hour >= 20 || currenttime.hour <= 5)
      display.drawPic(lightRain1, 225 + 35, 122 - 32 - 15, 104, 104);
    else
      display.drawPic(lightRain0, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "中雨") == 0) {
      display.drawPic(mediumRain, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "大雨") == 0) {
      display.drawPic(heavyRain, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "雪") == 0) {
      display.drawPic(snow, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "雷雨") == 0) {
      display.drawPic(hail, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "冰雹") == 0) {
      display.drawPic(thunderstorms, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "雾") == 0) {
      display.drawPic(fog, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "扬尘") == 0) {
      display.drawPic(dust, 225 + 35, 122 - 32 - 15, 104, 104);
  }else if(strcmp(weatherNow, "大风") == 0) {
      display.drawPic(windy, 225 + 35, 122 - 32 - 15, 104, 104);
  }
}
String airlevel(int aqi) {
  if(aqi < 50) {
    return "good";
  }else if(aqi < 100) {
    return "Moderate";
  }else if(aqi < 150) {
    return "LP";    //for Lightly Polluted
  }else if(aqi < 200) {
    return "MP";    //for Moderately Polluted
  }else if(aqi < 300) {
    return "HP";    //for Heavily Polluted
  }else {
    return "SP";    //for Severely Polluted
  }
}

////////////////////////////////////wifi//////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void wifiConnect(){
//  wifiMulti.addAP("修仙小分队", "18159812859a");
  wifiMulti.addAP("p20", "123456789");
  while (wifiMulti.run() != WL_CONNECTED) {  // 此处的wifiMulti.run()是重点。通过wifiMulti.run()，NodeMCU将会在当前
                                             // 环境中搜索addAP函数所存储的WiFi。如果搜到多个存储的WiFi那么NodeMCU
  }                                          // 一旦连接WiFI成功，wifiMulti.run()将会返回“WL_CONNECTED”。这也是
                                             // 此处while循环判断是否跳出循环的条件。
  // WiFi连接成功后将通过串口监视器输出连接成功信息 
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // 通过串口监视器输出连接的WiFi名称
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());           // 通过串口监视器输出ESP8266-NodeMCU的IP
  //显示ip等信息
}
String getJsonWeatherData(){
  HTTPClient http;
  String temp = "";
  
  //Serial.print("[HTTP] begin...\n");
  if (http.begin(client, "http://api.help.bj.cn/apis/weather/?id=101230101")) {
    //Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        temp = http.getString();
        Serial.println(temp);
      }
    } else {
      temp = http.errorToString(httpCode);
      //Serial.printf("[HTTP] GET... failed, error: %s\n", temp.c_str());
    }
    http.end();
  } else {
    //Serial.printf("[HTTP} Unable to connect\n");
  }
  lastApiUpdate=currenttime.hour;
  //Serial.println(temp);
  return temp;
}
//解析json数据
void deserializeJSONData(String JsonWeatherData){
  deserializeJson(doc, JsonWeatherData);
  JsonObject obj = doc.as<JsonObject>();
  //基本信息
  Location = obj["cityen"].as<String>();
  Location = (char(Location.charAt(0)-32)) + Location.substring(1,Location.length());
  
  weather.weathernow = obj["weather"].as<String>();
  
  weather.temperature = doc["temp"];

  weather.humidity = getFloat(obj["humidity"].as<String>());

  weather.airQuality = doc["pm25"];

  String d = obj["today"].as<String>();
  date.year = d.substring(0,4).toInt();
  date.month = d.substring(5,7).toInt();
  date.day = d.substring(8,10).toInt();
  date.week = d.substring(11,d.length());

}//初始化

void timeInit(){
  timeClient.begin();
  timeClient.update();
}


//时间更新
void timeUpdate(){
  timeClient.update();
  if(timeClient.getDay() != date.day){
    date.day = timeClient.getDay();
  }
    if(currenttime.minute != timeClient.getMinutes()){
      currenttime.minute = timeClient.getMinutes();
    }
    if(currenttime.hour != timeClient.getHours()){
      currenttime.hour = timeClient.getHours();
    }
}
//获取string里的数字
float getFloat(String s){
  int l = s.length();
  float result = 0;
  bool ifNotDot = true;
  int times=10;
  for(int i=0;i<l;i++){
    if(s[i]<='9'&&s[i]>='0'&&ifNotDot){
      result = result*10 + (s[i]-'0'); 
    }else if(s[i]<='9'&&s[i]>='0'&&!ifNotDot){
      result += 1.0*(s[i]-'0')/times; 
      times*=10;
    }
    if(s[i]=='.'){
      ifNotDot = false;
    }
  }
  return result;
}
/////////////////////////////语音播报模块相关函数////////////////////
void boradcast(unsigned int hour,unsigned int weather){
  writeByte(0b11111000);
      if(hour>4&&hour<12)
      writeByte(0b11011000);
   else if(hour>=12&&hour<=18)
      writeByte(0b01011000);
  else if(hour>=18||hour<=4)
     writeByte(0b10011000);
       delay(1300); 
     writeByte(0b11111000);
      delay(100);
      switch(weather)
      {
        case 1:
        writeByte(0b01111000);
        break;
         case 2:
        writeByte(0b10111000);
        break;
        case 3:
        writeByte(0b00111000);
        break;
        }
      delay(5000);
      writeByte(0b11111000);
      delay(100);
  }
void writeByte(int data){
  for(int i=0;i<8;i++){
    if(data>>i&1){
      digitalWrite(dat,HIGH);
    }else{
      digitalWrite(dat,LOW);
    }
    digitalWrite(clk,LOW);
    delayMicroseconds(10);
    digitalWrite(clk,HIGH);
    delayMicroseconds(10);
    digitalWrite(clk,LOW);
  }
  }
  ////////////////////////////网页配置相关函数//////////////////////////////
 
void handleRoot() {
  esp8266_server.send(200,"text/html",htmlCode);
}
void handleNotFound(){
  esp8266_server.send(404, "text/plain", "404: Not found"); // 发送 HTTP 状态 404 (未找到页面) 并向浏览器发送文字 "404: Not found"
}
void handleSubmit() {
  ssid = esp8266_server.arg(0);
  password = esp8266_server.arg(1);
  Serial.print("ssid : ");
  Serial.print(ssid);
  Serial.print(", password : ");
  Serial.print(password);

  wifiSSIDAndPassword_write(0);

  wifiSSIDAndPassword_read(0);
  
  digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));// 改变LED的点亮或者熄灭状态
  esp8266_server.sendHeader("Location","/");          // 跳转回页面根目录
  esp8266_server.send(303);                           // 发送Http相应代码303 跳转  
}
 void getHtmlCode(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  htmlCode = "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" charset=\"UTF-8\">"
  "<form action=\"/BUTTON\" method=\"post\">"
    "<div style=\"text-align:center\">"
      "<h1>ESP8266 NodeMCU</h1><br>"
      "<h1>网络配置</h1><br>"
      "<h1></h1><br>"
        "<select name=\"SSID\">"
  ;
  
  for(int i = 0;i < n;i++){
    htmlCode = htmlCode + "<option value=\"" + WiFi.SSID(i) + "\">";
    htmlCode = htmlCode + WiFi.SSID(i);
    htmlCode = htmlCode + "</option>";
  }
  htmlCode = htmlCode + "</select><br>"
      "<input type=\"password\" name=\"PASSWORD\" placeholder=\"WIFI PASSWORD\"><br>"
      "<input type=\"submit\" value=\"上传\"><br>"
    "</div>"
  "</form>"
  ;
}
void wifiSSIDAndPassword_read(int index){
  int startIndex = index*50 + 0;
  int i=1+startIndex;
  int l1 = startIndex+34;
  int l2 = startIndex+50;
  char temp;
  
  Serial.print("位置 : ");
  Serial.print(startIndex);
  Serial.print(" ,配置状态 : ");
  Serial.println(EEPROM.read(startIndex));

  ssid = "";
  password = "";
  for(;i<l1;i++){
    temp = (char) EEPROM.read(i);
      ssid = ssid + temp;
      Serial.print("i : ");
      Serial.print(i);
      Serial.print(" ,ssid : ");
      Serial.println(ssid);
  }
  
  for(i=startIndex+34;i<l2;i++){
    password = password + (char) EEPROM.read(i);
    Serial.print("i : ");
    Serial.print(i);
    Serial.print(" ,password : ");
    Serial.println(password);
  }
  
  Serial.println("读取完成");
}
void wifiSSIDAndPassword_write(int index){
  int count;
  int startIndex = index*50 + 0;
  int i=1+startIndex;
  int l1 = ssid.length();
  int l2 = startIndex+password.length();

  EEPROM.write(startIndex,1);
  for(count=0;count<l1;i++,count++){
    EEPROM.write(i,ssid.charAt(count));
  }
  Serial.println("停止写");
  for(i=startIndex+34,count=0;count<l2;i++,count++){
    EEPROM.write(i,password.charAt(count));
  }
  EEPROM.commit();
  delay(100);
  Serial.println("写入完成");
  Serial.print("位置 : ");
  Serial.print(startIndex);
  Serial.print(" ,配置状态 : ");
  Serial.println(EEPROM.read(startIndex));
}

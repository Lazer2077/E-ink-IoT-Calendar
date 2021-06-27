/**
 * 
 * 
 * 库文件
 * 
 * 
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>//解析json

#include <ESP8266WiFiMulti.h>   //  ESP8266WiFiMulti库
#include <ESP8266WebServer.h>   //  ESP8266WebServer库

#include <U8g2lib.h>//gui

#include <WiFiUdp.h>
#include <NTPClient.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

/**
 * 
 * 
 * 宏定义
 * 
 * 
 */


//#define SCL_PIN 0
//#define SDA_PIN 2

#define SCL_PIN D2
#define SDA_PIN D3


/**
 * 
 * 
 * 实例及全局变量
 * 
 * 
 */

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer esp8266_server(80);

//端口及数据
WiFiServer server(80);
String data = "";

WiFiClient client;
String str=""; 

DynamicJsonDocument doc(1024);

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL_PIN, /* data=*/ SDA_PIN, /* reset=*/ U8X8_PIN_NONE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 8*3600, 60000);

/**
 * 
 * 
 * 结构体
 * 
 * 
 */

 
//详细天气信息
struct weatherData{
  //基本信息
  String city;//城市
  String weather;//天气
  int temp;//温度
  int uptimeHour;//更新时间小时
  int uptimeMinute;//更新时间分钟

  //风力相关
  String wd;//风向
  int wdforce;//风力
  float wdspd;//风速

  //降水相关
  int prcp;//降雨
  float prcp24h;//24小时降雨量
  int humidity;//湿度

  //其他
  int stp;//气压
  float wisib;//能见度
  int aqi;//AQI 空气质量
  int pm25;//pm2.5

  String today;
}weatherDataToday;

//36小时内温度天气变化
struct weatherInterval3Hour{
  int temp;
  int day;
  int timeHour;
  String weather;
};

struct weatherInterval3Hour weather36Hour[8];

//时间数据
struct timeData{
  int day;
  String week;

  int hour;
  int minute;
  int second;
}timeDataNow;


//页数状态
struct pageState{
  int lastChange;
  int interval;
  bool page_1;
  bool page_2;
  bool page_3;
};

struct pageState pageState = {0,15,false,true,false};

unsigned long lasttime100ms = 0;
unsigned long lasttime1s = 0;
unsigned long lasttime1m = 0;
unsigned long lasttime1h = 0;

/**
 * 
 * 
 * 主函数
 * 
 * 
 */
void setup() {
  Serial.begin(115200);
  //初始化gui
  draw_init();
  //联网
  wifiConnect();
  //时间初始化
  timeInit();

  //
  pageState.lastChange = timeClient.getSeconds();

  //初始化间隔循环
  loopIntervalInit();
}

void loop() {

  loop100ms();
  loop1s();
  loop1m();
  loop1h();
}

void loopIntervalInit(){
  timeUpdate();
  deserializeJSONData(getJsonWeatherData());
  deserializeJSONData36h(getJsonWeatherData36h());
  
  //画图（放到最后）
  changePage();
}

void loop100ms(){
  if(millis()-lasttime100ms<100){
    return ;
  }
  //更新
  lasttime100ms = millis();

  //主体
  if(timeDataNow.hour==0&&timeDataNow.minute==0){
    timeUpdate();
  }
  
}


void loop1s(){
  //更新
  if(millis()-lasttime1s<1000){
    return ;
  }
  lasttime1s = millis();
  //主体
  Serial.print("loop1s");
  Serial.println(millis());

  timeUpdate();
  
  int diff = timeClient.getSeconds()-pageState.lastChange;
  if(diff<0){
    diff += 60;
  }
  if(diff >= pageState.interval){
    changePage();
    pageState.lastChange = timeClient.getSeconds();
  }
  
  drawPage();
}

void loop1m(){
  //更新
  if(millis()-lasttime1m<60*1000){
    return ;
  }
  lasttime1m = millis();
  //主体
  
}

void loop1h(){
  //更新
  if(millis()-lasttime1h<3600*1000){
    return ;
  }
  lasttime1h = millis();
  //主体
  
  //获取数据
  deserializeJSONData(getJsonWeatherData());
  deserializeJSONData36h(getJsonWeatherData36h());
}

void changePage(){
  if(pageState.page_1==true){
    pageState.page_1 = false;
    pageState.page_2 = true;
  }else if(pageState.page_2==true){
    pageState.page_2 = false;
    pageState.page_1 = true;
  }
}

void drawPage(){
  if(pageState.page_1){
    draw_firstPage();
  }else if(pageState.page_2){
    draw_secondPage();
  }
}


/**
 * 
 * 
 * 功能函数
 * 
 * 
 */

//****************
//     WIFI相关
//****************

//连接wifi
void wifiConnect(){
  wifiMulti.addAP("修仙小分队", "18159812859a");
  wifiMulti.addAP("NS", "www2621689614");

  int i = 0;             
  while (wifiMulti.run() != WL_CONNECTED) {  // 此处的wifiMulti.run()是重点。通过wifiMulti.run()，NodeMCU将会在当前
                                             // 环境中搜索addAP函数所存储的WiFi。如果搜到多个存储的WiFi那么NodeMCU
    draw_boot_animation("网络连接中",500,i);    // 将会连接信号最强的那一个WiFi信号。
    i++;
    if(i>=5){
      i=0;
    }
  }                                          // 一旦连接WiFI成功，wifiMulti.run()将会返回“WL_CONNECTED”。这也是
                                             // 此处while循环判断是否跳出循环的条件。
  // WiFi连接成功后将通过串口监视器输出连接成功信息 
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // 通过串口监视器输出连接的WiFi名称
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());           // 通过串口监视器输出ESP8266-NodeMCU的IP
  //显示ip等信息
  draw_WIFI_connect(3000);
}

//获取json天气数据
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
  //Serial.println(temp);
  return temp;
}

//获取36h时间信息
String getJsonWeatherData36h(){
  HTTPClient http;
  String temp = "";
  
  //Serial.print("[HTTP] begin...\n");
  if (http.begin(client, "http://api.help.bj.cn/apis/weather36h/?id=福州")) {
    //Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        temp = http.getString();
        //Serial.println(temp);
      }
    } else {
      temp = http.errorToString(httpCode);
      //Serial.printf("[HTTP] GET... failed, error: %s\n", temp.c_str());
    }
    http.end();
  } else {
    //Serial.printf("[HTTP} Unable to connect\n");
  }
  return temp;
}


//解析json数据
void deserializeJSONData(String JsonWeatherData){
  deserializeJson(doc, JsonWeatherData);
  JsonObject obj = doc.as<JsonObject>();
  //基本信息
  weatherDataToday.city = obj["city"].as<String>();
  weatherDataToday.weather = obj["weather"].as<String>();
  weatherDataToday.temp = doc["temp"];
  float uptime = getFloat(obj["uptime"].as<String>());
  //更新时间
  weatherDataToday.uptimeHour = (int) uptime/100;
  weatherDataToday.uptimeMinute = (int) uptime%100;
  //风力相关
  weatherDataToday.wd = obj["wd"].as<String>();
  weatherDataToday.wdforce = (int)doc["wdforce"][0];
  weatherDataToday.wdspd = getFloat(obj["wdspd"].as<String>());
  //降水相关
  weatherDataToday.prcp = doc["prcp"];
  weatherDataToday.prcp24h = doc["prcp24h"];
  weatherDataToday.humidity = getFloat(obj["humidity"].as<String>());
  //其他
  weatherDataToday.stp = doc["stp"];
  weatherDataToday.wisib = getFloat(obj["wisib"].as<String>());
  weatherDataToday.aqi = doc["aqi"];
  weatherDataToday.pm25 = doc["pm25"];
  //日期
  
  weatherDataToday.today = obj["today"].as<String>().substring(0,obj["today"].as<String>().length()-7);
}

//解析36hjson数据
void deserializeJSONData36h(String JsonWeatherData36h){
  deserializeJson(doc, JsonWeatherData36h);
  JsonObject obj = doc.as<JsonObject>();

  Serial.println(JsonWeatherData36h);
  
  for(int i=0;i<8;i++){
    weather36Hour[i].temp = doc["weather36h"][i]["temp"];
    String s = obj["weather36h"][i]["time"].as<String>();
    weather36Hour[i].day = (int) getFloat(s.substring(8,10));
    weather36Hour[i].timeHour = (int) getFloat(s.substring(11,13));
    weather36Hour[i].weather = obj["weather36h"][i]["weather"].as<String>();
  }
//
//  for(int i=0;i<8;i++){
//    Serial.print("温度：");
//    Serial.print(weather36Hour[i].temp);
//    Serial.print(",日期：");
//    Serial.print(weather36Hour[i].day);
//    Serial.print("，时间：");
//    Serial.print(weather36Hour[i].timeHour);
//    Serial.print("，天气：");
//    Serial.println(weather36Hour[i].weather);
//  }
//  
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

//****************
//     功能函数
//****************

//获取体感温度
float getApparentTemperature(){
  double e = 1.0*weatherDataToday.humidity/100 * 6.105 * exp(17.27*weatherDataToday.temp/(237.7+weatherDataToday.temp));
  double AT = 1.07*weatherDataToday.temp + 0.2*e - 0.065*weatherDataToday.wdspd/3.6 - 2.7;
  AT = (int)(AT*100);
  AT = AT/100;
  return (float) AT;
}

//空气质量分级
String getAirQuality(int aqi){
  if(aqi<50){
    return "优";
  }else if(aqi<100){
    return "良";
  }else if(aqi<120){
    return "轻度污染";
  }else if(aqi<200){
    return "中度污染";
  }else if(aqi<300){
    return "重度污染";
  }else{
    return "严重污染";
  }
}

//时间格式化
String timeFormat(int timeHour,int timeMinute){
  String s;
  if(timeHour<10){
    s = "0";
  }
  s = s + timeHour;
  s = s + ":";
  if(timeMinute<10){
    s = s + "0";
  }
  s = s + timeMinute;
  return s;
} 

//****************
//     GUI相关
//****************

//初始化
void draw_init(){
  u8g2.begin();
  u8g2.enableUTF8Print();
}

//开机动画
void draw_boot_animation(String tmp,int interval,int num){
  static int index = 0;
  int y = 39;
  int x = 64;
  u8g2.clearBuffer();
  if(num > 0){
    if(num > 1){
      if(num > 2){
        if(num > 3){
          u8g2.setDrawColor(1);
          u8g2.drawDisc(x,y,61,U8G2_DRAW_ALL);
          u8g2.setDrawColor(0);
          u8g2.drawDisc(x,y,50,U8G2_DRAW_ALL);
        }
        u8g2.setDrawColor(1);
        u8g2.drawDisc(x,y,44,U8G2_DRAW_ALL);
        u8g2.setDrawColor(0);
        u8g2.drawDisc(x,y,37,U8G2_DRAW_ALL);
      }
      u8g2.setDrawColor(1);
      u8g2.drawDisc(x,y,31,U8G2_DRAW_ALL);
      u8g2.setDrawColor(0);
      u8g2.drawDisc(x,y,24,U8G2_DRAW_ALL);
    }
    u8g2.setDrawColor(1);
    u8g2.drawDisc(x,y,18,U8G2_DRAW_ALL);
    u8g2.setDrawColor(0);
    u8g2.drawDisc(x,y,11,U8G2_DRAW_ALL);
  }
  u8g2.setDrawColor(1);
  u8g2.drawDisc(x,y,5,U8G2_DRAW_ALL);

  u8g2.setDrawColor(0);
  u8g2.drawTriangle(0,0,63,34,128,0);
  u8g2.drawTriangle(0,64,63,44,128,64);

  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor((128-u8g2.getUTF8Width(tmp.c_str()))/2,14);
  u8g2.print(tmp);
  
  u8g2.sendBuffer();
  delay(interval);
}

//联网成功界面
void draw_WIFI_connect(int delayTime){
  
  int x,y;
  
  String tmp = WiFi.SSID();
  tmp = "SSID:" + tmp;
  
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  x=(128-u8g2.getUTF8Width(tmp.c_str()))/2;
  y = 27;
  u8g2.setCursor(x, y);
  u8g2.print(tmp);
  
  
  tmp = WiFi.localIP().toString().c_str();
  tmp = "IP:" + tmp;
  x=(128-u8g2.getUTF8Width(tmp.c_str()))/2;
  y = 43;
  u8g2.setCursor(x, y);
  u8g2.print(tmp);

  tmp = "By ~WYX";
  y = 61;
  x=128-u8g2.getUTF8Width(tmp.c_str());
  u8g2.setCursor(x, y);
  u8g2.print(tmp);
  
  u8g2.sendBuffer();
  Serial.println("显示ip及ssid");
  delay(delayTime);
}

//第一页
void draw_firstPage(){
  u8g2.clearBuffer();
  drawTopBar_1(weatherDataToday.city,"星期"+timeDataNow.week,timeFormat(timeDataNow.hour,timeDataNow.minute));
  drawDetail_1(weatherDataToday.temp , weatherDataToday.humidity , weatherDataToday.weather , weatherDataToday.uptimeHour , getApparentTemperature());
  u8g2.sendBuffer();
}

//顶部条
void drawTopBar_1(String location,String center,String timeNow){
  int barH = 16;
  int x,y;
  y=14;
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  const char* tmp = location.c_str();
  x=(32-u8g2.getUTF8Width(tmp))/2;
  u8g2.setCursor(x,y);
  u8g2.print(tmp);
  tmp = center.c_str();
  x=(128-u8g2.getUTF8Width(tmp))/2;
  u8g2.setCursor(x,y);
  u8g2.print(tmp);
  tmp = timeNow.c_str();
  x=(32-u8g2.getUTF8Width(tmp))/2+96;
  u8g2.drawStr(x,y,tmp);
}

//温度湿度天气
void drawDetail_1(int tempeature,int humidity,String weather,int timeHour,float ATemp){
  int defaultH = 16;
  int barH = 16;
  int x,y;
  int x_;
  String tempS;
  y=45;
  u8g2.setFont(u8g2_font_helvR24_tf);
  //u8g2.setFont(u8g2_font_courB24_tf);
  //u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
  tempS = tempeature;
  x_ = u8g2.getUTF8Width(tempS.c_str());
  tempS =  tempS + " C";
  const char* tmp = tempS.c_str();
  x=(64-u8g2.getUTF8Width(tmp))/2+64;
  //u8g2.setCursor(x,y);
  //u8g2.print(tmp);
  u8g2.drawUTF8(x,y,tmp);
  u8g2.drawCircle(x+x_+9, y-22, 3, U8G2_DRAW_ALL);

  //湿度
  y=63;
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  tempS = "湿度:";
  tempS = tempS + humidity + "%";
  tmp = tempS.c_str();
  x=(48-u8g2.getUTF8Width(tmp))/2;
  u8g2.setCursor(x,y);
  u8g2.print(tmp);

  //温度
  tempS = ATemp;
  tempS = tempS + "℃";
  tmp = tempS.c_str();
  x=(48-u8g2.getUTF8Width(tmp))/2+80;
  u8g2.setCursor(x,y);
  u8g2.print(tmp);

  //天气文字
  tempS = weather;
  tmp = tempS.c_str();
  x=(32-u8g2.getUTF8Width(tmp))/2+48;
  u8g2.setCursor(x,y);
  u8g2.print(tmp);

  //天气图标
  u8g2.setFont(u8g2_font_open_iconic_weather_4x_t ); // 天气图标 32*32
  u8g2.setCursor(15, 49);
  u8g2.print(getWertherIconNum(weather,timeHour));
}


//天气状态转字符 阴(多云) 晴(夜晚) 雨 星星 晴
char getWertherIconNum(String weather,int timeHour){
  if(weather.equals("晴")){
    if(timeHour>18||timeHour<6){
      return 'B';
    }else{
      return 'E';
    }
  }else if(weather.indexOf("雨")>=0){
    return 'C';
  }else if(weather.equals("多云")||weather.equals("阴")){
    return 'A';
  }
  return -1;
}


//画第二页
void draw_secondPage(){
  u8g2.clearBuffer();
  drawTopBar_1(weatherDataToday.city,"12小时天气",timeFormat(timeDataNow.hour,timeDataNow.minute));
  drawDetail_2();
  u8g2.sendBuffer();
}


//温度折线图
void drawDetail_2(){
  int maxTemp = -50;
  int minTemp = 50;
  for(int i=0;i<8;i++){
    if(weather36Hour[i].temp>maxTemp){
      maxTemp = weather36Hour[i].temp;
    }
    if(weather36Hour[i].temp<minTemp){
      minTemp = weather36Hour[i].temp;
    }
  }
  
  //折线图
  int x=15;
  int y=48-map(weather36Hour[0].temp,minTemp,maxTemp,0,24);
  int l = weather36Hour[0].weather.length();
  u8g2.setFont(u8g2_font_5x8_tf);
  if(weather36Hour[0].temp<10&&weather36Hour[0].temp>=0){
    u8g2.setCursor(x-3,y-2);
    u8g2.print(weather36Hour[0].temp);
  }else{
    u8g2.setCursor(x-5,y-2);
    u8g2.print(weather36Hour[0].temp);
  }
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  if(l>1){
    u8g2.setCursor(x-6,62);
    u8g2.print(weather36Hour[0].weather.substring(l-3,l));
  }else{
    u8g2.setCursor(x-6,62);
    u8g2.print(weather36Hour[0].weather);
  }
  
  for(int i=1;i<8;i++){
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.drawLine(x, y, x+14, 48-map(weather36Hour[i].temp,minTemp,maxTemp,0,24));
    u8g2.drawCircle(x, y, 2, U8G2_DRAW_ALL);
    y=48-map(weather36Hour[i].temp,minTemp,maxTemp,0,24);
    x+=14;
    
    //下方文字
    l = weather36Hour[i].weather.length();
    if(l>1){
      u8g2.setCursor(x-4,62);
      u8g2.print(weather36Hour[i].weather.substring(l-3,l));
    }else{
      u8g2.setCursor(x-4,62);
      u8g2.print(weather36Hour[i].weather);
    }
    
    //温度数字
    u8g2.setFont(u8g2_font_5x8_tf);
    if(weather36Hour[i].temp<10&&weather36Hour[i].temp>=0){
      u8g2.setCursor(x-3,y-2);
      u8g2.print(weather36Hour[i].temp);
    }else{
      u8g2.setCursor(x-5,y-2);
      u8g2.print(weather36Hour[i].temp);
    }

    
  }
  u8g2.drawCircle(x, y, 2, U8G2_DRAW_ALL);

}


//****************
//   NTP时间同步
//****************

//初始化
void timeInit(){
  timeClient.begin();
  timeClient.update();
}

//时间更新
bool timeUpdate(){
  bool b = false;
  timeClient.update();
  if(timeClient.getDay()!=timeDataNow.day){
    timeDataNow.day = timeClient.getDay();
    b = true;
    timeDataNow.week = getWeekString(timeDataNow.day);
  }

  if(timeDataNow.second != timeClient.getSeconds()){
    timeDataNow.second != timeClient.getSeconds();
    if(timeDataNow.minute != timeClient.getMinutes()){
      timeDataNow.minute = timeClient.getMinutes();
      if(timeDataNow.hour = timeClient.getHours()){
        timeDataNow.hour = timeClient.getHours();
      }
      b=true;
    }
  }
  
  return b;
}

//时间转字符串
String getWeekString(int week){
  switch(week){
    case 1:return "一";
    case 2:return "二";
    case 3:return "三";
    case 4:return "四";
    case 5:return "五";
    case 6:return "六";
    case 7:return "日";
  }
}

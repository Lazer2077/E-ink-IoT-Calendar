#include <E-ink.h>
#include "DHT.h"  

#define DHTPIN D6
#define DHTTYPE DHT11
#define pushButton D3  
DHT dht(DHTPIN, DHTTYPE);

int updatetime = 60;
bool hour24 = true;
bool hasAlarm = false;
bool isCelsius = true;
String Location = "Fu zhou";
byte dateFormat = 1;//1:yyyy-mm-dd, 2:dd-mm-yyyy, 3:mm-dd-yyyy

struct Time {
  int hour;
  int minute;
}currenttime = {0, 0}, Alarm = {7, 0};

struct Weather {
  int humanDetect;
  String weathernow;          /*暂不清楚天气共有多少种，分别叫什么名字，目前已知的类型有{“小雨”，“中雨”，“阴”，“多云”}。如果有确定的，可以联系电话13395699612来修改*/
  float temperature;
  float humidity;//湿度
  float airQuality;
}weather = {1,"晴", 25, 0, 0};
Weather *pt=&weather;
struct Date {
  int year;
  int month;
  int day;
  String week;
}date = {2021, 1, 1, "星期二"};


void checksensor(Weather *a){
  a->humanDetect=digitalRead(pushButton);
  a->humidity=dht.readHumidity();
  a->temperature = dht.readTemperature();
  a->airQuality=random(200);
  }
  
void setup()
{
  Serial.begin(115200);
  Serial.println();
  dht.begin(); //DHT开始工作
  pinMode(pushButton, INPUT);
  Serial.println("setup");

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
  
 // while(/*Todo：判断网络未连接*/){}
  
/*Todo: 传入WIFI名和IP地址，下面为例子*/

  char wifiname[] = "617Dormitory's WiFi";
  String ip = "192.168.0.1";        //换成char[]会出bug，原因未知

  toaskii(wifiname, strlen(wifiname));
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(10, 50);
  display.println("Connect Successful");
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 225);
  display.print("Name: ");
  display.print(wifiname);
  display.setCursor(10, 275);
  display.print("  IP: ");
  display.println(ip);
  display.drawPic(wifi,140,80,120,96);
  display.update();
  delay(3000);
  
}

unsigned long lastupdate = millis();

void loop() {
  
  if(weather.humanDetect)
    updatetime = 10;
  else
    updatetime = 300;
  
  if(hasAlarm) {
    if(Alarm.hour == currenttime.hour && Alarm.minute == currenttime.minute) {
      /*Todo: 启动蜂鸣器*/
    }
  }

  /*Todo: 更新数据*/
  /*1.更新本地数据
   * 
   *       hour24               //true = 24小时制， false = 12小时制
   *       hasAlarm             //true = 闹铃开启， false = 闹铃关闭
   *       isCelsius            //true = 显示摄氏度， false = 显示华氏度
   *       dateFormat           //日期格式: 1 = yyyy-mm-dd, 2 = dd-mm-yyyy, 3 = mm-dd-yyyy
   *       Time结构的Alarm       //闹铃时间，包括Alarm.hour和Alarm.minute,注意: 闹铃的小时和分钟均为int类型
   */
     checksensor(pt); 
     
  /*2.更新联网时间数据，包括
   *       Time结构的currenttime//当前时间
   *       Date结构的date       //当前日期
   */

   /*3.更新联网天气数据
    *      Weather.weather     //当前天气
    */


//  updateinf();               //测试函数,可删

  show();
  delay(100);
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
  if(strcmp(week, "星期一") == 0) {
    return "Monday";
  }else if(strcmp(week, "星期二") == 0) {
    return "Tuesday";
  }else if(strcmp(week, "星期三") == 0) {
    return "Wednesday";
  }else if(strcmp(week, "星期四") == 0) {
    return "Thursday";
  }else if(strcmp(week, "星期五") == 0) {
    return "Friday";
  }else if(strcmp(week, "星期六") == 0) {
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

#include "DHT.h"  
#include <Ticker.h>  //导入定时器库
#define pushButton D3                     
#define DHTPIN D6
#define DHTTYPE DHT11
#define PW_Pin SD2
//#define latch
//#define clockPin
//#define dataPin
Ticker flipper;  
int detect_state=0;
//void play(int weather)//音乐播放函数 参数：今天天气
//{
//    //digitalWrite(latchPin,LOW); //将ST_CP口上面加低电平让芯片准备好接收数据
//   // shiftOut(dataPin,clockPin,MSBFIRST,weather);
//    
//   // digitalWrite(latchPin,HIGH); //将ST_CP这个针脚恢复到高电平
// }

typedef struct sensor{
  int humanDetect;
  float h,t;
  float PM;
  };
  

 DHT dht(DHTPIN, DHTTYPE);
  sensor se;
  sensor *pt=&se;
 void checksensor(sensor *a){
  a->humanDetect=digitalRead(pushButton);
  Serial.println(dht.readTemperature());
  a->h=dht.readHumidity();
  a->t = dht.readTemperature();
  }
  
void flip() {     //Call back function
detect_state=1;
checksensor(pt);
}
void setup() {
  Serial.begin(115200);
  Serial.print("This is start:");
  dht.begin(); //DHT开始工作
  pinMode(pushButton, INPUT);
  flipper.attach(20, flip);
  
}
void loop() {
 if(se.humanDetect&&detect_state){
  Serial.print("Humidity: ");
  Serial.println(se.h);
  Serial.print("Temperature: ");
  Serial.println(se.t);
  detect_state=0;
  }
}

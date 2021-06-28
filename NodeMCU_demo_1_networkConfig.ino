#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>

#include <EEPROM.h>

#define SSID_LOCAL "ESP8266 网络配置"
#define PASSWRD_LOCAL "123456789"

#define SSID_MAX_LENGTH 33
#define PASSWORD_MAX_LENGTH 16

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer esp8266_server(80);// 建立网络服务器对象，该对象用于响应HTTP请求。监听端口（80）

String htmlCode;

String ssid = "";
String password = "";

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  EEPROM.begin(1024);
  getHtmlCode();
  
  Serial.print("位置 : ");
  Serial.print(0);
  Serial.print(" ,配置状态 : ");
  Serial.println(EEPROM.read(0));
  
  if(EEPROM.read(0)==0){
    WiFi.softAP(SSID_LOCAL, PASSWRD_LOCAL);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }else{
    Serial.print("ssid : ");
    Serial.print(ssid);
    Serial.print(" ,password : ");
    Serial.println(password);
    wifiSSIDAndPassword_read(0);
    Serial.print("ssid : ");
    Serial.print(ssid);
    Serial.print(" ,password : ");
    Serial.println(password);
    wifiConnect(ssid,password);
  }
  
  
  
  esp8266_server.begin();                           // 启动网站服务
  esp8266_server.on("/", HTTP_GET, handleRoot);     // 设置服务器根目录即'/'的函数'handleRoot'
  esp8266_server.on("/BUTTON", HTTP_POST, handleSubmit);  // 设置处理按钮上传请求的函数'handleSubmit'
  esp8266_server.onNotFound(handleNotFound);        // 设置处理404情况的函数'handleNotFound'
}

void loop() {
  esp8266_server.handleClient();                     // 检查http服务器访问
}


//设置服务器根目录
void handleRoot() {
  esp8266_server.send(200,"text/html",htmlCode);
}
// 设置处理404情况的函数'handleNotFound'
void handleNotFound(){
  esp8266_server.send(404, "text/plain", "404: Not found"); // 发送 HTTP 状态 404 (未找到页面) 并向浏览器发送文字 "404: Not found"
}
//处理按钮上传请求的函数'handleSubmit'
void handleSubmit() {
  ssid = esp8266_server.arg(0);
  password = esp8266_server.arg(1);
  Serial.print("ssid : ");
  Serial.print(ssid);
  Serial.print(", password : ");
  Serial.print(password);

  wifiSSIDAndPassword_write(0);

  wifiSSIDAndPassword_read(0);
  
  wifiConnect(ssid,password);
  digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));// 改变LED的点亮或者熄灭状态
  esp8266_server.sendHeader("Location","/");          // 跳转回页面根目录
  esp8266_server.send(303);                           // 发送Http相应代码303 跳转  
}

//获取httpcode
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

void wifiConnect(String ssids,String passwords){
  char ssid[SSID_MAX_LENGTH];
  char password[PASSWORD_MAX_LENGTH];
  ssids.toCharArray(ssid,SSID_MAX_LENGTH);
  passwords.toCharArray(password,PASSWORD_MAX_LENGTH);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifiMulti.addAP(ssid, password);
  int i = 20;
  while (wifiMulti.run() != WL_CONNECTED&&i--) {
    Serial.print(".");
    delay(1000);
  }Serial.println('\n');
  if(wifiMulti.run() == WL_CONNECTED){
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
  }else{
    Serial.println("Failed!!! Please retry!");
  }
  
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

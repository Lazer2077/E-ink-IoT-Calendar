int dat = 9;
int clk = 10;


void setup ()
{
  pinMode(dat,OUTPUT);
  pinMode(clk,OUTPUT);
  digitalWrite(clk,LOW);

}
void loop()
{
  for(int i=0;i<24;i+=3)
{boradcast(i,2);
delay(10000);}
}
/* 语音播报模块  传入小时  天气 1为晴天 2为雨天  3为多云（阴）*/
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
  

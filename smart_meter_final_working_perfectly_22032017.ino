#include <EEPROM.h>
#include "EmonLib.h"
#include <LiquidCrystal.h>
#include <SPI.h>
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>


LiquidCrystal lcd(9, 8, 7,6,5,4);
  
EnergyMonitor emon1;
int i=0,z=0,saveaddress=50,costaddress,ack[7];
double start,t,cons,lastupdate,lastsave,room[7];
//char res[400];
float cost;
RF24 radio(2,3);
 
// Network uses that radio
RF24Network network(radio);
 
// Address of our node
const uint16_t this_node = 0;
 
// Address of the other node
const uint16_t other_node = 0;
 
// How often to send 'hello world to the other unit
const unsigned long interval = 2000; //ms
 
// When did we last send?
unsigned long last_sent;
 
// How many have we sent already
unsigned long packets_sent;
 struct packet{
  bool billpaid;
  float costperunit;
 };
// Structure of our payload
struct payload_t
{
  float kwh;
  int rno;
};
packet p;


void setup()
{
  costaddress=saveaddress+70;  
  lcd.begin(16, 2);
  emon1.current(A1, 30);
  start=0;
  lastupdate=0;
  lastsave=0;
  EEPROM.get(saveaddress, cons);
 if(isnan(cons))
 cons=0;
  EEPROM.get(costaddress, cost);
 if(isnan(cost))
 cost=5;
 lcd.print("Connecting!!!");
 lcd.setCursor(0,1);
 lcd.print("Cons=");
 lcd.print(cons);
 delay(15000);
 Serial.begin(9600);
 delay(2000);
 Serial.flush();
 
 Serial.println("AT+CGATT?");
 delay(1000);
 toSerial();
 Serial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
 delay(2000);
 toSerial();
 Serial.println("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"");
 delay(2000);
 toSerial();
 Serial.println("AT+SAPBR=1,1");
 delay(2000);
 toSerial();
 
 SPI.begin();
 radio.begin();
 network.begin(/*channel*/ 90, /*node address*/ other_node);

 for(z=0;z<7;z++)
 {
 room[z]=-1;
 ack[z]=-1;
 }
 lcd.clear();
 p = { false,cost };

}

void loop()
{
  lcd.setCursor(0,0);
  double Irms =emon1.calcIrms(1480);
  int asensor=analogRead(A1); 
  int sensorValue = analogRead(A0);
  double vold=0,volm=0;
  if(sensorValue!=0){
    vold=sensorValue*0.00488758553;
    volm=round(783.856797873973*vold-117.532585075125*vold*vold-1070.75376570224);
    //volm=783.856797873973*vold-117.532585075125*vold*vold-1070.75376570224

  }
  float power=volm*Irms;
  t=millis()-start;
  start=millis();
  t=t/1000;
  double temp;
  temp=t*power;
  temp=temp/1000;
  temp=temp/3600;
  cons=cons+temp;
  float concost=cons*cost;
  lcd.print(concost);
  lcd.print("|");
  lcd.print(volm);
  lcd.print("|");
  lcd.print(Irms);
    
  lcd.setCursor(0,1);
  lcd.print(cons);
  lcd.print("KWH|");
  lcd.print(power);
  if((millis()-lastupdate)>70000)
  {
    lcd.clear();
    lcd.print("Saving");
    int ret=0;
    while((ret=savetoserver("7899194973",cons))==0){}
    lcd.clear();
    lcd.print("Saved");
    lastupdate=millis();
    delay(1000);
    lcd.clear();
  }
  if((millis()-lastsave)>300000)
  {
    EEPROM.put(saveaddress, cons);
    lastsave=millis();
  }

  network.update();
  while ( network.available() )
  {
    // If so, grab it and print it out
    RF24NetworkHeader header;
    payload_t payload;
    network.read(header,&payload,sizeof(payload));
    room[payload.rno]=payload.kwh;
   }
   for(z=1;z<3;z++)
   {
    if(ack[z]!=-1)
    {
      network.update();
      //lcd.clear();
      //lcd.print("Sending...");
      RF24NetworkHeader header(/*to node*/ z);
      bool ok = network.write(header,&p,sizeof(packet));
      if(ok)
      ack[z]=-1;
    }
   }
   for(z=1;z<=7;z++)
   {
    if(room[z]!=-1)
    {
      lcd.print("R");
      lcd.print(z);
      lcd.print("=");
      lcd.print(room[z]);
      lcd.print("|");
    }
   }
}


  void toSerial()
{
  while(Serial.available()!=0)
  {
    char ch=Serial.read();
  }
}
void recSerial()
{
}

int savetoserver(String mno,double powr)
{
   char res[400];
   Serial.println("AT+HTTPINIT");
   delay(2000); 
   toSerial();

   String URL="AT+HTTPPARA=\"URL\",\"http://konsole.netau.net/update.php?meterno=";
   URL.concat(mno);
   URL.concat("&reading=");
   URL.concat(powr);
   for(z=1;z<7;z++)
  {
    if(room[z]!=-1)
    {
    URL.concat("&R");
   URL.concat(z);
   URL.concat("=");
   URL.concat(room[z]);
    }
  }
   URL.concat("\"");
   Serial.println(URL);
   delay(2000);
   toSerial();

   Serial.println("AT+HTTPACTION=0");
   delay(6000);
   toSerial();

   Serial.println("AT+HTTPREAD"); 
   delay(1000);
   //recSerial();
   
  while(Serial.available()!=0 && i<400)
  {
    res[i++]=Serial.read();
  }
   Serial.println("");
   Serial.println("AT+HTTPTERM");
   toSerial();
   delay(300);
   Serial.println("");
  char *x;
   if(x=strstr(res,"aaa"))
   {
  
    char c[10];
    int i=0;
    while(x[i]=='a')
    i++;
    int j=0;
    while(x[i]!='a')
    {
      c[j]=x[i];
      i++;
      j++;
    }
    //Serial.println(x);
    //Serial.println(c);
    String aa=String(c);
    float price=aa.toFloat();
    //Serial.println(p);
    p.billpaid=false;
    p.costperunit=price;
    cost=price;
    EEPROM.put(costaddress,price);
    for(z=1;z<7;z++)
    ack[z]=1;
    } 

   if(strstr(res,"reset2414"))
   {
   cons=0;
   EEPROM.put(saveaddress,cons);
   p.billpaid=true;
   for(z=1;z<7;z++)
    ack[z]=1;
    
   }

   
   if(strstr(res,"recordinserted2414")){
   //Serial.println("record inserted");
   i=0;
   return 1;
   }
   else
   {
    //Serial.println("record not inserted");
    i=0;
    return 0;
   }
   
   delay(10000);
   

}

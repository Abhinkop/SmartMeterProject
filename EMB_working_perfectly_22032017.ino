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
int i=0,saveaddress=50,costaddress;   
double start,t,cons,lastupdate,lastsave;
char res[200];
float cost;
RF24 radio(2,3);
 
// Network uses that radio
RF24Network network(radio);
 
// Address of our node
const uint16_t this_node = 2;
 
// Address of the other node
const uint16_t other_node = 0;
 struct packet{
  bool billpaid;
  float costperunit;
 };
packet p;
 
// Structure of our payload
struct payload_t
{
  float kwh;
  int rno;
};

void setup()
{  
  lcd.begin(16, 2);
  emon1.current(A1, 30);
  start=0;
  lastupdate=0;
  lastsave=0;
  costaddress=saveaddress+70;
  
  EEPROM.get(saveaddress, cons);
  if(isnan(cons))
  cons=0;
  EEPROM.get(costaddress, cost);
  if(isnan(cost))
  cost=5;
 
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  p= { false ,cost };
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
    volm=round((564.423216946423*vold-65.0640675071626*vold*vold-987.430686280131));
    //volm=564.423216946423*vold-65.0640675071626*vold*vold-987.430686280131

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
  if((millis()-lastsave)>300000)
  {
    EEPROM.put(saveaddress, cons);
    lastsave=millis();
  }
  network.update();
  //lcd.clear();
  lcd.print("Sending...");
  payload_t payload = { cons, this_node };
  RF24NetworkHeader header(/*to node*/ other_node);
  bool ok = network.write(header,&payload,sizeof(payload));
  //lcd.clear();
  network.update();
  while ( network.available() )
  {
    // If so, grab it and print it out
    RF24NetworkHeader header;
    payload_t payload;
    network.read(header,&p,sizeof(packet));
    if(p.billpaid==true)
    {
      cons=0;
      EEPROM.put(saveaddress,cons);
      p.billpaid=false;
    }
    if(p.costperunit!=cost)
    {
      cost=p.costperunit;
      EEPROM.put(costaddress,cost);
    }
   }
  
}


  

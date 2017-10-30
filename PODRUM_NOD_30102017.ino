/*
 Reconnecting MQTT example - non-blocking
 This sketch demonstrates how to keep the client connected
 using a non-blocking reconnect function. If the client loses
 its connection, it attempts to reconnect every 5 seconds
 without blocking the main loop.
 03.06.2016. - SA E5CN-a
 15.07.2016.
 19.07.2016. - Probano radi sve ! Dodat je i izlaz 36 sa Arduino Mega na Rele4 ali se ne koristi
 15.08.2016 -  Dodato i dobro ocitavanje pritiska vode 0-25bar.
 26.10.2017 -  revizija - izbaceno DHT22 a ubacena jos jedna sonda DS18B20
 
Probano i radi bez diskonektovanja !!!!! :P
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>

//#include <Wire.h>  // Comes with Arduino IDE
//#include "DHT.h"

// IZLAZI

#define PODRUM_SVETLO           22
#define PODRUM_VENTILACIJA      24
#define PODRUM_GREJANJE         26 
#define PODRUM_JUNE_ZADAVANJE   2    

//  ULAZI

#define PODRUM_PROZOR                    28
#define PODRUM_NIVO_VODE_GREJANJA        A0   
#define PODRUM_PRITISAK_VODOVOD          A1
#define PODRUM_TEMP_PODRUMA              31  // Senzor DS18B20 
#define PODRUM_TEMP_GREJANJE             38  // Senzor DS18B20
#define PODRUM_KVALIET_VAZDUHA           A2  // Senzor za gas MQ-2


// SWF , 9600, 8 Bit, 1 Stop, NoNe Parity, 1 Address 
#define TX_ENABLE_PIN 35         // define the EIA-485 transmit driver pin ( RE + DE pin's modules)
SoftwareSerial rs485(30, 34);    // SoftwareSerial rs485(RO, DI); pins name on the module

OneWire oneWire1(PODRUM_TEMP_GREJANJE), oneWire2(PODRUM_TEMP_PODRUMA);
DallasTemperature DS18B20_G(&oneWire1), DS18B20_P(&oneWire2);

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 30);
IPAddress server(192, 168, 1, 100);

long lastMsg = 0;
int  value = 0;

long lastMsg1 = 0;
int  value1 = 0;

char Buf[10];

float tempG;
float tempOldG = 0;

float tempP;
float tempOldP = 0;

float vodaPice;

int ko1=1;      // Kontakt prozora u podrumu
int old_ko1=0;

int Tpomocni, TpomocniStaro;  // Temperatura E5CN

//******* E5CN ****************************************************

   
   char msgE[] = {'@','0','1','W','S','0','1', '1','6','3','2','4','2','*','\r'}; // podatak za temperaturu 163.2 C za testiranje
   
     
   char msgRbuf[50];
   byte crcR;
   int  dig1,dig2,dig3,dig4, u;
   int  aUlaz=0, aUlazStaro ; 


//************************************************************************************

EthernetClient ethClient;
PubSubClient client(ethClient);

void callback(char* topic, byte* payload, unsigned int length) {
    int i=0;
    char message_buff[100];

    for (int i = 0; i < length; i++) {
       message_buff[i] = (char)payload[i];
//     Serial.print((char)payload[i]);
    }

    payload[length] = '\0';
    String strPayload = String((char*)payload);
   
   Serial.println("");
   Serial.print(String((char*)topic));
   Serial.print("[");
   Serial.print(strPayload);
   Serial.println("]");


 if (String((char*)topic) == "kuca/Podrum/sv1") {         // Svetlo u podrumu
      if (strPayload == "on") { 
        digitalWrite(PODRUM_SVETLO,LOW);
        Serial.println("Ukljuceno je svetlo u podrumu");
                              }
       else       {         
        digitalWrite(PODRUM_SVETLO,HIGH);  
        Serial.println("Iskljuceno je svetlo u podrumu");
                  }
 }

if (String((char*)topic) == "kuca/Podrum/sv2") {         // Ventilacija u podrumu
      if (strPayload == "on") { 
        digitalWrite(PODRUM_VENTILACIJA,LOW);
        Serial.println("Ukljucena je ventilacija u podrumu");
                              }
       else       {         
        digitalWrite(PODRUM_VENTILACIJA,HIGH);  
        Serial.println("Iskljucena je ventilacija u podrumu");
                  }
 }


if (String((char*)topic) == "kuca/Podrum/sv3") {         // Grejanje / Pec JUNE
      if (strPayload == "on") { 
        digitalWrite(PODRUM_GREJANJE,LOW);
        Serial.println("Ukljuceno je grejanje peci JUNE");
                              }
       else       {         
        digitalWrite(PODRUM_GREJANJE,HIGH);  
        Serial.println("Iskljuceno je grejanje peci JUNE");
                  }
 }


 if (String((char*)topic) == "kuca/Podrum/VoGrejSet") {  // Zadavanje temperature - kotao JUNE

       Serial.println("");
       Tpomocni = strPayload.toInt();
       Serial.print("Zadata temperatura vode grejanja - kotao JUNE : ");
       Serial.print(strPayload.toInt());
       Serial.println("");
             
           }
 
 
  
}




boolean reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
          Serial.println("Pokusavam da se konektujem na MQTT...");
          client.connect("Klijent_Podrum");
          delay(10);
          // Attempt to connect
    if (client.connect("Klijent_Podrum")) {
        Serial.println("konektovan");
         // Once connected, publish an announcement...
        client.publish("Klijent_Podrum", "Klijent PODRUM je konektovan");
         // ... and resubscribe
        client.subscribe("kuca/Podrum/#");
    } else {
      Serial.print("greska, rc=");
      Serial.print(client.state());
      Serial.println(" pokusacu ponovo za 2 sekunde");
      // Wait 3 seconds before retrying
      delay(2000);
    }
  }
return client.connected();


/*  
  if (client.connect("Klijent_Podrum")) {
    
    // Once connected, publish an announcement...
    
    client.publish("Klijent_Podrum","Klijent PODRUM je konektovan");
    Serial.println("Klijent PODRUM je konektovan");
    // ... and resubscribe
    client.subscribe("kuca/Podrum/#");
  }
  return client.connected();
 */
}

void setup()
{
  pinMode(PODRUM_SVETLO,         OUTPUT);
  pinMode(PODRUM_VENTILACIJA,    OUTPUT);
  pinMode(PODRUM_GREJANJE,       OUTPUT); 
  pinMode(PODRUM_JUNE_ZADAVANJE, OUTPUT);     

  pinMode(PODRUM_PROZOR,               INPUT_PULLUP);      
  pinMode(PODRUM_NIVO_VODE_GREJANJA,   INPUT);      
  pinMode(PODRUM_PRITISAK_VODOVOD,     INPUT);      
  pinMode(PODRUM_TEMP_PODRUMA,         INPUT_PULLUP);      
  pinMode(PODRUM_KVALIET_VAZDUHA,      INPUT); 
  pinMode(PODRUM_TEMP_GREJANJE,        INPUT); 

// inicijalizacija izlaza - Ugasi sve izlaze

 digitalWrite(PODRUM_SVETLO,         HIGH);
 digitalWrite(PODRUM_VENTILACIJA,    HIGH);
 digitalWrite(PODRUM_GREJANJE,       HIGH);
 digitalWrite(PODRUM_JUNE_ZADAVANJE, HIGH);

  

  pinMode(TX_ENABLE_PIN, OUTPUT);   // driver output enable
  rs485.begin (9600);               // vitesse com

  Serial.begin(115200);
  
  client.setServer(server, 1883);
  client.setCallback(callback);

  Ethernet.begin(mac, ip);
  delay(1500);
  
 
// Posalji trenutno stanje digitalnih ulaza na openHAB

ko1 = digitalRead(PODRUM_PROZOR);
client.publish("kuca/Podrum/ko1",( ko1 == 1) ? "OPEN" : "CLOSED" );

}

void loop() {

// ***** TEMP E5CN *********
      byte crcR =0;
      char charBuf[16];   
      u=0; 
      
     dig1= 0;
     dig2= Tpomocni / 10;
     dig3= Tpomocni % 10;
     dig4= 0;
   
     msgE[7] =  '48';
     msgE[8] =  dig2+48;
     msgE[9] =  dig3+48;
     msgE[10]=  '48';

// ********************* CRC kalkulacija *************************   

  byte m = msgE[0] ^ msgE[1]^ msgE[2] ^ msgE[3] ^ msgE[4] ^ msgE[5] ^ msgE[6] ^ msgE[7] ^ msgE[8] ^ msgE[9] ^ msgE[10];  // Decimalno
  
  String m1=String(m,HEX);
  m1.toUpperCase();

  String msgT1= String("@01WS01");
  String msgT2= String(dig1);  // broj 0
  String msgT3= String(dig2);  // prva cifra temperature
  String msgT4= String(dig3);  // druga cifra temperature
  String msgT5= String(dig4);  // broj 0
  String msgT6= String("*\r\0");
 
  String msgT= msgT1 + msgT2 + msgT3 + msgT4 + msgT5 +m1 +msgT6 ;

  msgT.toCharArray(charBuf, 16) ;

//******** Zadavanje  TEMPERATURE ******************************

if (Tpomocni != TpomocniStaro)  {
  
  digitalWrite (TX_ENABLE_PIN, HIGH);   // Omoguci slanje ZADATE TEMPERATURE
  delay(30);
  Serial.println("");
  Serial.print("Salje na rs485: "); 
  
    for (int k = 0; k < 15; k++) {
       Serial.print(charBuf[k]);                
       rs485.write(charBuf[k]);
                                 }   
}

  TpomocniStaro = Tpomocni;
 
//************************************************************************  
  if (!client.connected()) {
    Serial.println("Izgubljena je konekcija - pozivam...reconnect() !");
    reconnect();
    delay(100);
        }

//******* Da li je prozor u podrumu  otvoren ili zatvorena ? *************
  
  ko1 = digitalRead(PODRUM_PROZOR);
  if ( old_ko1 != ko1) {
          client.publish("kuca/Podrum/ko1",( ko1 == 1) ? "OPEN" : "CLOSED" );  
      }
     old_ko1 = ko1;
//**********************************************************************
//****** Petlja za slanje podataka na 3 sekunde ************************

 long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    ++value;

//****** Saljem temperaturu podruma ***********************************
  
          do {
          DS18B20_P.requestTemperatures(); 
          tempP = DS18B20_P.getTempCByIndex(0);
//        Serial.print("Temperatura podruma je : ");
//        Serial.println(temp);
          delay(10);
        } while (tempP == 85.0 || tempP == (-127.0));

        
  if (tempOldP != tempP) {
         client.publish("kuca/tempPod",dtostrf(tempP,4,2,Buf));         
         Serial.print("Temperatura podruma je: ");
         }
      tempOldP = tempP;
      
//***********************************************************
//****** Saljem Kvalitet vazduha u podrumu   **************** 

  int vazduh = analogRead(PODRUM_KVALIET_VAZDUHA) / 10.24;  // Skalirano od 0 - 100%
  client.publish("kuca/kvaPod", dtostrf(vazduh,4,2,Buf));
  Serial.print("Kvalitet vazduha u podrumu  je:  ");
  Serial.println(vazduh);
     
//  snprintf (vazduhCh, 75, "%ld", vazduh);  

//****** Saljem Nivo vode grejanja u podrumu **************** 

  float vodaGrej = analogRead(PODRUM_NIVO_VODE_GREJANJA) / 10.24;  // Skalirano od 0 - 100%
  client.publish("kuca/nivoVoGrej", dtostrf(vodaGrej,4,2,Buf));  
  Serial.print("Nivo vode za grejanje u podrumu  je:  ");
  Serial.println(vodaGrej);
  
//****** Saljem Pritisak vode za pice u podrumu *************   

if (analogRead(PODRUM_PRITISAK_VODOVOD)>204) {
  
  float vodaPice = (analogRead(PODRUM_PRITISAK_VODOVOD) - 204) / 32.76; // Skalirano od 0 - 25bar
  client.publish("kuca/pritVoPice", dtostrf(vodaPice,4,2,Buf)); 
  Serial.print("Pritisak vode za pice u podrumu  je:  ");
  Serial.println(vodaPice);
 
      }

else  {

  vodaPice = 0;  // U koliko je struja manja od 4mA
  client.publish("kuca/pritVoPice", dtostrf(vodaPice,4,2,Buf)); 
  Serial.print("Pritisak vode za pice u podrumu  je:  ");
  Serial.println(vodaPice);  
}

//****** Saljem temperaturu grejanja peci VAILLANT *************
  
          do {
          DS18B20_G.requestTemperatures(); 
          tempG = DS18B20_G.getTempCByIndex(0);
//        Serial.print("Temperatura grejanja je : ");
//        Serial.println(temp);
          delay(10);
        } while (tempG == 85.0 || tempG == (-127.0));

        
  if (tempOldG != tempG) {
         client.publish("kuca/tempVoGrej",dtostrf(tempG,4,2,Buf));
         Serial.print("Temperatura vode grejanja je: ");       
         }
      tempOldG = tempG;
      
//*****************************************************************************************
//******** Javi openHAB-u da si ziv :)*****************************************************

  client.publish("kuca/Podrum/NodPodRandom",dtostrf(random(0,10),1,0,Buf) );

  }
//***** Na svakih 1 minut uploduj sa openHAB-a trenutna stanja 10000 = 1.40 minuta ********

long now1 = millis();
  if (now1 - lastMsg1 > 100000) {
    lastMsg1 = now1;
    ++value1;

    client.subscribe("kuca/Podrum/#");

     }
  
  client.loop(); 
  delay(500); 
}

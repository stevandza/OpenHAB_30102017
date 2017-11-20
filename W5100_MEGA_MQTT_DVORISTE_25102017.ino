/* 
 DVORISTE
 Kombinacija sa MEGA2560  i Ethernet Shield plocom W5100 

 Proba RX-a sa serijskom komunikacijom  08.02.2017. - OK
 Prima pun kod sa daljinskog i salje ga na openHAB
 
 Reconnecting MQTT example - non-blocking

 This sketch demonstrates how to keep the client connected
 using a non-blocking reconnect function. If the client loses
 its connection, it attempts to reconnect every 5 seconds
 without blocking the main loop.

 Revizija 25.10.2017
 Revizija 27.10.2017.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <SparkFunTSL2561.h>

#include <Wire.h>  // Comes with Arduino IDE
#include "DHT.h"

//#include <OneWire.h>
//#include <DallasTemperature.h>

SFE_TSL2561 light; // Kreiramo objekat

// IZLAZI

#define DIMER_DVORISTE       8     //  OUT -   
#define DVORISNA_RASVETA     23    //  OUT -   
#define SVETLO_IS_GARAZA     25    //  OUT -   
#define SVETLO_IS_KUCE       27    //  OUT -   
#define GARAZA_VRATA_RELE    29    //  OUT -     

// ULAZI

#define PRISUSTVO_OSOBE      22     //  IN - 
#define GARAZA_VRATA_STATE   26     //  IN -


//  20, 21 , SDA, SCL, INT1, INT0  - Ulazi za senzor svetla TSL 2561     
   
#define GARAZA_DHTPIN        24     //  IN  - DHT22  Vlaznost i temperatura napolju
#define DHTTYPE DHT22
DHT dht(GARAZA_DHTPIN, DHTTYPE);

byte mac[]    = {  0xDA, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 31);
IPAddress server(192, 168, 1, 100);

EthernetClient ethClient;
PubSubClient client(ethClient);

boolean gain;     // Gain setting, 0 = X1, 1 = X16;
unsigned int ms;  // Integration ("shutter") time in milliseconds

long lastMsg  = 0;
long lastMsg1 = 0;

char Buf[20];

int svd1=0;      //  Dimer rasvete na dvoristu
int svn1=0;      //  Nivo svetla napolju

int ko1=1;       // Garazna vrata Stanje
int ko2=1;       // Prisustvo osobe
int ko3=1;       // Garazna vrata Rele

int old_ko1=0;
int old_ko2=0;
int old_ko3=0;

double lux;       // Resulting lux value
boolean good;     // True if neither sensor is saturated


String broj;
unsigned long Ibroj;

void printError(byte error)      // Ispis greske ukoliko nije ocitano stanje senzora svetla
{
  Serial.print("I2C error: ");
  Serial.print(error,DEC);
  Serial.print(", ");
  
  switch(error)
  {
    case 0:
      Serial.println("success");
      break;
    case 1:
      Serial.println("data too long for transmit buffer");
      break;
    case 2:
      Serial.println("received NACK on address (disconnected?)");
      break;
    case 3:
      Serial.println("received NACK on data");
      break;
    case 4:
      Serial.println("other error");
      break;
    default:
      Serial.println("unknown error");
  }
}

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



  if (String((char*)topic) == "kuca/Dvo/sv1") {         // Dvorisna rasveta
      if (strPayload == "on") { 
        digitalWrite(DVORISNA_RASVETA,LOW);
        Serial.println("Ukljuceno je svetlo na dvoristu");
                              }
       else       {         
        digitalWrite(DVORISNA_RASVETA,HIGH);  
        Serial.println("Iskljuceno je svetlo na dvoristu");
                  }
 }

  if (String((char*)topic) == "kuca/Dvo/svd1") {       //  Dimer - Dvorisna rasveta

       analogWrite(DIMER_DVORISTE,255 - strPayload.toInt()*2.55 );   
       Serial.println("");
       Serial.print("Nivo svetla na dvoristu je: ");
       Serial.print(strPayload.toInt());
       Serial.println("");
                     
           }
 
 
 if (String((char*)topic) == "kuca/Dvo/sv2") {          // Svetlo ispred garaze
     if (strPayload == "on") { 
        digitalWrite(SVETLO_IS_GARAZA,LOW);
        Serial.println("Ukljuceno je svetlo ispred garaze");
                              }
       else       {         
        digitalWrite(SVETLO_IS_GARAZA,HIGH);  
        Serial.println("Iskljuceno je svetlo ispred garaze");
                  }                          
 }
 
 if (String((char*)topic) == "kuca/Dvo/sv3") {          // Svetlo  ispred kuce
     if (strPayload == "on") { 
        digitalWrite(SVETLO_IS_KUCE,LOW);
        Serial.println("Ukljuceno je svetlo ispred kuce");
                              }       
       else       {         
        digitalWrite(SVETLO_IS_KUCE,HIGH);  
        Serial.println("Iskljuceno je svetlo ispred kuce");
                  }  
 }
 
 if (String((char*)topic) == "kuca/Dvo/sv4") {    // Zakljucavanje / otkljucavanje vrata garaze
     if (strPayload == "on") { 
        digitalWrite(GARAZA_VRATA_RELE,LOW);
        Serial.println("Vrata su zakljucana");
                              }      
       else       {         
        digitalWrite(GARAZA_VRATA_RELE,HIGH);  
        Serial.println("Vrata su otkljucana");
                  }  
 }
  
}


boolean reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
          Serial.println("Pokusavam da se konektujem na MQTT...");
          client.connect("Klijent_Dvoriste");
          delay(10);
          // Attempt to connect
    if (client.connect("Klijent_Dvoriste")) {
        Serial.println("konektovan");
         // Once connected, publish an announcement...
        client.publish("Klijent_Dvoriste", "Klijent DVORISTE je konektovan");
         // ... and resubscribe
        client.subscribe("kuca/Dvo/#");
    } else {
      Serial.print("greska, rc=");
      Serial.print(client.state());
      Serial.println(" pokusacu ponovo za 2 sekunde");
      // Wait 3 seconds before retrying
      delay(2000);
    }
  }
return client.connected();

}

void setup()
{

    pinMode(DVORISNA_RASVETA, OUTPUT);       // Dvorisna rasveta
    pinMode(DIMER_DVORISTE,   OUTPUT);       // Dvorisna rasveta - DIMMER
    pinMode(SVETLO_IS_GARAZA, OUTPUT);       // Svetlo ispred garaze
    pinMode(SVETLO_IS_KUCE,   OUTPUT);       // Svetlo ispred kuce
    pinMode(GARAZA_VRATA_RELE,OUTPUT);       // Zakljucavanje / otkljucavanje vrata od garaze

    pinMode(GARAZA_VRATA_STATE,INPUT_PULLUP); // Vrata garaze - Reed kontakt 
    pinMode(PRISUSTVO_OSOBE,INPUT_PULLUP);    // Prisustvo osobe u garazi
    pinMode(GARAZA_DHTPIN,INPUT_PULLUP);      // DHT22 - senzor
    
    
// Inicijalizacija izlaza - Ugasi sve izlaze pri dolasku napajanja

    digitalWrite(DVORISNA_RASVETA, HIGH);
    digitalWrite(SVETLO_IS_GARAZA, HIGH); 
    digitalWrite(SVETLO_IS_KUCE,   HIGH); 
    digitalWrite(GARAZA_VRATA_RELE,HIGH);
    

   Serial.begin(115200);
   client.setServer(server, 1883);
   client.setCallback(callback);

   Serial2.begin(9600);   

   Ethernet.begin(mac, ip);
   delay(1500);

//**** Paramerisanje senzora za svetlo TSL2561 *********
   light.begin();
   
   unsigned char ID;
  
  if (light.getID(ID))
  {
    Serial.print("Got factory ID: 0X");
    Serial.print(ID,HEX);
    Serial.println(", should be 0X5X");
  }
  
  else
  {
    byte error = light.getError();
    printError(error);
  }

  gain = 0;
  unsigned char time = 2;
  Serial.println("Set timing...");
  light.setTiming(gain,time,ms);
  
  Serial.println("Powerup...");
  light.setPowerUp();
      
//************************************************************************** 
// Posalji trenutno stanje digitalnih ulaza na openHAB

ko3 = digitalRead(GARAZA_VRATA_RELE);
client.publish("kuca/Dvo/sv4/state",( ko3 == 1) ? "OPEN" : "CLOSED" ); 
old_ko3 = ko3;
 
}

void loop() {

  if (!client.connected()) {
    Serial.println("Izgubljena je konekcija - pozivam...reconnect() !");
    reconnect();
    delay(50);
        }
        
//****  Serijska komunikacija sa RF PRIJEMNIKA *****************************
  if (Serial2.available()) {    
     broj = Serial2.readStringUntil('E');    // String
     Serial.println(broj);
     Ibroj = broj.toInt();                   // Unsigned long
     client.publish("kuca/rfKod", dtostrf(Ibroj,5,0,Buf));        // Prosledjuje primljeni KOD sa Arduino Nano na HAB !       
 }
  
//******* Da li su vrata GARAZE  otvorena ili zatvorena ? **************
  int ko1 = digitalRead(GARAZA_VRATA_STATE);
  if ( old_ko1 != ko1) { client.publish("kuca/Dvo/ko1",( ko1 == 1) ? "OPEN" : "CLOSED" ); }
      old_ko1 = ko1;
      
//**********************************************************************
//******* Da li su vrata garaze  OTKLJUCANA ili ZAKLJUCANA ? ***********
// Provera stanja izlaza Arduino MEGA 2560 modula i slanje stanja na OpenHAB

  int ko3 = digitalRead(GARAZA_VRATA_RELE);
  if ( old_ko3 != ko3) { client.publish("kuca/Dvo/sv4/state",( ko3 == 1) ? "OPEN" : "CLOSED" );  }
     old_ko3 = ko3;
     
//************** Provera prisustva osobe ***************************
  int ko2 = digitalRead(PRISUSTVO_OSOBE);
  if (old_ko2 != ko2 ) { client.publish("kuca/Dvo/ko2", ( ko2 == 1) ? "OPEN" : "CLOSED" ); }   
     old_ko2 = ko2;

//****** Petlja za slanje podataka na 5 sekundi *************
 long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;


//****** Ocitavanje sa DHT22 senzora ********************************************
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t) ) {
    delay(20);
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Vlaznost napolju je (DHT): ");
  Serial.println(h);
  client.publish("kuca/vlaDvo",dtostrf(h,4,2,Buf));
  
  Serial.print("Temperatura napolju je (DHT): ");
  Serial.println(t);
  client.publish("kuca/tempNap",dtostrf(t,4,2,Buf));

//**** Ocitavanje sa senzora svetla TSL2561 *************************************
 unsigned int data0, data1;
 if (light.getData(data0,data1))
  {
       
    good = light.getLux(gain,ms,data0,data1,lux);
    if (good) Serial.print("(GOOD)"); else Serial.print("(BAD)");
  }
  else
  {
    byte error = light.getError();
    printError(error);
  }

 
  Serial.print("Nivo merenog dnevnog svetla je:");
  Serial.println(lux);
  client.publish("kuca/svn1",dtostrf(lux,3,1,Buf)); 
  

//******** Javi openHAB-u da si ziv :)  **************************************************** 
 client.publish("kuca/Dvo/NodDvoRandom",dtostrf(random(0,10),1,0,Buf) );
  
  }

  
//***** Na svakih 1 minut uploduj sa openHAB-a trenutna stanja 10000 = 1.40 minuta ********
long now1 = millis();
  if (now1 - lastMsg1 > 10000) {
    lastMsg1 = now1;
    Serial.println(" ");    
    Serial.println("Redovno ocitavanje stanja prekidaca sa HAB-a !");
    client.subscribe("kuca/Dvo/#");

  }
  


  client.loop(); 
  delay(50); 

}

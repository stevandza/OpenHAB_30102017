/*
Pir senzor svetla ispod terase na dvoristu.
MQTT : "kuca/Priz/ko3"   ---->  OPEN / CLOSE
MQTT : "kuca/Priz/tempPrizHod"
MAP  :  pir.map
30.05.2017
 Na samom senzoru je i povratna informacija o stanju dvorisne rasvete navezano za PIR senzor !
 Iz nekog razloga ova verzija ne gubi konekciju pretpostavljam da svaki klijent mora imati svoje ime    client.connect("ESP8266ClientPirPriz")!
 
*/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define PIR_PRIZEMLJE_HODNIK    14      //  IN - PIR - D5
#define TEMP_PRIZEMLJE_HODNIK   12      //  IN - DS18B20 - D6
#define OUT_PIR_PRIZ_HOD        10      //  OUT - SDD3

unsigned long previousMillis = 0;
const long interval = 8000;            //  Interval slanja podataka

OneWire oneWire(TEMP_PRIZEMLJE_HODNIK);
DallasTemperature DS18B20(&oneWire);
DeviceAddress insideThermometer;


const char* ssid = "Zozon";
const char* password = "zozon1978";
const char* mqtt_server = "192.168.1.100";

int channel = 11;


WiFiClient espClient;
PubSubClient client(espClient);


int ko1=1; 
int old_ko1=0; 

float temp;
float tempOld = 0;
char msg1[10];

void setup_wifi() {
  
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}



void callback(char* topic, byte* payload, unsigned int length) {
  int i=0;
  char message_buff[100];
  
//  Serial.print("Message arrived [");
//  Serial.print(topic);
//  Serial.print("] ");

  for (int i = 0; i < length; i++) {
       message_buff[i] = (char)payload[i];
//     Serial.print((char)payload[i]);
    }
   payload[length] = '\0';
   String strPayload = String((char*)payload);
   
//   Serial.println("");
//   Serial.print(String((char*)topic));
//   Serial.print("[");
//   Serial.print(strPayload);
//   Serial.println("]");

     
  if (String((char*)topic) == "kuca/Priz/ko3") {         // Prisustvo osobe u prizemlju
      if (strPayload == "OPEN") {                        // Ovjfjhfjfjyfyjfyf
        Serial.println("Neko se seta u prizemlju");
        digitalWrite(OUT_PIR_PRIZ_HOD, LOW);
                              }
       else {digitalWrite(OUT_PIR_PRIZ_HOD, HIGH);}
 }
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266ClientPirPriz")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("PIR_ESP8266", "hello world");
      // ... and resubscribe
      client.subscribe("kuca/Priz/ko3");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  pinMode(OUT_PIR_PRIZ_HOD,     OUTPUT);
  pinMode(PIR_PRIZEMLJE_HODNIK, INPUT_PULLUP);     // Senzor za svetlo
  pinMode(TEMP_PRIZEMLJE_HODNIK,INPUT_PULLUP);     // Temperatura prizemlja
  digitalWrite(OUT_PIR_PRIZ_HOD, HIGH);
  
  DS18B20.begin();
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop() {
  
  digitalWrite(OUT_PIR_PRIZ_HOD, LOW);
  
  unsigned long currentMillis = millis();
 
  if (!client.connected()) {
    reconnect();
        }
        
  client.loop();
  delay(200);
  
  int ko1 = digitalRead(PIR_PRIZEMLJE_HODNIK);
  if (old_ko1 != ko1 ) {
     if ( ko1 == 1)   { client.publish("kuca/Priz/ko3", "OPEN");
                        Serial.println("PIR je OPEN");
                       }
     
     else           {  client.publish("kuca/Priz/ko3", "CLOSED");  
                       Serial.println("PIR je CLOSED");
                       }
  }
     old_ko1 = ko1; 


//****** Periodicno slanje temperature u hodniku prizemlja *************

 if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    digitalWrite(OUT_PIR_PRIZ_HOD, LOW);
    
   do {
          DS18B20.requestTemperatures(); 
          temp = DS18B20.getTempCByIndex(0);
          delay(10);
       } while (temp == 85.0 || temp == (-127.0));

        
  if (tempOld != temp) {
         dtostrf(temp,4,2,msg1);
         Serial.print("");
         Serial.print("Temperatura prizemlja je: ");
         Serial.println(msg1);
         client.publish("kuca/Priz/tempPrizHod",msg1);
         }
      tempOld = temp;

//****** Periodicno slanje stanja PIR  senzora u hodniku   *************
   ko1 = digitalRead(PIR_PRIZEMLJE_HODNIK);
   if ( ko1 == 1)   { client.publish("kuca/Priz/ko3", "OPEN");
                        Serial.println("PIR je OPEN");
                       }
     
   else           {  client.publish("kuca/Priz/ko3", "CLOSED");  
                       Serial.println("PIR je CLOSED");
                       }
                       
//****** Periodicno slanje random broja 0-9 PIR senzora u hodniku   *************

   char msgRand1[2];
   dtostrf(random(0,10),1,0,msgRand1);
   client.publish("kuca/Priz/PrizHodPIRrandom",msgRand1 );
   
    }

    
}

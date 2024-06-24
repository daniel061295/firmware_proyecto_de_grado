//#include <SPI.h>
// Pins used for the connection with the sensor
#include <ArduinoJson.h>
#ifdef ESP8266
 #include <ESP8266WiFi.h>  // Pins for board ESP8266 Wemos-NodeMCU
 #else
 #include <WiFi.h>  
#endif

#include "time.h" 
#include <ESP32Time.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
//#include "soc/rtc_wdt.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 17     // Digital pin connected to the DHT sensor 

#define DHTTYPE    DHT22     // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

float temp;
float hum;
//######################DECLARANDO REGISTROS Y VARIABLES ADXL355############################################
//#define SCK 5
//#define MISO 19
//#define MOSI 27

ESP32Time rtc(3600);  // offset in seconds GMT+1
// Memory register addresses:
const String id_nodo = "0"; //RECUERDA AGREGAR TAMBIEN ESTE VALOR AL TOPIC
//double Ts = 0.05;
double Ts = 120;
// const int CHIP_SELECT_PIN = 17; // PARA EL ACELEROMETRO
const int csPin = 18;          // LoRa radio chip select




////---- WiFi settings
////const char* ssid = "PROYECTO";
//const char* password = "12345678";
////const char* ssid = "12345";
////const char* password = "DAVIDVAS";
//const char* ssid = "Daniel";
////const char* password = "123456789";


//wifi casa para pruebas
const char* ssid = "FAMILIA TORRES CARDENAS";
const char* password = "YULIANAAMORMIO";



//---- MQTT Broker settings
const char* mqtt_server = "a33454d332054780b8feaf83950ed54a.s2.eu.hivemq.cloud";  // replace with your broker url
const char* mqtt_username = "danielcardenaz";
const char* mqtt_password = "Manzana2132881";
const int mqtt_port = 8883;
const char* topic_mqtt= "SHM_PROYECTO/MEDICIONES/0";         //<---------------------- AGREGAR id_nodo AL FINAL 
const char* topic_status_online = "SHM_PROYECTO/STATUS/0/REQUEST";    //<---------------------- AGREGAR id_nodo AL FINAL 
const char* topic_status_online_true = "SHM_PROYECTO/STATUS/0/RESPONSE";//<---------------------- AGREGAR id_nodo AL FINAL 
const char* topic_trigger = "SHM_PROYECTO/TRIGGER";

//---- Time server settings
const char* ntpServer = "pool.ntp.org"; //Servidor para tiempo
const long  gmtOffset_sec = -18000;    //GTM pais  -5*60-60    
const int   daylightOffset_sec = -3600;  //Delay de hora
//const int   daylightOffset_sec = 0;  //SIN Delay

char dateTime[50];
char usec[30];


bool timeflag = false;
bool lectura_exitosa = false;

char payload_json[100] = "9999-12-31 23:59,9999-12-31 23:59";
char starting_time[50] = "9999-12-31 23:59:59";
char ending_time[50] = "9999-12-31 23:59:59";
String incommingMessage;

// Initialize JSON Document with apropiate size
DynamicJsonDocument doc(2048); //4096
int count;
String mensaje;


WiFiClientSecure espClient;   // for no secure connection use WiFiClient instead of WiFiClientSecure 
//WiFiClient espClient;
PubSubClient client(espClient);
double lastMsg = 0;


static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

TaskHandle_t Task1;
TaskHandle_t Task2;
//###################################################################
//FUNCIONES PRINCIPALES DEL CODIGO PARA CADA CORE
void Task1code( void * parameter); //MIDE
void Task2code( void * pvParameters ); //ENVIA
//void feed_rtc( void * pvParameters);
//==========================================
void setup_wifi() {
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //printLocalTime();
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }
}


//=====================================
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");

//      client.subscribe(topic_mqtt);   // subscribe the topics here
      client.subscribe(topic_trigger);   // subscribe the topics here
      client.subscribe(topic_status_online);   // subscribe the topics here
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);

  delay(100);
  while (!Serial) delay(1);
  setup_wifi();

  #ifdef ESP8266
    espClient.setInsecure();
  #else   // for the ESP32
    espClient.setCACert(root_ca);      // enable this line and the the "certificate" code for secure connection
  #endif
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);

  delayMS = Ts*1000;

  Serial.println("Iniciando la Task1...");
  xTaskCreatePinnedToCore(Task1code, "Task1", 100000, NULL, 5, &Task1,  0);
  Serial.println("Iniciando la Task2...");
  xTaskCreatePinnedToCore(Task2code, "Task2", 20000, NULL, 5, &Task2,  1);

  
//  xTaskCreatePinnedToCore(feed_rtc, "Feed_rtc", 10000, NULL, 0, NULL,  1); 

  }
 
//###################################### BEGIN LOOP #################################################

void loop() {


}

//###################################### END LOOP #################################################


//void callback(char* topic, byte* payload, unsigned int length) {
void callback(char* topic, byte* payload, unsigned int length) {
  if ( strcmp(topic,topic_trigger)== 0){
  incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];
  Serial.println(incommingMessage);
  incommingMessage.toCharArray(payload_json,100);
//  DynamicJsonDocument doc1(1024);
//  deserializeJson(doc1, incommingMessage);
   char * token = strtok(payload_json, ",");
   for(int i = 0; i<16;i++) starting_time[i]=token[i];
   starting_time[16]='\0';
   char * token2 = strtok(NULL, ",");
   for(int i = 0; i<16;i++) ending_time[i]=token2[i];
   ending_time[16]='\0';
 
  Serial.println("Message arrived ["+String(topic)+"]"+ + "starting at: " + starting_time + "to: " + ending_time);
  timeflag = false;
  }
  if ( strcmp(topic,topic_status_online)== 0){
  publishMessage(topic_status_online_true,"ONLINE",true);
  }
}

void publishMessage(const char* topic, String payload , boolean retained){

  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: " + payload);
}


void Task1code( void * parameter) {
  while (true) {
    vTaskDelay(20);
    if (!client.connected()) reconnect();
    client.loop();

    struct tm timeinfo = rtc.getTimeStruct();;
  
    struct timeval tv;
    if (gettimeofday(&tv, NULL)!= 0) {
      Serial.println("Failed to obtain time");
      return;
    }

    strftime(dateTime,50, "%Y-%m-%d %H:%M:%S", &timeinfo);
    sprintf(usec, "%ld", rtc.getMicros() );
 
//    strcat(dateTime,".");
//    strcat(dateTime,usec); 


//    ===================================== TIME FLAGS TO START OR STOP MEASSURAMENT==========================================================
    if ((strcmp(dateTime,starting_time) >  0)&&(timeflag == false)) {
      Serial.println("Triggered!");
      timeflag = true;
      }
    if ((strcmp(dateTime,ending_time) >  0)&&(timeflag == true)) {
    Serial.println("Timeout!");
    timeflag = false;
    char * start = "9999-12-31 23:59";
    for(int i = 0; i<16;i++) starting_time[i]=start[i];
    starting_time[16]='\0';
    for(int i = 0; i<16;i++) ending_time[i]=start[i];
    ending_time[16]='\0';
    }
//    =========================================== COLLECTING DATA ====================================================
  
  //if (timeflag == true ){ 
      //Serial.println(String(long(tv.tv_usec )/ 1000000.0));
      double rtc_clock = double(rtc.getEpoch()) + double(rtc.getMicros())/1000000;
      
      double now = rtc_clock;
      //Serial.println(now - lastMsg);
      if (now - lastMsg > Ts) {
        lastMsg = now;
        int count_lect = 0;
        lectura_exitosa = false;
        while (lectura_exitosa == false && count_lect < 3 ){
          bool temp_flag = false;
          bool hum_flag = false;
          sensors_event_t event;
          dht.temperature().getEvent(&event);
          if (isnan(event.temperature)) {
            temp = 0.0;
            Serial.println(F("Error reading temperature!"));
          }
          else {
            Serial.print(F("Temperature: "));
            temp = event.temperature;
            temp_flag = true;
            Serial.print(event.temperature);
            Serial.println(F("°C"));
          }
          dht.humidity().getEvent(&event);
          if (isnan(event.relative_humidity)) {
            hum = 0.0;
            Serial.println(F("Error reading humidity!"));
          }
          else {
            Serial.print(F("Humidity: "));
            hum = event.relative_humidity;
            hum_flag = true;
            Serial.print(event.relative_humidity);
            Serial.println(F("%"));
          }
          if ((temp_flag == true )&& (hum_flag == true)){
            Serial.println("AMBAS LECTURAS EXITOSAS, SALIENDO DE WHILE");
            lectura_exitosa = true;
            
            }
          else{
            Serial.println("INTENTO DE LECTURA FALLIDO"); 
            count_lect ++;
            Serial.print("Intentos:");
            Serial.println(count_lect);
            
            }
          delay(3000);
        }

        
//        sensors_event_t event;
//        dht.temperature().getEvent(&event);
//        if (isnan(event.temperature)) {
//          temp = 0.0;
//          Serial.println(F("Error reading temperature!"));
//        }
//        else {
//          Serial.print(F("Temperature: "));
//          temp = event.temperature;
//          Serial.print(event.temperature);
//          Serial.println(F("°C"));
//        }
//        dht.humidity().getEvent(&event);
//        if (isnan(event.relative_humidity)) {
//          hum = 0.0;
//          Serial.println(F("Error reading humidity!"));
//        }
//        else {
//          Serial.print(F("Humidity: "));
//          hum = event.relative_humidity;
//          Serial.print(event.relative_humidity);
//          Serial.println(F("%"));
//        }
          
  
          
          JsonObject doc_0 = doc.createNestedObject();
          doc_0["fecha"] = dateTime;
          doc_0["nodo"] = id_nodo;
          doc_0["temperatura"] = temp;
          doc_0["humedad"] = hum;
          //Serial.println(uxTaskGetStackHighWaterMark(NULL));
          count++;
      }  
    }
}


void Task2code( void * pvParameters ){
  while (true) {
    vTaskDelay(1000);
      if (count >= 1){
      Serial.print("Sending packet: ");
      serializeJson(doc, mensaje);
      Serial.println(mensaje);
      publishMessage(topic_mqtt,mensaje,true);
      doc.clear();
      mensaje = "";
      count = 0; 
    }
  
  }
}

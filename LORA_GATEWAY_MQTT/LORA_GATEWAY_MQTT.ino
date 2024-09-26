#include <ArduinoJson.h>
#ifdef ESP8266
#include <ESP8266WiFi.h> // Pins for board ESP8266 Wemos-NodeMCU
#else
#include <WiFi.h>
#endif

#include "time.h"
#include <ESP32Time.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include "heltec.h"

#define BAND 915E6 // you can set band here directly,e.g. 868E6,915E6

uint32_t delayMS;
double Ts = 120;
float temp;
float hum;
bool firstTimeMqtt = false;

const String id_nodo = "0";

ESP32Time rtc(3600); // offset in seconds GMT+1

const int csPin = 18; // LoRa radio chip select

// wifi casa para pruebas
// const char *ssid = "FAMILIA TORRES CARDENAS";2222
// const char *password = "YULIANAAMORMIO";
const char *ssid = "WMSAS-TALLER";
const char *password = "303wm2021";

//---- MQTT Broker settings
const char *mqtt_server = "a33454d332054780b8feaf83950ed54a.s2.eu.hivemq.cloud"; // replace with your broker url
const char *mqtt_username = "danielcardenaz";
const char *mqtt_password = "Manzana2132881";
const int mqtt_port = 8883;
const char *topic_mqtt = "SHM_PROYECTO/MEDICIONES/0"; //<---------------------- AGREGAR id_nodo AL FINAL

// const char* topic_status_online = "SHM_PROYECTO/STATUS/0/REQUEST";    //<---------------------- AGREGAR id_nodo AL FINAL
// const char* topic_status_online_true = "SHM_PROYECTO/STATUS/0/RESPONSE";//<---------------------- AGREGAR id_nodo AL FINAL
// const char* topic_trigger = "SHM_PROYECTO/TRIGGER";

//---- Time server settings
const char *ntpServer = "pool.ntp.org"; // Servidor para tiempo
const long gmtOffset_sec = -18000;      // GTM pais  -5*60-60
const int daylightOffset_sec = -3600;   // Delay de hora
// const int   daylightOffset_sec = 0;  //SIN Delay

char dateTime[50];
char usec[30];

bool timeflag = false;
bool lectura_exitosa = false;


// Initialize JSON Document with apropiate size
DynamicJsonDocument doc(2048); // 4096
int count;
String mensaje;
bool flag = false;
WiFiClientSecure espClient; // for no secure connection use WiFiClient instead of WiFiClientSecure
// WiFiClient espClient;
PubSubClient client(espClient);
double lastMsg = 0;

String packet ;
String vars[10];

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

void setup_wifi()
{
    delay(10);
    Serial.print("\nConnecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);

    Heltec.display->drawString(0, 0, "Connecting to: ");
    Heltec.display->drawString(0, 15, String(ssid));
    Heltec.display->display();
    randomSeed(micros());
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Heltec.display->drawString(0, 30, "Connected!");
    Heltec.display->display();
    Serial.println("\nWiFi connected\nIP address: ");
    Serial.println(WiFi.localIP());

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // printLocalTime();
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
    }
}

//=====================================
void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP8266Client-"; // Create a random client ID
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
        {
            Serial.println("connected");
            if (firstTimeMqtt == false){
            Heltec.display->drawString(0, 45, "MQTT:OK!");
            Heltec.display->display(); 
            firstTimeMqtt = true;
              }
            
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
      
    // register the receive callback
    LoRa.onReceive(onReceive);
  
    // put the radio into receive mode
    LoRa.receive();
    
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);

    delay(100);
    while (!Serial)
        delay(1);
    setup_wifi();

#ifdef ESP8266
    espClient.setInsecure();
#else // for the ESP32
    espClient.setCACert(root_ca); // enable this line and the the "certificate" code for secure connection
#endif

    client.setServer(mqtt_server, mqtt_port);
    delayMS = Ts * 1000;
}

void publishMessage(const char *topic, String payload, boolean retained)
{

    if (client.publish(topic, payload.c_str(), true))
        Serial.println("Message publised [" + String(topic) + "]: " + payload);
}


void loop()
{
  if (!client.connected())
  {
  
      reconnect();
      client.loop();
  }
struct tm timeinfo = rtc.getTimeStruct();
  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0)
  {
      Serial.println("Failed to obtain time");
      return;
  }

  if (flag == true){
    strftime(dateTime, 50, "%Y-%m-%d %H:%M:%S", &timeinfo);
  
    JsonObject doc_0 = doc.createNestedObject();
    doc_0["fecha"] = dateTime;
    doc_0["nodo"] = id_nodo;
    doc_0["temperatura"] = vars[0];
    doc_0["humedad"] = vars[1];
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));
    
    Serial.print("Sending packet: ");
    serializeJson(doc, mensaje);
    Serial.println(mensaje);
    publishMessage(topic_mqtt, mensaje, true);
    doc.clear();
    mensaje = "";
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);

    Heltec.display->drawString(0, 0, "Temperatura: ");
    Heltec.display->drawString(90, 0, String(vars[0]));
    Heltec.display->drawString(0, 15, "Humedad: ");
    Heltec.display->drawString(90, 15, String(vars[1]));
    Heltec.display->drawString(0, 30, "Timestamp:");
    Heltec.display->drawString(0, 45, String(String(dateTime)));
    Heltec.display->display();
    flag = false;
  }
}
void onReceive(int packetSize)
{
  // received a packet
  packet = "";
  int StringCount = 0;
  Serial.println("Received packet ");
  for (int i = 0; i < packetSize; i++) { packet += (char) LoRa.read(); }
   while (packet.length() > 0)
  {
    int index = packet.indexOf(',');
    if (index == -1) // No space found
    {
      vars[StringCount++] = packet;
      break;
    }
    else
    {
      vars[StringCount++] = packet.substring(0, index);
      packet = packet.substring(index+1);
    }
  }
  Serial.print("temperatura:");
  Serial.println(vars[0]);
  Serial.print("humedad:");
  Serial.println(vars[1]);
  flag = true;

}

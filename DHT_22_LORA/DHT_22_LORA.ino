
#include "heltec.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define BAND 915E6 // you can set band here directly,e.g. 868E6,915E6

#define DHTPIN 17 // Digital pin connected to the DHT sensor

#define DHTTYPE DHT22 // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);

float temp;
float hum;
unsigned long startMillis; // some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 60000*30;
bool lectura_exitosa = false;
void setup()
{
    Serial.begin(115200);
    startMillis = millis();

    dht.begin();
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    dht.humidity().getSensor(&sensor);
    Serial.println(F("DHTxx Unified Sensor Example"));
    // WIFI Kit series V1 not support Vext control
    Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
    Heltec.display->init();
    Heltec.display->flipScreenVertically();  
    Heltec.display->setFont(ArialMT_Plain_10);
    delay(1500);
    Heltec.display->clear();
  
    Heltec.display->drawString(0, 0, "Te amo yuli <3");
    Heltec.display->display();
    delay(1000);
}

void loop()
{
    currentMillis = millis();
    if (currentMillis - startMillis >= period)
    {
        int count_lect = 0;
        lectura_exitosa = false;
        while (lectura_exitosa == false && count_lect < 3)
        {
            bool temp_flag = false;
            bool hum_flag = false;
            sensors_event_t event;
            dht.temperature().getEvent(&event);
            if (isnan(event.temperature))
            {
                temp = 0.0;
                Serial.println(F("Error reading temperature!"));
            }
            else
            {
                Serial.print(F("Temperature: "));
                temp = event.temperature;
                temp_flag = true;
                Serial.print(event.temperature);
                Serial.println(F("°C"));
            }
            dht.humidity().getEvent(&event);
            if (isnan(event.relative_humidity))
            {
                hum = 0.0;
                Serial.println(F("Error reading humidity!"));
            }
            else
            {
                Serial.print(F("Humidity: "));
                hum = event.relative_humidity;
                hum_flag = true;
                Serial.print(event.relative_humidity);
                Serial.println(F("%"));
            }
            if ((temp_flag == true) && (hum_flag == true))
            {
                Serial.println("AMBAS LECTURAS EXITOSAS, SALIENDO DE WHILE");
                lectura_exitosa = true;
            }
            else
            {
                Serial.println("INTENTO DE LECTURA FALLIDO");
                count_lect++;
                Serial.print("Intentos:");
                Serial.println(count_lect);
            }
            delay(3000);
        }
        

        // send packet
        LoRa.beginPacket();
        /*
         * LoRa.setTxPower(txPower,RFOUT_pin);
         * txPower -- 0 ~ 20
         * RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
         *   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
         *   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
         */
        LoRa.setTxPower(14, RF_PACONFIG_PASELECT_PABOOST);
        LoRa.print(temp);
        LoRa.print(",");
        LoRa.print(hum);
        LoRa.endPacket();

        Heltec.display->clear();
        Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
        Heltec.display->setFont(ArialMT_Plain_10);
  
        Heltec.display->drawString(0, 0, "Temperatura: ");
        Heltec.display->drawString(90, 0, String(temp));
        Heltec.display->drawString(0, 30, "Humedad: ");
        Heltec.display->drawString(90, 30, String(hum));
        Heltec.display->display();
        startMillis = currentMillis;
    }
}

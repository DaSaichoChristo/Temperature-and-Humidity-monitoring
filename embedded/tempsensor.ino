#include "DHT.h"
#include "ArduinoJson.h"
#include <WiFi.h>
#include "secrets.h"

#define DHTPIN 11
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  
  delay(1000);
  Serial.begin(115200);
  
  dht.begin();
  Serial.println("DHT11 sensor test");
  
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.println("\nConnecting");
  
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to avoid printing wrong data)
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %");
}

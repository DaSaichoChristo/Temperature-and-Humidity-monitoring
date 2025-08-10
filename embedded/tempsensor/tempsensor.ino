#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <FirebaseJson.h>
#include "DHT.h"
#include "time.h"
#include "secrets.h"

#define DHTPIN 11
#define DHTTYPE DHT11

// DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Firebase Authentification
UserAuth user_auth(apiKeyFirebase, firebaseUserEmail, firebaseUserPasswd);

// Firebase Components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// timer variable to send data
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 30000; // 10 seconds in milliseconds

void initWifi() { 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passwd);
  
  Serial.println("\nConnecting");
  
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

struct tm getCurrentTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    memset(&timeinfo, 0, sizeof(timeinfo));
  }
  return timeinfo;
}


void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}

void writeJson(int temp, int humidity, tm timeinfo, AsyncClient &aClient){
  FirebaseJson json;
  long timestamp = (long)mktime(&timeinfo);
  json.set("timestamp", timestamp); 
  json.set("temperature", temp);
  json.set("humidity", humidity);
  String jsonPath = "/test/data/" + String(timestamp);
  String jsonStr;
  json.toString(jsonStr);

  object_t obj(jsonStr);

  Database.set<object_t>(aClient, jsonPath, obj, processData, "setJsonTask");
}

void setup() {
  
  delay(1000);
  Serial.begin(115200);
  
  dht.begin();
  Serial.println("DHT11 sensor test");
  
  delay(1000);

    initWifi();
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  // set local time by talking to ntp server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Configure SSL Client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);

  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(urlFirebaseProject);
}

void loop() {
  // maintain app loop for Authentification
  app.loop();
  if (app.ready()){
    // Periodic data sending every 10 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      // Update the last send time
      lastSendTime = currentTime;
      // collect temp and humidity
      int h = dht.readHumidity();
      int t = dht.readTemperature();
      Serial.print("temp = ");
      Serial.println(t);
      Serial.print("humi = ");
      Serial.println(h);
  
      // Check if any reads failed and exit early (to avoid printing wrong data)
      if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
      }

      writeJson(t, h, getCurrentTime(), aClient);
    }
  delay(2000);
  }
}

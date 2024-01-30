#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define WIFI_SSID "TechDrips"
#define WIFI_PASSWORD "TechDrips123456789"
#define API_KEY "AIzaSyCVxQkgu_DdXALFM38388Zb8ZcBe2Zuyxk"
#define USER_EMAIL "TechDrips@gmail.com"
#define USER_PASSWORD "TechDrips123456789"
#define DATABASE_URL "https://techdrips-ad1ce-default-rtdb.asia-southeast1.firebasedatabase.app/"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String uid;
String databasePath;
String rainPath = "/rain";
String waterLevelPath = "/waterLevel";
String motorPath = "/motorStatus";
String solenoidValvePath = "/valveStatus";
String timePath = "/timestamp";
String parentPath;
int timestamp;
FirebaseJson json;
const char* ntpServer = "pool.ntp.org";
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;
#define rainPin 13
#define trigPin 15
#define echoPin 2
#define motorPump 25
#define solenoidValve 27
int rain_state;
int fullTankDistance = 20;
int emptyTankDistance = 160;
int waterLevel;
long duration;
int distance;
bool motorStatus = false;
bool solenoidValveStatus = false;
bool rainStatus = false;

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}



unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}


void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(rainPin, INPUT);
  pinMode(motorPump, OUTPUT);
  pinMode(solenoidValve, OUTPUT);
  initWiFi();
  configTime(0, 0, ntpServer);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
  databasePath = "/TechDrips/" + uid + "/torenTech";
}

void loop() {
  waterLevelSystem();
  delay(1500);
  RainSensorSystem();
  delay(1500);

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath = databasePath + "/";
    json.set(rainPath.c_str(), String(rainStatus));
    json.set(waterLevelPath.c_str(), String(waterLevel));
    json.set(motorPath.c_str(), String(motorStatus));
    json.set(solenoidValvePath.c_str(), String(solenoidValveStatus));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
  delay(7000);
  ESP.restart();
}

void RainSensorSystem() {
  rain_state = digitalRead(rainPin);
  if (rain_state == HIGH) {
    Serial.println("The rain is NOT detected");
    solenoidValveStatus = true;
    rainStatus = false;
    digitalWrite(solenoidValve, HIGH);
    Serial.print("Solenoid Valve: ON : ");
    Serial.println(solenoidValveStatus);
  } else {
    Serial.println("The rain is detected");
    solenoidValveStatus = false;
    rainStatus = true;
    digitalWrite(solenoidValve, LOW );
    Serial.print("Solenoid Valve: OFF : ");
    Serial.println(solenoidValve);
  }
  delay(1000);
}

void waterLevelSystem() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  // Hitung jarak berdasarkan waktu pulsa
  distance = ((duration / 2) * 0.343) / 10;
  if (distance > (fullTankDistance - 15) && distance < emptyTankDistance) {
    waterLevel = map((int)distance, emptyTankDistance, fullTankDistance, 0, 100);
    Serial.print("Water Level: ");
    Serial.print(waterLevel);
    Serial.println("%");
    if (waterLevel < 90) {
      motorStatus = true;
      digitalWrite(motorPump, HIGH);
      Serial.print("Motor Pump: ON : ");
      Serial.println(motorStatus);
    } else if (waterLevel > 90) {
      motorStatus = false;
      digitalWrite(motorPump, LOW);
      Serial.println("Motor Pump: OFF : ");
      Serial.println(motorStatus);
    }
  }
  Serial.print("Jarak :");
  Serial.println(distance);
  delay(1000);

}

/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-date-time-ntp-client-server-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include "time.h"

const char* ssid     = "TechDrips";
const char* password = "TechDrips123456789";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop() {
  delay(1000);
  printLocalTime();
}

// Function that gets current epoch time
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);


  // Menambahkan angka 7 ke timeHour
  int hourOffset = 7;
  int currentHour = atoi(timeHour);
  currentHour = (currentHour + hourOffset) % 24; // Menghindari nilai yang melebihi 24 jam
  sprintf(timeHour, "%02d", currentHour);

  
  char timeWeekDay[10];
  strftime(timeWeekDay, 10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  char timeYear[6];
  strftime(timeYear, 10, "%Y", &timeinfo);
  Serial.println(timeYear);
  char timeMonth[12];
  strftime(timeMonth, 12, "%B", &timeinfo);
  Serial.println(timeMonth);
  char timeDate[3];
  strftime(timeDate, 3, "%d", &timeinfo);
  Serial.println(timeDate);
  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  Serial.println(timeMinute);
  Serial.println();
  String timeInfo = String(timeWeekDay) + ", " + String(timeDate) + " " + String(timeMonth) + " " + String(timeYear) + ", " + String(timeHour) + ":" + String(timeMinute);

  Serial.print("Time Info : ");
  Serial.println(timeInfo);
}

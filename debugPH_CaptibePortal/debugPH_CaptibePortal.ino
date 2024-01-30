#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <Preferences.h>
#include "BluetoothSerial.h"
#include "FS.h"
#include <SPIFFS.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include <DHT.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
const char *pin = "123456789"; // Change this to more secure PIN.
String device_name = "TECH-DRIPS PLANT-TECH";
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif
BluetoothSerial SerialBT;

#define API_KEY "AIzaSyCVxQkgu_DdXALFM38388Zb8ZcBe2Zuyxk"
#define USER_EMAIL "TechDrips@gmail.com"
#define USER_PASSWORD "TechDrips123456789"
#define DATABASE_URL "https://techdrips-ad1ce-default-rtdb.asia-southeast1.firebasedatabase.app/"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  SerialBT.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    SerialBT.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    SerialBT.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      SerialBT.print("  DIR : ");
      Serial.println(file.name());
      SerialBT.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      SerialBT.print("  FILE: ");
      Serial.print(file.name());
      SerialBT.print(file.name());
      Serial.print("\tSIZE: ");
      SerialBT.print("\tSIZE: ");
      Serial.println(file.size());
      SerialBT.println(file.size());
    }
    file = root.openNextFile();
  }
}

Preferences preferences;
DNSServer dnsServer;
AsyncWebServer server(80);
IPAddress apIP(8, 8, 4, 4);
String timeInfo;
String ssid;
String password;
bool is_setup_done = false;
bool valid_ssid_received = false;
bool valid_password_received = false;
bool wifi_timeout = false;
String uid;
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String pHPath = "/pHPlant";
String timePath = "/timestamp";
String parentPath;
FirebaseJson json;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
#define pH_Pin 35
#define DHT_PIN 15
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);
float outputValue, humidityPlant, temperaturePlant;
// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;

class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
      //request->addInterestingHeader("ANY");
      return true;
    }

    void handleRequest(AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false);
    }
};

void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html", false);
    Serial.println("Client Connected");
    SerialBT.println("Client Connected");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css", false);
  });

  server.on("/bgOrnament.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bgOrnament.png", "image/png", false);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("ssid")) {
      inputMessage = request->getParam("ssid")->value();
      inputParam = "ssid";
      ssid = inputMessage;
      Serial.println(inputMessage);
      SerialBT.println(inputMessage);
      valid_ssid_received = true;
    }

    if (request->hasParam("password")) {
      inputMessage = request->getParam("password")->value();
      inputParam = "password";
      password = inputMessage;
      Serial.println(inputMessage);
      SerialBT.println(inputMessage);
      valid_password_received = true;
    }
    request->send(200, "text/html", "The values entered by you have been successfully sent to the device. It will now attempt WiFi connection");
  });
}


void WiFiSoftAPSetup()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP("PLANT-TECH-WIFI-MANAGER");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
}


void WiFiStationSetup(String rec_ssid, String rec_password)
{
  wifi_timeout = false;
  WiFi.mode(WIFI_STA);
  char ssid_arr[20];
  char password_arr[20];
  rec_ssid.toCharArray(ssid_arr, rec_ssid.length() + 1);
  rec_password.toCharArray(password_arr, rec_password.length() + 1);
  Serial.print("Received SSID: ");
  SerialBT.print("Received SSID: ");
  Serial.println(ssid_arr);
  SerialBT.println(ssid_arr);
  Serial.print("And password: ");
  SerialBT.print("And password: ");
  Serial.println(password_arr);
  SerialBT.println(password_arr);
  WiFi.begin(ssid_arr, password_arr);

  uint32_t t1 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
    SerialBT.print(".");
    if (millis() - t1 > 50000) //50 seconds elapsed connecting to WiFi
    {
      Serial.println();
      SerialBT.println();
      Serial.println("Timeout connecting to WiFi. The SSID and Password seem incorrect.");
      SerialBT.println("Timeout connecting to WiFi. The SSID and Password seem incorrect.");
      valid_ssid_received = false;
      valid_password_received = false;
      is_setup_done = false;
      preferences.putBool("is_setup_done", is_setup_done);

      StartCaptivePortal();
      wifi_timeout = true;
      break;
    }
  }
  if (!wifi_timeout)
  {
    is_setup_done = true;
    Serial.println("");
    SerialBT.println("");
    Serial.print("WiFi connected to: ");
    SerialBT.print("WiFi connected to: ");
    Serial.println(rec_ssid);
    SerialBT.println(rec_ssid);
    Serial.print("IP address: ");
    SerialBT.print("IP address: ");
    Serial.println(WiFi.localIP());
    SerialBT.println(WiFi.localIP());
    preferences.putBool("is_setup_done", is_setup_done);
    preferences.putString("rec_ssid", rec_ssid);
    preferences.putString("rec_password", rec_password);
  }
}

void StartCaptivePortal() {
  Serial.println("Setting up AP Mode");
  SerialBT.println("Setting up AP Mode");
  WiFiSoftAPSetup();
  Serial.println("Setting up Async WebServer");
  SerialBT.println("Setting up Async WebServer");
  setupServer();
  Serial.println("Starting DNS Server");
  SerialBT.println("Starting DNS Server");
  dnsServer.start(53, "*", apIP);
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  server.begin();
  dnsServer.processNextRequest();
}


// Function that gets current epoch time
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    SerialBT.println("Failed to obtain time");
    ESP.restart();
  }
  Serial.println("Time variables");
  SerialBT.println("Time variables");
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);
  SerialBT.println(timeHour);
  // Menambahkan angka 7 ke timeHour
  int hourOffset = 7;
  int currentHour = atoi(timeHour);
  currentHour = (currentHour + hourOffset) % 24; // Menghindari nilai yang melebihi 24 jam
  sprintf(timeHour, "%02d", currentHour);
  char timeWeekDay[10];
  strftime(timeWeekDay, 10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  SerialBT.println(timeWeekDay);
  char timeYear[6];
  strftime(timeYear, 10, "%Y", &timeinfo);
  Serial.println(timeYear);
  SerialBT.println(timeYear);
  char timeMonth[12];
  strftime(timeMonth, 12, "%B", &timeinfo);
  Serial.println(timeMonth);
  SerialBT.println(timeMonth);
  char timeDate[3];
  strftime(timeDate, 3, "%d", &timeinfo);
  Serial.println(timeDate);
  SerialBT.println(timeDate);
  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  Serial.println(timeMinute);
  SerialBT.println(timeMinute);
  Serial.println();
  SerialBT.println();
  timeInfo = String(timeWeekDay) + ", " + String(timeDate) + " " + String(timeMonth) + " " + String(timeYear) + ", " + String(timeHour) + ":" + String(timeMinute);
  Serial.print("Time Info: ");
  SerialBT.print("Time Info: ");
  Serial.println(timeInfo);
  SerialBT.println(timeInfo);
}

void setup() {
  //your other setup stuff...
  Serial.begin(115200);
  SerialBT.begin(device_name);
  SerialBT.setPin(pin);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    SerialBT.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //List contents of SPIFFS
  listDir(SPIFFS, "/", 0);

  Serial.println();
  SerialBT.println();
  preferences.begin("my-pref", false);

  is_setup_done = preferences.getBool("is_setup_done", false);
  ssid = preferences.getString("rec_ssid", "Sample_SSID");
  password = preferences.getString("rec_password", "abcdefgh");

  if (!is_setup_done)
  {
    StartCaptivePortal();
  }
  else
  {
    Serial.println("Using saved SSID and Password to attempt WiFi Connection!");
    SerialBT.println("Using saved SSID and Password to attempt WiFi Connection!");
    Serial.print("Saved SSID is "); Serial.println(ssid);
    SerialBT.print("Saved SSID is "); SerialBT.println(ssid);
    Serial.print("Saved Password is "); Serial.println(password);
    SerialBT.print("Saved Password is "); SerialBT.println(password);
    WiFiStationSetup(ssid, password);
  }

  while (!is_setup_done)
  {
    dnsServer.processNextRequest();
    delay(10);
    if (valid_ssid_received && valid_password_received)
    {
      Serial.println("Attempting WiFi Connection!");
      SerialBT.println("Attempting WiFi Connection!");
      WiFiStationSetup(ssid, password);
    }
  }

  Serial.println("All Done!");
  SerialBT.println("All Done!");
  dht.begin();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
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
  SerialBT.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    SerialBT.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  SerialBT.print("User UID: ");
  Serial.println(uid);
  SerialBT.println(uid);
  databasePath = "/TechDrips/" + uid + "/plantTech";
}

void loop() {
  //Your Loop code
  printLocalTime();
  if (isnan(humidityPlant) && isnan(temperaturePlant)) {
    Serial.println("Failed to read sensor");
    SerialBT.println("Failed to read sensor");
    ESP.restart();
  }
  outputValue = (-0.0139 * (analogRead(pH_Pin) / 1000)) + 7.7851;
  humidityPlant = dht.readHumidity();
  temperaturePlant = dht.readTemperature();
  Serial.print("Nilai Analog Input :");
  SerialBT.print("Nilai Analog Input :");
  Serial.println(analogRead(pH_Pin));
  SerialBT.println(analogRead(pH_Pin));
  Serial.print("Nilai PH : ");
  SerialBT.print("Nilai PH : ");
  Serial.println(outputValue);
  SerialBT.println(outputValue);
  Serial.print("Nilai Humidty Plant :");
  SerialBT.print("Nilai Humidty Plant :");
  Serial.println(humidityPlant);
  SerialBT.println(humidityPlant);
  Serial.print("Nilai Temperature Plant: ");
  SerialBT.print("Nilai Temperature Plant: ");
  Serial.println(temperaturePlant);
  SerialBT.println(temperaturePlant);
  Serial.println("Sending to Database");
  SerialBT.println("Sending to Database");
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    parentPath = databasePath + "/";

    json.set(tempPath.c_str(), String(temperaturePlant));
    json.set(humPath.c_str(), String(humidityPlant));
    json.set(pHPath.c_str(), String(outputValue));
    json.set(timePath, String(timeInfo));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    SerialBT.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
  delay(7000);
  ESP.restart();
}

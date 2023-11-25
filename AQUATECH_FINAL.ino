#include <OneWire.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DallasTemperature.h>

#define WIFI_SSID "WMSU ICS"
#define WIFI_PASSWORD "facultyunion"
#define API_KEY "AIzaSyC3dJOUk9CTBf7ptrPrI7UH_eImYHiG_og"
#define DATABASE_URL "https://aquatech-1d9b9-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define ONE_WIRE_BUS 4 // PIN FOR WATER TEMPERATURE SENSOR
#define MQ135_THRESHOLD_1 1000 // Fresh Air threshold

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0; // Corrected the variable declaration
bool signupOK = false;
float PH_LEVEL;
int pH_Value;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient client;

unsigned long last_time = 0;
unsigned long Delay = 30000;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi"); // Added a missing semicolon
  while (WiFi.status() != WL_CONNECTED) { // Corrected the while loop condition
    Serial.print("Failed to connect"); 
    delay(1000);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("signUp OK");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str()); // Added missing '\n' and a semicolon
  }
  // config.token_status_callback = tokenStatusCallBack; // Commented out for now as it's not defined
  Firebase.begin(&config, &auth); // Corrected the variable name from 'confi' to 'config'
  Firebase.reconnectWiFi(true);
  
  pinMode(pH_Value, INPUT); 
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
  
    // WATER TEMPERATURE CODE:
    sensors.requestTemperatures();
    float WATER_TEMP = sensors.getTempCByIndex(0);
  
    // PH LEVEL CODE:
    pH_Value = analogRead(36);
    PH_LEVEL = pH_Value * (5.0 / 1023.0);
  
    // AMMONIA LEVEL CODE:
    int AMMONIA = analogRead(39);

    // SEND DATA TO FIREBASE:
    if (Firebase.RTDB.setFloat(&fbdo, "Sensor/WATER_TEMP", WATER_TEMP)) {
      Serial.println();
      Serial.print(WATER_TEMP);
      Serial.print("Successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED : " + fbdo.errorReason()); // Corrected string concatenation
    }
    
    if (Firebase.RTDB.setFloat(&fbdo, "Sensor/PH_LEVEL", PH_LEVEL)) {
      Serial.println();
      Serial.print(PH_LEVEL);
      Serial.print("Successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED : " + fbdo.errorReason()); // Corrected string concatenation
    }
    
    if (Firebase.RTDB.setInt(&fbdo, "Sensor/AMMONIA", AMMONIA)) {
      Serial.println();
      Serial.print(AMMONIA);
      Serial.print("Successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED : " + fbdo.errorReason()); // Corrected string concatenation
    }
  }
  delay(5000);
}

float readTemperature() {
  float WATER_TEMP = sensors.getTempCByIndex(0);
}

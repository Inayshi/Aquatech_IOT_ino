#include <OneWire.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DallasTemperature.h>
#include <PubSubClient.h> //library for MQTT
#include <ESP32Servo.h>


#define WIFI_SSID "B0DAK YELL0W"
#define WIFI_PASSWORD "grasshopper120801"
#define API_KEY "AIzaSyC3dJOUk9CTBf7ptrPrI7UH_eImYHiG_og"
#define DATABASE_URL "https://aquatech-1d9b9-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define ONE_WIRE_BUS 4 // PIN FOR WATER TEMPERATURE SENSOR
#define MQ135_THRESHOLD_1 1000 // Fresh Air threshold
#define echoPin 26               // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
#define trigPin 27              // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
#define RELAY_PIN 18            // PUMP  1
#define RELAY_PIN2 19            // PUMP 2
#define SERVO_PIN 2             // SERVO MOTOR

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;



const char* mqttServer = "broker.emqx.io"; //MQTT URL
const char* mqttUserName = "aquatech";  // MQTT username
const char* mqttPwd = "aquatech_iot";  // MQTT password
const char* clientID = "aquatechIOT"; // client id username+0001
const char* topic = "aquatech/pump"; //publish topic


//for sensors
unsigned long sendDataPrevMillis = 0; // Corrected the variable declaration
bool signupOK = false;
float PH_LEVEL;
int pH_Value;

//for water changing mechanism 
long duration, distance;

//for servo motor

int pos = 0; 

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
PubSubClient client(espClient);
Servo servoMotor;

unsigned long last_time = 0;
unsigned long Delay = 30000;
//parameters for using non-blocking delay
unsigned long previousMillis = 0;
const long interval = 5000;
 
String msgStr = "";      // MQTT message buffer

void reconnect() {
  while (!client.connected()) {  
    if (client.connect(clientID, mqttUserName, mqttPwd)) {
      Serial.println("MQTT connected");
 
      client.subscribe("aquatech/pump");
      client.subscribe("aquatech/servo");
      Serial.println("Successfully subscribed to topics.");
     
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);  // wait 5sec and retry
    }
  }
}

void callback(char*topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String data = "";

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    data += (char)payload[i];
  }

  Serial.println();
  Serial.print("Message size :");
  Serial.println(length);
  Serial.println();
  Serial.println("-----------------------");
  Serial.println(data);

  if (data == "1") { //THIS WILL TAKE WATER FROM THE TANK
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Received message. Pumping out water from tank.");
      }

  if(data=="2")
  {
        for (pos = 0; pos <= 180; pos += 1) 
        { 
          servoMotor.write(pos); 
          delay(10);  
        }          

        for (pos = 180; pos >= 0; pos -= 1) 
        { 
         servoMotor.write(pos);   
          delay(10);             
        }
  }

}

void setup() {

  client.setServer(mqttServer, 1883); //setting MQTT server
  client.setCallback(callback);
  
  Serial.begin(9600);
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
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(RELAY_PIN2, HIGH);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  servoMotor.setPeriodHertz(50); // PWM frequency for SG90
  servoMotor.attach(SERVO_PIN, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180
}


 
void loop() {
 
  if (!client.connected()) { //if client is not connected
    reconnect(); //try to reconnect
  }
  client.loop();
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //PUMP1:
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      
      duration = pulseIn(echoPin, HIGH);
      distance = duration / 58.2;
      String disp = String(distance);
      Serial.print (distance);
      Serial.print("Distance: ");
      Serial.print(disp);

      
      if (distance >= 15 ) {
      Serial.println("Distance is greater or equal to 15cm. Pump will now stop taking out water.");
      digitalWrite(RELAY_PIN, HIGH);
      }

      delay(3000);
      
      if (distance == 15 ) {
      Serial.println("Distance is equal to 15cm. Pump will now start refilling tank.");
      digitalWrite(RELAY_PIN2, LOW);
      }

      if (distance <= 3 ) {
      Serial.println("Distance is greater or equal to 3cm. Pump will now stop refilling tank.");
      digitalWrite(RELAY_PIN2, HIGH);
      }

    
      
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

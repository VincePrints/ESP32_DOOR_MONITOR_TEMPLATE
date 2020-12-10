#include <WiFi.h>
#include <PubSubClient.h>

//Deep sleep wake up variable
#define BUTTON_PIN_BITMASK 0x100000000 //2^32[GPIO PIN] in hex 

// WIFI setup
#define WIFISSID "[]" // Put your WifiSSID here
#define PASSWORD "[]" // Put your wifi password here
#define TOKEN "[]" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "[]" // MQTT Client Name, 8-12 alphanumeric character ASCII string; 
                              //Unique ASCII string and different from all other devices
const char * VARIABLE_LABEL_1 = "ReedSwitch"; // Assing the variable label
const char * VARIABLE_LABEL_2 = "VBat"; // Assign the variable label
const char * DEVICE_LABEL = "esp32"; // Assig the device label

// Declare I/O devices
#define SENSOR 32
#define LED_BUILTIN 5

// MQTT variables
char mqttBroker[]  = "things.ubidots.com";
char payload[100];
char topic[150];

// Space to store values to send
char str_reedSwitch[10];
char str_VBat[10];

//Call clients
WiFiClient ubidots;
PubSubClient client(ubidots);

void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);
  Serial.println(topic);
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("Connected.");
    } else {
      Serial.print("Failed, RC=");
      Serial.print(client.state());
      Serial.println(" Trying again in 2 seconds");
      delay(2000);
    }
  }
}

void blinkLED(int j, int pWidth) {
  for(int i = 0; i <= j; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(pWidth);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(pWidth);
  }
}

void setup() {
  Serial.begin(115200);

  //Declare I/O
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  
  //Declare how "wake up" will occur
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32,0); //1 = High, 0 = Low

  //Connect and confirm WIFI
  WiFi.begin(WIFISSID, PASSWORD);
  Serial.println();
  Serial.print("Wait for WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  //Successful connection feedback
  blinkLED(3, 100);
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);  
}

void loop() {
  //Check for connection
  if (!client.connected()) {
    reconnect();
  }

  //Read sensors
  int reedSwitch = digitalRead(SENSOR);
  float VBat = analogRead(34);
  VBat = ((VBat*1.63)/1000);

  //Convert float to character array
  dtostrf(reedSwitch, 4, 2, str_reedSwitch);
  dtostrf(VBat, 4, 2, str_VBat);

  //Format and post data
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\": %s,", VARIABLE_LABEL_1, str_reedSwitch); // Adds the variable label
  sprintf(payload, "%s\"%s\": %s}", payload, VARIABLE_LABEL_2, str_VBat); // Adds the variable label
  Serial.println("Publishing data to Ubidots Cloud");
  Serial.print("reedSwitch = ");
  Serial.println(reedSwitch);
  Serial.print("VBat = ");
  Serial.println(VBat);
  Serial.println("payload");
  Serial.println(payload);
  client.publish(topic, payload);
  client.loop();
  delay(50);

  //Decide if deep sleep should enable
  if (reedSwitch == 1) {
    Serial.println("Door is locked, going to sleep...");
    delay(1000);
    esp_deep_sleep_start();
    Serial.println("Deep sleep failed");
  }
  Serial.println("Deep sleep not activated");
  delay(30000);
}

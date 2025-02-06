#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

// Set AP credentials
#define AP_SSID "ESP8266"
#define AP_PASS "magicword"

bool isWifiInit = false;


const int SSID_SIZE = 32;      // Maximum size for SSID
const int PASSWORD_SIZE = 32;  // Maximum size for password
const int IP_SIZE = 16;

char ssid[SSID_SIZE];          // Buffer to hold the Wi-Fi SSID
char password[PASSWORD_SIZE];  // Buffer to hold the Wi-Fi password
char mqttBrokerIP[IP_SIZE];    // Buffer to hold the MQTT broker IP


const char *mqtt_topic = "temperature/topic";  // MQTT topic
const char *mqtt_username = "admin";           // MQTT username for authentication
const char *mqtt_password = "admin@123456";    // MQTT password for authentication
const int mqtt_port = 1883;

const int BUFFER_SIZE = 20;  // Buffer size for IP address


WiFiClient espClient;

PubSubClient mqtt_client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


//I2C Connect To Slave ESP8266
#define SDA_PIN 4
#define SCL_PIN 5
const int16_t I2C_MASTER = 0x42;

const int16_t I2C_SLAVE_1 = 0x08;
const int16_t I2C_SLAVE_2 = 0x09;

const int ledWifiConnection = 12;  // LED for status of wifi conections
const int ledMqttConnection = 14;  // LED for status of MQTT conections

void setup() {

  Serial.begin(115200);

  // Initialize the digital pin as an output
  pinMode(ledWifiConnection, OUTPUT);
  pinMode(ledMqttConnection, OUTPUT);

  delay(10);

  Serial.println("");

  EEPROM.begin(512);  // Initialize EEPROM with 512 bytes
                      // Write zeros to all bytes in the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);  // Write 0 to each byte
  }

  // Commit the changes to EEPROM
  EEPROM.commit();

  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  // Read the stored Wi-Fi settings and broker IP from EEPROM
  readWiFiSettings();

  readBrokerIP();

  // Start the web server
  server.on("/", handleRoot);
  server.on("/wifi", handleWiFiSettings);
  server.on("/broker", handleBrokerSettings);
  server.begin();
  Serial.println("HTTP server started");

  connectToWiFi();
}

void connectToWiFi() {

  WiFi.begin(ssid, password);  // Connect to the Wi-Fi network

  int timeout = 0;

  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");

    if (timeout > 10) {  // maybe ssid or password is incorrect
      isWifiInit = false;
      Serial.println("Can not Connected to Wi-Fi!");
      return;
    }
    timeout++;
  }

  digitalWrite(ledWifiConnection, HIGH);
  Serial.println("Connected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupI2CBus() {
  Wire.begin(SDA_PIN, SCL_PIN, I2C_MASTER);  // join i2c bus (address optional for master)
}

long lastReconnectAttempt = 0;

void loop() {

  server.handleClient();

  if (isWifiInit) {

    if (!mqtt_client.connected()) {  // mqtt_reconnect non blocking
      digitalWrite(ledMqttConnection, LOW);

      unsigned long now = millis();
      if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;

        // Attempt to reconnect
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
    } else {
      digitalWrite(ledMqttConnection, HIGH);
      mqtt_client.loop();

      long now = millis();
      if (now - lastMsg > 2000) {
        lastMsg = now;
        ++value;
        snprintf(msg, MSG_BUFFER_SIZE, "temperature #%ld", value);
        Serial.print("Publish message: ");
        Serial.println(msg);

        mqtt_client.publish(mqtt_topic, msg);
      }
    }
  }
}

boolean reconnect() {

  if (!mqtt_client.connected()) {

    mqtt_client.setServer(mqttBrokerIP, mqtt_port);
    mqtt_client.setCallback(mqttCallback);

    if (mqtt_client.connect("esp8266Client", mqtt_username, mqtt_password)) {
      mqtt_client.publish("outTopic", "hello world");
      mqtt_client.subscribe("inTopic");
    }
  }

  return mqtt_client.connected();
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}


void readSensors() {
  uint8_t error;

  // Serial.printf("Read Temperature off slave 1", address.size());
  //Read 16 bytes from the slave
  uint8_t bytesReceived = Wire.requestFrom(I2C_SLAVE_1, 6);  // request 6 bytes from slave 1
  Serial.printf("requestFrom: %u\n", bytesReceived);

  if ((bool)bytesReceived) {  //If received more than zero bytes
    uint8_t temp[bytesReceived];

    Wire.readBytes(temp, bytesReceived);
    Serial.printf("Buffer: %s\n", temp);
  }

  Wire.requestFrom(I2C_SLAVE_1, 6);  // request 6 bytes from slave device #8

  while (Wire.available()) {  // slave may send less than requested
    char c = Wire.read();     // receive a byte as character
    Serial.print(c);          // print the character
  }
}

void handleRoot() {
  String html = "<h1>ESP8266 Configuration</h1>";
  html += "<h2>Wi-Fi Settings</h2>";
  html += "<form action=\"/wifi\" method=\"POST\">";
  html += "SSID: <input type=\"text\" name=\"ssid\" value=\"" + String(ssid) + "\"><br>";
  html += "Password: <input type=\"password\" name=\"password\" value=\"" + String(password) + "\"><br>";
  html += "<input type=\"submit\" value=\"Connect\">";
  html += "</form>";

  html += "<h2>MQTT Broker Settings</h2>";
  html += "<form action=\"/broker\" method=\"POST\">";
  html += "Broker IP: <input type=\"text\" name=\"ip\" value=\"" + String(mqttBrokerIP) + "\"><br>";
  html += "<input type=\"submit\" value=\"Save Broker\">";
  html += "</form>";

  server.send(200, "text/html", html);
}

void handleWiFiSettings() {
  isWifiInit = false;
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");

    // Write the new Wi-Fi settings to EEPROM
    writeWiFiSettings(newSSID.c_str(), newPassword.c_str());

    // Attempt to connect to the new Wi-Fi
    connectToWiFi();

    server.send(200, "text/html", "Wi-Fi settings saved! Attempting to connect...");
  } else {
    server.send(400, "text/html", "Invalid Request");
  }
}


void handleBrokerSettings() {
  if (server.hasArg("ip")) {
    String newIP = server.arg("ip");

    // Validate the IP address
    if (isValidIP(newIP)) {
      newIP.toCharArray(mqttBrokerIP, IP_SIZE);  // Update the broker IP
      writeBrokerIP(mqttBrokerIP);               // Write the new IP to EEPROM
      readBrokerIP();
      server.send(200, "text/html", "Broker IP set to: " + newIP);
    } else {
      server.send(400, "text/html", "Invalid IP address: " + newIP + ". Please enter a valid IPv4 address.");
    }
  } else {
    server.send(400, "text/html", "Invalid Request");
  }
}

void readWiFiSettings() {
  // Read the SSID and password from EEPROM
  for (int i = 0; i < SSID_SIZE; i++) {
    ssid[i] = EEPROM.read(i);
  }
  ssid[SSID_SIZE - 1] = '\0';  // Null-terminate the string

  for (int i = 0; i < PASSWORD_SIZE; i++) {
    password[i] = EEPROM.read(i + SSID_SIZE);  // Password starts after SSID
  }
  password[PASSWORD_SIZE - 1] = '\0';  // Null-terminate the string

  // Check if SSID and password are initialized
  if (strlen(ssid) == 0 || strlen(password) == 0) {
    Serial.println("Wi-Fi settings not initialized. Please configure the SSID and password.");
    // You can set default values or prompt for new settings here
    // For example, you could set:
    // strcpy(ssid, "defaultSSID");
    // strcpy(password, "defaultPassword");
    isWifiInit = false;
  } else {
    Serial.print("SSID read from EEPROM: ");
    Serial.println(ssid);
    Serial.print("Password read from EEPROM: ");
    Serial.println(password);
    isWifiInit = true;
  }
}

void writeWiFiSettings(const char *newSSID, const char *newPassword) {
  // Write the SSID to EEPROM
  for (int i = 0; i < SSID_SIZE; i++) {
    EEPROM.write(i, newSSID[i]);

    ssid[i] = newSSID[i];
  }

  // Write the password to EEPROM
  for (int i = 0; i < PASSWORD_SIZE; i++) {
    EEPROM.write(i + SSID_SIZE, newPassword[i]);
    password[i] = newPassword[i];
  }

  EEPROM.commit();  // Commit changes to EEPROM
  Serial.print("SSID written to EEPROM: ");
  Serial.println(newSSID);
  Serial.print("Password written to EEPROM: ");
  Serial.println(newPassword);
}

void readBrokerIP() {
  // Read the IP address from EEPROM
  for (int i = 0; i < IP_SIZE; i++) {
    mqttBrokerIP[i] = EEPROM.read(i + SSID_SIZE + PASSWORD_SIZE);  // Start after SSID and password
  }
  mqttBrokerIP[IP_SIZE - 1] = '\0';  // Null-terminate the string

  Serial.print("Broker IP read from EEPROM: ");
  Serial.println(mqttBrokerIP);
}

void writeBrokerIP(const char *ip) {
  // Write the IP address to EEPROM
  for (int i = 0; i < IP_SIZE; i++) {
    EEPROM.write(i + SSID_SIZE + PASSWORD_SIZE, ip[i]);
  }

  EEPROM.commit();  // Commit changes to EEPROM
  Serial.print("Broker IP written to EEPROM: ");
  Serial.println(ip);
}

bool isValidIP(const String &ip) {
  int num, dots = 0;
  String temp;

  for (int i = 0; i < ip.length(); i++) {
    // If the character is a dot
    if (ip[i] == '.') {
      dots++;
      // Check if there is a number before the dot
      if (temp.length() == 0 || temp.toInt() > 255) {
        return false;
      }
      temp = "";  // Reset temp for next segment
    } else if (ip[i] >= '0' && ip[i] <= '9') {
      temp += ip[i];  // Build the number
    } else {
      return false;  // Invalid character
    }
  }

  // Check the last segment
  if (temp.length() == 0 || temp.toInt() > 255) {
    return false;
  }

  // Valid if there are exactly 3 dots
  return (dots == 3);
}


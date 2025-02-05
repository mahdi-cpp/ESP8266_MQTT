#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// WiFi settings
const char *ssid = "reza";           // Replace with your WiFi name
const char *password = "reza12345";  // Replace with your WiFi password


// MQTT Broker settings
const char *mqtt_broker = "192.168.1.114";   // EMQX broker endpoint
const char *mqtt_topic = "temperature/topic";       // MQTT topic
const char *mqtt_username = "admin";         // MQTT username for authentication
const char *mqtt_password = "admin@123456";  // MQTT password for authentication
const int mqtt_port = 1883;                  // MQTT port (TCP)


WiFiClient espClient;

PubSubClient client(espClient);

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


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);  // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
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

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);
}


void setupI2CBus() {
  Wire.begin(SDA_PIN, SCL_PIN, I2C_MASTER);  // join i2c bus (address optional for master)
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf(msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqtt_topic, msg);
  }
}


void writeToSlaves() {
  uint8_t error;

 // Serial.printf("Read Temperature off slave 1", address.size());

  //Read 16 bytes from the slave
  uint8_t bytesReceived = Wire.requestFrom(I2C_SLAVE_1, 6); // request 6 bytes from slave 1
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

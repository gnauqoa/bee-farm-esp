#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

IPAddress mqttServer(167, 71, 221, 111); // IP: 167.71.221.111
const int mqttPort = 1883;

WiFiManager wifi_manager;
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

const String deviceId = "2";
const String chanelUpdate = "device/" + deviceId + "/update";
const String username = "dev_" + deviceId;
const String token = "0a4f3ba26abed956822feb8dc9e61e59";
char incomingData[64];
float tempRange = 0;
float humiRange = 0;
float luxRange = 0;
uint8_t mosfetSpeed = 100;
bool autoControl = false;

int relayState1 = 0;
int relayState2 = 0;
int relayState3 = 0;
int relayState4 = 0;

void readSerial();
void handleRoot();
void sendToArduino();
void callback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();

void setup()
{
  Serial.begin(9600);
  Serial.println("ESP8266 MQTT Connection");

  wifi_manager.autoConnect("ESP8266_MQTT");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started.");
}

void loop()
{
  server.handleClient();
  readSerial();
  if (!client.connected())
  {
    reconnectMQTT();
  }
  client.loop();
}

void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("...");
    if (client.connect(deviceId.c_str(), username.c_str(), token.c_str()))
    {
      Serial.println("connected");
      const String topic = "device/" + deviceId;
      const bool status = client.subscribe(topic.c_str());
      Serial.print("Subscribing to: ");
      Serial.print(topic);
      Serial.print(" -> ");
      Serial.println(status ? "Success" : "Failed");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  relayState1 = doc["btn1"] ? 1 : 0;
  relayState2 = doc["btn2"] ? 1 : 0;
  relayState3 = doc["btn3"] ? 1 : 0;
  relayState4 = doc["btn4"] ? 1 : 0;
  tempRange = doc["tempRange"];
  humiRange = doc["humiRange"];
  luxRange = doc["luxRange"];
  mosfetSpeed = doc["mosfetSpeed"];
  autoControl = doc["autoControl"];

  // Serial.printf("Received -> Btn1: %d, Btn2: %d, Btn3: %d, Btn4: %d, TempRange: %.2f, MosfetSpeed: %d\n",
  //               relayState1, relayState2, relayState3, relayState4, tempRange, mosfetSpeed);

  sendToArduino();
}

void readSerial()
{
  if (Serial.available() > 0)
  {
    int len = Serial.readBytesUntil('\n', incomingData, sizeof(incomingData) - 1);
    incomingData[len] = '\0';

    if (strstr(incomingData, "s,") == incomingData)
    {
      float temperature = 0, humidity = 0, lux = 0;
      int result = sscanf(incomingData, "s,%f,%f,%f", &temperature, &humidity, &lux);
      if (result == 3)
      {
        StaticJsonDocument<128> json;
        json["temp"] = temperature;
        json["humi"] = humidity;
        json["lux"] = lux;
        json["id"] = deviceId;

        char buffer[128];
        size_t len = serializeJson(json, buffer);
        client.publish(chanelUpdate.c_str(), buffer, len);
      }
    }
    else if (strstr(incomingData, "d,") == incomingData)
    {
      int relay1 = 0, relay2 = 0, relay3 = 0, relay4 = 0;
      int result = sscanf(incomingData, "d,%d,%d,%d,%d,%d",
                          &relay3, &relay4, &relay1, &relay2, &mosfetSpeed);

      if (result == 5)
      {
        StaticJsonDocument<128> json;
        json["id"] = deviceId;
        json["btn1"] = relay1;
        json["btn2"] = relay2;
        json["btn3"] = relay3;
        json["btn4"] = relay4;
        json["mosfetSpeed"] = mosfetSpeed;

        char buffer[128];
        size_t len = serializeJson(json, buffer);
        client.publish(chanelUpdate.c_str(), buffer, len);
      }
    }
  }
}

void handleRoot()
{
  String html = "<html><body><h1>ESP8266 Serial Log</h1></body></html>";
  server.send(200, "text/html", html);
}

void sendToArduino()
{
  String message = "g," + String(tempRange) + "," + String(humiRange) + "," + String(luxRange) + "," +
                   String(autoControl) + "," + String(mosfetSpeed) + "," +
                   String(relayState3) + "," + String(relayState4) + "," +
                   String(relayState1) + "," + String(relayState2);

  Serial.println(message); // Gửi dữ liệu đến Arduino qua Serial
}
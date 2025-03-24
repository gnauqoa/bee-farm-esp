#define BLYNK_TEMPLATE_ID "TMPL6kWOWGogq"
#define BLYNK_TEMPLATE_NAME "bee"

#include <ESP8266WiFi.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WebServer.h> // For Web Server

const char blynkAuth[] = "S_v0DhUiH8tcr6xgBi3o0dFU3nRqW6AX"; // Blynk Auth Token

WiFiManager wifi_manager;
ESP8266WebServer server(80); // Web server on port 80

String incoming_data = "";
String serial_log = ""; // To store serial log
bool debug_server = false;
bool control_mode = 0;
char incomingData[64]; // Array to hold incoming data
float temp_range = 0;
uint8_t mosfetSpeed = 100;

int relayState1 = 0;
int relayState2 = 0;
int relayState3 = 0;
int relayState4 = 0;
int relayState5 = 0;

void readSerial();
void handleRoot(); // Web server root handler
void sendToArduino();
byte calculateChecksum(byte *data, size_t length);

void setup()
{
  Serial.begin(9600);
  Serial.println("ESP8266 Blynk Connection");

  // Set up WiFiManager for automatic connection
  wifi_manager.autoConnect("ESP8266_Blynk");
  Serial.println("WiFi connected.");

  // Initialize Blynk
  Blynk.begin(blynkAuth, WiFi.SSID().c_str(), WiFi.psk().c_str());

  // Start the Web Server
  if (debug_server)
  {
    server.on("/", handleRoot); // Define root endpoint
    server.begin();
  }
  Serial.println("Web server started.");
}

void loop()
{
  Blynk.run();           // Keep Blynk active
  server.handleClient(); // Handle web server requests
  readSerial();
}

void readSerial()
{
  if (Serial.available() > 0)
  {
    int len = Serial.readBytesUntil('\n', incomingData, sizeof(incomingData));
    incomingData[len] = '\0'; // Null terminate the string

    // Check if the incoming data starts with "s,"
    if (strstr(incomingData, "s,") != NULL)
    {
      float temperature = 0;
      float humidity = 0;
      float lux = 0;

      // Parse the string data: "s,25.5,60.2,123.4,relay1State,relay2State,...,mosfetSpeed"
      int result = sscanf(incomingData, "s,%f,%f,%f",
                          &temperature, &humidity, &lux);
      Serial.println(result);
      if (result == 3)
      { // Successfully parsed all expected values
        // Output received values

        // Send the data to Blynk (Virtual Pins)
        Blynk.virtualWrite(V0, temperature); // Send temperature to V0
        Blynk.virtualWrite(V1, humidity);    // Send humidity to V1
        Blynk.virtualWrite(V2, lux);         // Send lux value to V2
      }
    }
    else if (strstr(incomingData, "d,") != NULL)
    {
      int relay1 = 0, relay2 = 0, relay3 = 0, relay4 = 0;

      // Parse the string data: "s,25.5,60.2,123.4,relay1State,relay2State,...,mosfetSpeed"
      int result = sscanf(incomingData, "d,%d,%d,%d,%d,%d",
                          &relay1, &relay2, &relay3, &relay4, &mosfetSpeed);

      if (result == 5)
      { // Successfully parsed all expected values
        // Output received values

        Blynk.virtualWrite(V6, relay1);      // Send Relay 1 state to V6
        Blynk.virtualWrite(V7, relay2);      // Send Relay 2 state to V7
        Blynk.virtualWrite(V8, relay3);      // Send Relay 3 state to V8
        Blynk.virtualWrite(V9, relay4);      // Send Relay 4 state to V9
        Blynk.virtualWrite(V5, mosfetSpeed); // Send Mosfet speed to V5
      }
    }
  }
}
byte calculateChecksum(byte *data, size_t length)
{
  byte checksum = 0;
  for (size_t i = 0; i < length; i++)
  {
    checksum ^= data[i];
  }
  return checksum;
}

// Web server root handler
void handleRoot()
{
  String html = "<html><head><title>ESP8266 Serial Log</title></head><body>";
  html += "<h1>ESP8266 Serial Log</h1>";
  html += "<div style='white-space: pre-wrap; font-family: monospace;'>" + serial_log + "</div>";
  html += "</body></html>";
  serial_log = "";
  server.send(200, "text/html", html);
}

void sendToArduino()
{
  String message = "g," + String(temp_range) + "," + String(control_mode) + "," + String(mosfetSpeed);
      // Add relay states to the message
      message += "," + String(relayState1) + "," + String(relayState2) + "," + String(relayState3) + "," + String(relayState4);

  Serial.println(message); // Send the formatted message to Arduino over serial
}

BLYNK_WRITE(V5)
{                              // Virtual Pin V5 is assigned to input number widget
  mosfetSpeed = param.asInt(); // Set the temperature range from the app
  sendToArduino();
}

BLYNK_WRITE(V4)
{                                 // Virtual Pin V4 is assigned to button widget
  control_mode = !!param.asInt(); // 0 = Off, 1 = On
  sendToArduino();
}

BLYNK_WRITE(V3)
{                               // Virtual Pin V3 is assigned to input number widget
  temp_range = param.asFloat(); // Set the temperature range from the app
  sendToArduino();
}

BLYNK_WRITE(V6)
{                              // Virtual Pin V6 controls Relay 1
  relayState1 = param.asInt(); // Update the state of relay 1
  sendToArduino();
}

BLYNK_WRITE(V7)
{                              // Virtual Pin V7 controls Relay 2
  relayState2 = param.asInt(); // Update the state of relay 2
  sendToArduino();
}

BLYNK_WRITE(V8)
{                              // Virtual Pin V8 controls Relay 3
  relayState3 = param.asInt(); // Update the state of relay 3
  sendToArduino();
}

BLYNK_WRITE(V9)
{                              // Virtual Pin V9 controls Relay 4
  relayState4 = param.asInt(); // Update the state of relay 4
  sendToArduino();
}

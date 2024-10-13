
#include "secrets.h" //wifi info

#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <WebServer.h>

#define LED_PIN    18       
#define LED_COUNT  16
#define BRIGHTNESS 50        

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

// Color Definitions
#define OFF        strip.Color(0, 0, 0, 0)
#define RED        strip.Color(255, 0, 0, 0)
#define GREEN      strip.Color(0, 255, 0, 0)
#define BLUE       strip.Color(0, 0, 255, 0)
#define YELLOW     strip.Color(255, 255, 0, 0) 

const unsigned long WARMUP_T = 10000; 
const int alcPin = 34;      
const int fanPin = 39;     
const int valvePin = 23;    

int BAC = 0;
int fanVal = 0;

WebServer server(80);


void setColor(uint32_t color);
void flashColor(uint32_t color);
void connectToWiFi();
void handleRoot();
void handleSensorData();


void setup() {                

  Serial.begin(115200);
  Serial.println("Serial communication started at 115200 baud.");
  

  pinMode(valvePin, OUTPUT);
  digitalWrite(valvePin, LOW);
  
  analogReadResolution(10);
  Serial.println("ADC resolution set to 10 bits.");


  strip.begin();           
  strip.show();            
  strip.setBrightness(BRIGHTNESS);
  Serial.println("NeoPixel strip initialized.");
  
  // Connect to Wi-Fi
  connectToWiFi();
  
  // Start the web server
  server.on("/", handleRoot);
  server.on("/sensor", handleSensorData);
  server.begin();
  Serial.println("Web server started.");
  
  // Warm-up Sequence: Flash YELLOW during warm-up period
  Serial.println("Starting warm-up sequence...");
  unsigned long startTime = millis();
  while (millis() - startTime < WARMUP_T){
    flashColor(YELLOW);
    // Optionally, print remaining warm-up time
    unsigned long elapsed = millis() - startTime;
    Serial.print("Warm-up elapsed time: ");
    Serial.print(elapsed / 1000);
    Serial.println(" seconds.");
  }
  

  setColor(GREEN);
  Serial.println("Warm-up complete. LEDs set to GREEN.");
}


void loop() {  
  server.handleClient();
  
  // Read analog inputs
  BAC = analogRead(alcPin);
  fanVal = analogRead(fanPin);
  
  // Print sensor values to Serial Monitor
  Serial.print("BAC: ");
  Serial.print(BAC);
  Serial.print(" | Fan Value: ");
  Serial.println(fanVal);
  
  // Control valve based on sensor readings
  
  if(fanVal > 30 && BAC < 750){
    setColor(GREEN);
    Serial.println("Condition met: Activating valve.");
    digitalWrite(valvePin, HIGH);   // Activate valve
    delay(4000);                    // Keep valve open for 4 seconds
    digitalWrite(valvePin, LOW);    // Deactivate valve
    Serial.println("Valve deactivated. Waiting for 5 seconds.");
    delay(5000);                    // Wait for 5 seconds before next action
  }
    if(fanVal > 30 && BAC >750){
    Serial.println("Condition nott met");
    flashColor(RED);
    flashColor(RED);
    flashColor(RED);
    flashColor(RED);
  }
  setColor(GREEN);
  delay(1000); 
}


// Set NeoPixel color
void setColor(uint32_t color) {
  for(int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show(); 
}

void flashColor(uint32_t color) {
  setColor(color);
  delay(500);             // Keep the color on for 500ms
  setColor(OFF);          // Turn off LEDs
  delay(500);             // Keep LEDs off for 500ms
}

// Connect to Wi-Fi
void connectToWiFi(){
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Wi-Fi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Handle root URL ("/")
void handleRoot(){
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>ESP32 Sensor Data</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f0f0f0; text-align: center; margin-top: 50px; }";
  html += "h1 { color: #333; }";
  html += ".sensor { font-size: 1.5em; margin: 20px; }";
  html += ".error { color: red; }";
  html += "</style>";
  
  // JavaScript to fetch sensor data periodically and update the page
  html += "<script>";
  html += "function fetchSensorData() {";
  html += "  fetch('/sensor').then(response => {";
  html += "    if (!response.ok) {";
  html += "      throw new Error('Network response was not ok');";
  html += "    }";
  html += "    return response.json();";
  html += "  }).then(data => {";
  html += "    document.getElementById('bac').innerText = data.BAC;";
  html += "    document.getElementById('fanVal').innerText = data.fanVal;";
  html += "    document.getElementById('error').innerText = '';";
  html += "  }).catch(error => {";
  html += "    console.error('Error fetching sensor data:', error);";
  html += "    document.getElementById('error').innerText = 'Error fetching data';";
  html += "  });";
  html += "}";
  html += "setInterval(fetchSensorData, 1000); // Fetch data every second";
  html += "window.onload = fetchSensorData; // Fetch data on page load";
  html += "</script>";
  
  html += "</head><body>";
  html += "<h1>ESP32 Sensor Data</h1>";
  html += "<div class='sensor'><strong>BAC:</strong> <span id='bac'>--</span></div>";
  html += "<div class='sensor'><strong>Fan Value:</strong> <span id='fanVal'>--</span></div>";
  html += "<div id='error' class='error'></div>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Handle sensor data request
void handleSensorData(){
  String json = "{";
  json += "\"BAC\":" + String(BAC) + ",";
  json += "\"fanVal\":" + String(fanVal);
  json += "}";
  server.send(200, "application/json", json);
}



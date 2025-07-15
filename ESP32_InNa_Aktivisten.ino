//For Temperature/Humidity Sensor
#include "DHT.h"

//For Soil Temperature
#include <OneWire.h>
#include <DallasTemperature.h>

//For SD Card
#include <SPI.h>
#include <SD.h>

//For LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//For Wifi
#include <WiFi.h>


// Define pin and sensor type

// Air temperature and humidity sensor
#define DHT_AIR_PIN 15         // GPIO15 (change if needed)
#define DHT_AIR_TYPE DHT22     // DHT22 (also known as AM2302)
// Data wire is connected to GPIO 4

// Soil temperature sensor
#define OW_SOIL_TEMP_BUS 25

// Soil humidity sensor
#define ADC_SOIL_HUM_PIN 34  // ADC pin (use any ADC1 pin like 32–39)
const int dry_value = 3550;  // Value when sensor is in dry air
const int wet_value = 0;  // Value when sensor is in water or wet soil

// SD card
#define SD_CS 5  // Chip Select pin

// Initialize DHT air temp and hum sensor
DHT dht(DHT_AIR_PIN, DHT_AIR_TYPE);

// Initialize oneWire soil temp instance
OneWire oneWire(OW_SOIL_TEMP_BUS);
// Pass oneWire to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// LCD display
int I2C_Address = 0; //placeholder to initialize variables so they are accessible in both setup and loop
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(I2C_Address, 16, 2);

// Set default log intervall
int timedelta_ms = 10000;  // exact loop time in milliseconds
// Define log file
File logFile;

// Wi-Fi credentials for the Access Point
const char* ssid = "ESP32_InNaAktivisten";
const char* password = "12345678";

WiFiServer server(80);  // TCP server on port 80
WiFiClient client;

void initLCD() {
  // Scan for I2C Devices
  Wire.begin();
  Serial.println("Scanning for I2C Devices; Assuming that only one I2C Device is connected and this is the LCD Display");
  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(address, HEX);
      I2C_Address = address;
    }
  }

  // Turn on LCD backlight
  lcd = LiquidCrystal_I2C(I2C_Address, 16, 2);
  lcd.init();
  lcd.noBacklight();
  delay(250);
  lcd.backlight();
}


// SD Card, Serial Monitor
void initSD() {
  Serial.println("Send logging interval in milliseconds via serial");

  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("Card initialization failed!");
    return;
  }

  Serial.println("SD card initialized.");

  // Open the log file in append mode
  logFile = SD.open("/log.txt", FILE_WRITE);

  if (logFile) {
    logFile.println("---- New Session ----");
    logFile.println("ESP32 SD Logging Started.");
    logFile.close();
    Serial.println("Log started.");
  } else {
    Serial.println("Failed to open log file.");
  }
}


void printLCD(float air_temperature, float air_humidity, float soil_temperature, float soil_humidity) {
  
  // Check if readings are valid
  if (isnan(air_humidity) || isnan(air_temperature)) {
    lcd.print("Failed to read air temperature ");
    Serial.println("Failed to read air temperature ");
    return;
  }

  if (soil_temperature == DEVICE_DISCONNECTED_C) {
    lcd.println("Failed to read soil temperature data.");
    Serial.println("Failed to read soil temperature data.");
    return;
  }

  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("t");
  lcd.print(air_temperature);
  lcd.print("C");

  lcd.setCursor(8,0);
  lcd.print("h");
  lcd.print(air_humidity);
  lcd.print("%");

  lcd.setCursor(0,1);
  lcd.print("st");
  lcd.print(soil_temperature);
  lcd.print("C");

  lcd.setCursor(8,1);
  lcd.print("sh");
  lcd.print(soil_humidity);
  lcd.print("%");
}


void logSD(float air_temperature, float air_humidity, float soil_temperature, float soil_humidity) {
  // Open file again each loop to append
  logFile = SD.open("/log.txt", FILE_APPEND);

  if (logFile) {
    String data = "time: " + String(millis() / 1000) + 
    "s, air_temp: " + air_temperature + 
    "°C, air_hum: " + air_humidity +
    "%, soil_temp: " + soil_temperature +
    "°C, soil_hum: " + soil_humidity + "%";

    logFile.println(data);
    logFile.close();

    Serial.println("Logged: " + data);

  } else {
    Serial.println("Error opening file.");
  }
}

void logWiFi(float air_temperature, float air_humidity, float soil_temperature, float soil_humidity) {
//WiFiClient client = server.available();  // Listen for incoming clients
  if (client && client.connected()) {

      String message = "";
      message.concat(air_temperature);
      message.concat(",");
      message.concat(air_humidity);
      message.concat(",");
      message.concat(soil_temperature);
      message.concat(",");
      message.concat(soil_humidity);
      message.concat("\n");

      client.print(message);
      Serial.println("Sent: " + message);
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);  // Give time for Serial Monitor to start
  
  // Start reading air temperature and humidity
  dht.begin();
  Serial.println("Air temperature and humidity reading started");
  
  // Start reading soil temperature
  pinMode(OW_SOIL_TEMP_BUS, INPUT_PULLUP);
  sensors.begin();
  Serial.println("Soil temperature reading started");

  initLCD();

  initSD();

  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());  // Usually 192.168.4.1
  server.begin();

}


void loop() {

  if(!client.connected()) {
    client = server.available();
  }
  
  // Check if data is available from the Serial Monitor
  unsigned long loopStart = millis();
  
  // read air temperature and humidity sensor
  float air_temperature = dht.readTemperature();
  float air_humidity = dht.readHumidity();

  // read soil temperature sensor
  sensors.requestTemperatures();
  float soil_temperature = sensors.getTempCByIndex(0);

  // read soil humidity sensor
  int soil_humidity_analog = analogRead(ADC_SOIL_HUM_PIN);
  // Convert to voltage (ESP32 ADC is 12-bit: 0–4095)
  //float voltage = analogValue * (3.3 / 4095.0);
  int soil_humidity = map(soil_humidity_analog, dry_value, wet_value, 0, 100);
  soil_humidity = constrain(soil_humidity, 0, 100);

  printLCD(air_temperature, air_humidity, soil_temperature, soil_humidity);

  logSD(air_temperature, air_humidity, soil_temperature, soil_humidity);

  if (client.connected()) {
    logWiFi(air_temperature, air_humidity, soil_temperature, soil_humidity);
  }

  // Update loop interval
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // Read until newline
    input.trim();  // Remove leading/trailing whitespace
    timedelta_ms = input.toInt();
    Serial.print("New logging Interval: ");
    Serial.println(timedelta_ms);
  }

  //Serial.print("log: ");
  //Serial.println(millis());
  delay(111);
  while(loopStart + timedelta_ms > millis()) {
  //delay(1000);
  }


  
}



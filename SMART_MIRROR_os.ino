/*
========================================================
        NO !DEA SMART MIRROR OS
        Created by: mad4machines
========================================================

DESCRIPTION:
A futuristic Smart Mirror OS using:
- NodeMCU ESP8266
- ST7735 1.44" TFT Display
- IR Sensor
- WiFi
- OpenWeatherMap API
- NTP Internet Time

FEATURES:
✔ Futuristic Smart Mirror Animation
✔ Random Compliments
✔ Real Time Clock & Date
✔ Weather Updates
✔ IR Human Detection

========================================================
IMPORTANT SETUP
========================================================

1. ADD YOUR WIFI DETAILS

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

2. ADD YOUR OPENWEATHERMAP API KEY

Replace:
appid=YOUR_API_KEY

Get free API key from:
https://openweathermap.org/api

========================================================
LIBRARIES REQUIRED
========================================================

Install these libraries from Arduino IDE:

- Adafruit GFX
- Adafruit ST7735
- ArduinoJson
- ESP8266WiFi

========================================================
CONNECTIONS
========================================================

TFT DISPLAY -> NodeMCU

VCC  -> 3.3V
GND  -> GND
SCL  -> D5
SDA  -> D7
RES  -> D2
DC/A0-> D3
CS   -> D8

IR SENSOR -> NodeMCU

VCC -> 3.3V
GND -> GND
OUT -> D4

========================================================
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <ArduinoJson.h>
#include <SPI.h>

// ========================================================
// TFT DISPLAY PINS
// ========================================================

#define TFT_CS D8
#define TFT_RST D2
#define TFT_A0 D3

// ========================================================
// IR SENSOR PIN
// ========================================================

#define IR_SENSOR_PIN D4

// ========================================================
// WIFI DETAILS
// ADD YOUR WIFI NAME & PASSWORD HERE
// ========================================================

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ========================================================
// TIME SERVER SETTINGS
// ========================================================

const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO = "IST-5:30";

// ========================================================
// WEATHER API
// ADD YOUR OPENWEATHERMAP API KEY
// ========================================================

const char* serverName =
"http://api.openweathermap.org/data/2.5/weather?q=Hyderabad&appid=YOUR_API_KEY&units=metric";

WiFiClient client;

// ========================================================
// TIME VARIABLES
// ========================================================

tm timeinfo;
time_t now;

// ========================================================
// RANDOM COMPLIMENTS
// ========================================================

const char* compliments[] = {
  "You look great!!",
  "Have a nice day!",
  "You're amazing!",
  "Keep smiling!",
  "Stay awesome!",
  "You look good!"
};

const int complimentsCount =
sizeof(compliments) / sizeof(compliments[0]);

// ========================================================
// INITIALIZE TFT DISPLAY
// ========================================================

Adafruit_ST7735 tft =
Adafruit_ST7735(TFT_CS, TFT_A0, TFT_RST);

// ========================================================
// GLOBAL VARIABLES
// ========================================================

int sensorReading = 1;

String weatherCondition = "";
float temperature = 0.0;

// ========================================================
// SETUP FUNCTION
// ========================================================

void setup() {

  // Initialize IR sensor
  pinMode(IR_SENSOR_PIN, INPUT);

  // Initialize TFT display
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);

  // Start serial monitor
  Serial.begin(115200);

  // Random seed for compliments
  randomSeed(analogRead(A0));

  // ====================================================
  // BOOT SCREEN
  // ====================================================

  tft.setTextSize(2);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 10);
  tft.print("NO !DEA");

  tft.setTextColor(ST77XX_BLUE);
  tft.setCursor(20, 40);
  tft.print("Smart");

  tft.setCursor(5, 70);
  tft.print("Mirror OS");

  delay(2000);

  tft.fillScreen(ST77XX_BLACK);

  // ====================================================
  // CONNECT TO WIFI
  // ====================================================

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);

  tft.setCursor(5, 10);
  tft.print("Connecting");

  tft.setCursor(20, 40);
  tft.print("WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(300);

    Serial.print(".");

    tft.print(".");
  }

  Serial.println("\nWiFi Connected!");

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_GREEN);

  tft.setCursor(10, 20);
  tft.print("WiFi");

  tft.setCursor(5, 50);
  tft.print("Connected");

  delay(1500);

  tft.fillScreen(ST77XX_BLACK);

  // ====================================================
  // INTERNET TIME SETUP
  // ====================================================

  configTime(0, 0, NTP_SERVER);

  setenv("TZ", TZ_INFO, 1);

  if (!getNTPtime(10)) {

    Serial.println("Failed to sync time");
  }

  // ====================================================
  // FETCH WEATHER DATA
  // ====================================================

  fetchWeather();
}

// ========================================================
// MAIN LOOP
// ========================================================

void loop() {

  // Read IR sensor
  sensorReading = digitalRead(IR_SENSOR_PIN);

  Serial.print("Sensor: ");
  Serial.println(sensorReading);

  // Show idle animation
  showSmartMirrorAnimation();

  // ====================================================
  // IF PERSON DETECTED
  // ====================================================

  if (sensorReading == LOW) {

    // Show compliment
    showCompliment();

    // Detect again within 1.5 sec
    if (checkForNextDetection(1500)) {

      // Show time/date
      showTimeDate();

      // Detect again within 1.5 sec
      if (checkForNextDetection(1500)) {

        // Show weather
        showWeather();
      }
    }
  }
}

// ========================================================
// CHECK FOR NEXT DETECTION
// ========================================================

bool checkForNextDetection(unsigned long timeout) {

  unsigned long startTime = millis();

  while (millis() - startTime < timeout) {

    int sensorReading = digitalRead(IR_SENSOR_PIN);

    if (sensorReading == LOW) {

      return true;
    }

    delay(20);
  }

  tft.fillScreen(ST77XX_BLACK);

  return false;
}

// ========================================================
// GET INTERNET TIME
// ========================================================

bool getNTPtime(int sec) {

  uint32_t start = millis();

  do {

    time(&now);

    localtime_r(&now, &timeinfo);

    delay(10);

  } while ((millis() - start <= 1000 * sec)
           && (timeinfo.tm_year < (2016 - 1900)));

  return (timeinfo.tm_year > (2016 - 1900));
}

// ========================================================
// SMART MIRROR ANIMATION
// ========================================================

void showSmartMirrorAnimation() {

  static uint8_t colorIndex = 0;

  uint16_t colors[] = {
    ST77XX_RED,
    ST77XX_GREEN,
    ST77XX_BLUE,
    ST77XX_CYAN,
    ST77XX_MAGENTA,
    ST77XX_YELLOW
  };

  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;

  int maxRadius =
  min(centerX, centerY) - 4;

  int thickness = 11;

  tft.setTextColor(colors[colorIndex]);
  tft.setTextSize(2);

  // Display Smart Mirror text

  tft.setCursor(centerX - 30, centerY - 15);
  tft.print("Smart");

  tft.setCursor(centerX - 35, centerY + 10);
  tft.print("Mirror");

  // Draw glowing circles

  for (int i = 0; i < thickness; i++) {

    tft.drawCircle(
      centerX,
      centerY,
      maxRadius - i,
      colors[colorIndex]
    );

    delay(50);
  }

  colorIndex = (colorIndex + 1) % 6;
}

// ========================================================
// SHOW RANDOM COMPLIMENT
// ========================================================

void showCompliment() {

  int complimentIndex =
  random(0, complimentsCount);

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);

  tft.setTextSize(2);

  tft.setCursor(5, 40);

  tft.print(compliments[complimentIndex]);

  delay(1200);
}

// ========================================================
// SHOW TIME & DATE
// ========================================================

void showTimeDate() {

  tft.fillScreen(ST77XX_BLACK);

  getNTPtime(10);

  char time_output[20];

  // Time

  tft.setTextColor(ST77XX_CYAN);

  tft.setTextSize(2);

  tft.setCursor(10, 10);

  sprintf(
    time_output,
    "%02d:%02d",
    timeinfo.tm_hour,
    timeinfo.tm_min
  );

  tft.print(time_output);

  // Date

  tft.setTextColor(ST77XX_YELLOW);

  tft.setCursor(10, 40);

  sprintf(
    time_output,
    "%02d/%02d/%02d",
    timeinfo.tm_mday,
    timeinfo.tm_mon + 1,
    timeinfo.tm_year - 100
  );

  tft.print(time_output);

  // Day

  tft.setTextColor(ST77XX_GREEN);

  tft.setCursor(10, 70);

  tft.print(getDOW(timeinfo.tm_wday));

  delay(1200);
}

// ========================================================
// GET DAY NAME
// ========================================================

char* getDOW(uint8_t tm_wday) {

  switch (tm_wday) {

    case 0: return "Sunday";
    case 1: return "Monday";
    case 2: return "Tuesday";
    case 3: return "Wednesday";
    case 4: return "Thursday";
    case 5: return "Friday";
    case 6: return "Saturday";

    default: return "Error";
  }
}

// ========================================================
// FETCH WEATHER DATA
// ========================================================

void fetchWeather() {

  HTTPClient http;

  http.begin(client, serverName);

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {

    String payload = http.getString();

    DynamicJsonDocument doc(1024);

    deserializeJson(doc, payload);

    weatherCondition =
    doc["weather"][0]["description"].as<String>();

    temperature =
    doc["main"]["temp"];

    Serial.println("Weather Updated!");

  } else {

    Serial.print("HTTP Error: ");

    Serial.println(httpResponseCode);
  }

  http.end();
}

// ========================================================
// DISPLAY WEATHER
// ========================================================

void showWeather() {

  // Update latest weather
  fetchWeather();

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_BLUE);

  tft.setTextSize(2);

  tft.setCursor(10, 10);

  tft.print("Weather");

  // Temperature

  tft.setCursor(10, 40);

  tft.print(String(temperature));

  tft.print(" C");

  // Condition

  tft.setTextSize(1);

  tft.setCursor(10, 70);

  tft.print(weatherCondition);

  delay(1500);

  tft.fillScreen(ST77XX_BLACK);
}
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <time.h>

// Replace with your actual Wi-Fi credentials and API key
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey = "YOUR_API_KEY";

// OpenWeatherMap for Hubli, India
String weatherURL = "http://api.openweathermap.org/data/2.5/weather?q=Hubli,IN&units=metric&appid=" + String(apiKey);

// 20x4 LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Change I2C address if needed

// NTP (Indian Standard Time)
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = 19800;
int daylightOffset_sec = 0;

// Custom icons
byte sun[8] = {
  B00100,
  B10101,
  B01110,
  B11111,
  B01110,
  B10101,
  B00100,
  B00000
};

byte cloud[8] = {
  B00000,
  B00000,
  B01100,
  B10010,
  B10010,
  B11111,
  B00000,
  B00000
};

byte rain[8] = {
  B00100,
  B00000,
  B01110,
  B11111,
  B11111,
  B00100,
  B01010,
  B00100
};

void setup() {
  Serial.begin(115200);

  lcd.begin();       // Initialize LCD
  lcd.backlight();   // Turn on backlight

  lcd.createChar(0, sun);
  lcd.createChar(1, cloud);
  lcd.createChar(2, rain);

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    return;
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(1000);
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    lcd.setCursor(0, 0);
    lcd.print("Time error         ");
    delay(5000);
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherURL);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      float temp = doc["main"]["temp"];
      const char* mainWeather = doc["weather"][0]["main"];
      const char* description = doc["weather"][0]["description"];

      // Line 0: Time and Date
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.printf("%02d:%02d %02d/%02d/%04d", timeinfo.tm_hour, timeinfo.tm_min,
                 timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

      // Line 1: City name
      lcd.setCursor(0, 1);
      lcd.print("Hubli Weather");

      // Line 2: Temp and Icon
      lcd.setCursor(0, 2);
      lcd.print("Temp: ");
      lcd.print((int)temp);
      lcd.print("C ");

      if (strcmp(mainWeather, "Clear") == 0) {
        lcd.write(byte(0)); // ☀️ Sun
      } else if (strcmp(mainWeather, "Clouds") == 0) {
        lcd.write(byte(1)); // ☁️ Cloud
      } else if (strcmp(mainWeather, "Rain") == 0 || strcmp(mainWeather, "Drizzle") == 0) {
        lcd.write(byte(2)); // 🌧️ Rain
      } else {
        lcd.print(mainWeather); // fallback
      }

      // Line 3: Description
      lcd.setCursor(0, 3);
      lcd.print(description);
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Weather Error");
    }

    http.end();
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Lost");
  }

  delay(60000); // Refresh every 60 seconds
}

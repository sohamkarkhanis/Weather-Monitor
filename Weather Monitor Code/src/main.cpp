#include <Arduino.h>

#include "DHT.h"
#include "SPIFFS.h"
#include "ThingSpeak.h"
#include "config.h"
#include "time.h"

// Note: Default address of the BMP has been modified to '0x76' in <Adafruit_BMP280.h> included in this PlatformIO project, to make it compatible with my sensor module.
//       If you are using a different library, change it accordingly.
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>

// ------ Objects ------ //
#define DHTPIN  32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_BMP280 bmp;

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  32
#define OLED_RESET     -1        // Reset pin # (-1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

AsyncWebServer server(80);
WiFiClient client;

// ------ Variables ------ //
const char *ntpServer = "pool.ntp.org";
String dataMessage;
String formattedTime;
unsigned long uploadMillis = 0;
float temp, hum, pressure, alt;

// ------ Function Prototypes ------ //
unsigned long getTime();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void loadOLED();
void loadingScreen();
void sendDataToThingSpeak();
void initCsv();
void addDatatoCsv();
float getTemp();
float getHum();
float getPressure();
void getFormattedTime();
float getAltitude();

void setup() {
    Serial.begin(115200);
    Wire.begin();

    ThingSpeak.begin(client);
    dht.begin();
    bmp.begin();

    configTime(19800, 0, ntpServer);        // Configured to GMT +5:30
    getFormattedTime();

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    loadingScreen();
    display.display();

    pinMode(DHTPIN, INPUT);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(SOFT_SSID, SOFT_PWD);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("softAP IP address: ");
    Serial.println(IP);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(SPIFFS, "/index.html", "text/html"); });
    server.on("/csv", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(SPIFFS, "/WeatherData.csv", "text/csv"); });
    server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) { request->send_P(200, "text/plain", "Hello World"); });
    server.begin();

    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    File file = SPIFFS.open("/WeatherData.csv");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("File Content:");
    while (file.available()) {
        Serial.print((char)file.read());
    }
    file.close();
    initCsv();
}

void loop() {

    // using a function with millis() instead of delay() to avoid blocking in the loop
    if ((millis() - uploadMillis) > UPLOAD_DELAY) {

        // if unconnected, connect
        if (WiFi.status() != WL_CONNECTED) {
            Serial.print("Attempting to connect");
            while (WiFi.status() != WL_CONNECTED) {
                WiFi.begin(STA_SSID, STA_PWD);
                Serial.print(".");
                delay(1000);
            }
            Serial.println("\nConnected.");
        }

        getFormattedTime();
        sendDataToThingSpeak();
        addDatatoCsv();
        uploadMillis = millis();
    }

    // Get latest values
    temp = getTemp();
    hum = getHum();
    pressure = getPressure();
    alt = getAltitude();

    loadOLED();               // load data into oled registers
    display.display();        // display data on oled
}

// --------------------------------------------------------------------------------- UPLOAD TO THINGSPEAK FN ----------------------------------
void sendDataToThingSpeak() {
    // Up to 8 fields in a channel => up to 8 different pieces of info per channel
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.setField(3, pressure);
    ThingSpeak.setField(4, alt);
    int x = ThingSpeak.writeFields(CHANNEL_NUMBER, API_KEY);        // can use x for error checking
}

// --------------------------------------------------------------------------------- SENSOR FN -----------------------------------------
float getTemp() {
    return dht.readTemperature();
    // return bmp.readTemperature();    // bmp itself heats up while working, thus it will show a higher ambient temperature
}
float getHum() {
    return dht.readHumidity();
}
float getPressure() {
    return (bmp.readPressure() / 100);        // in hPa
}
float getAltitude() {
    return bmp.readAltitude(1006);        // change according to local area pressure(hPa) at sealevel, returns value in meter
}
// --------------------------------------------------------------------------------- OLED DISPLAY FN -----------------------------------------
void loadOLED() {
    // clang-format off
    display.clearDisplay();       
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 1);

    display.print("Temperature: ");  display.print(temp);      display.println(" C");
    display.print("Humidity: ");     display.print(hum);       display.println(" %");
    display.print("Pressure: ");     display.print(pressure);  display.println(" hPa");
    display.print("Altitude: ");     display.print(alt);       display.println(" m");
    //clang-format on
}

// Edit this function to customise the loading screen as per your preferences, using bitmaps, etc
void loadingScreen() { 
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(25, 13);
    display.print("Weather Monitor");
}
// --------------------------------------------------------------------------------- CSV FN -----------------------------------------
void initCsv() {
    dataMessage = String("Timestamp") + "," +
    "Temperature" + "," + 
    "Humidity" + "," + 
    "Pressure" + "," +
    "Altitude" + "\n";
    writeFile(SPIFFS, "/WeatherData.csv", dataMessage.c_str()); // this will overwrite the enitre file *EVERYTIME* the device resets
}

void addDatatoCsv() {
    dataMessage = formattedTime + "," +
    (String)temp + "," +
    (String)hum + "," +
    (String)pressure + "," +
    (String)alt + "\n";
    appendFile(SPIFFS, "/WeatherData.csv", dataMessage.c_str());
}

// ----------------------------------------------------------------------------------- getTime ----------------------
void getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("failed to get time :(");
        return;
    }

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d-%m-%Y | %H:%M:%S", &timeinfo);     // format: DD-MM-YYYY | HH:MM:SS
    formattedTime = buffer;
    // return buffer;
}

// --------------------------------------------------------------------------------- SPIFFS CMDS -----------------------------------------

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("− failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println(" − not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char *path) {
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
        Serial.println("− failed to open file for reading");
        return;
    }

    Serial.println("− read from file:");
    while (file.available()) {
        Serial.write(file.read());
    }
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("− failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("− file written");
    } else {
        Serial.println("− frite failed");
    }
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("− failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("− message appended");
    } else {
        Serial.println("− append failed");
    }
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("− file renamed");
    } else {
        Serial.println("− rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path) {
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path)) {
        Serial.println("− file deleted");
    } else {
        Serial.println("− delete failed");
    }
}

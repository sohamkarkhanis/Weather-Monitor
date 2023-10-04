#ifndef CONFIG_H_
#define CONFIG_H_

// -------------------------------------------------------------- WiFi Credentials
#define SOFT_SSID "WeatherMonitor"
#define SOFT_PWD  "12345678"

#define STA_SSID  "STATION_SSID"        // replace with your wifi credentials to which the ESP32 will connect to
#define STA_PWD   "STATION_PWD"

// -------------------------------------------------------------- ThingSpeak
#define CHANNEL_NUMBER 1        // default = 1, replace with your thingspeak channel number
#define API_KEY        "REPLACE_WITH_YOUR_THINGSPEAK_API_KEY"

#define UPLOAD_DELAY   5000

#endif /* CONFIG_H_ */

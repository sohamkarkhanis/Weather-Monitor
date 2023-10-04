
# Weather Monitor

Weather Monitor is an ESP32-based project featuring:

- Real-time data capture without the need for an RTC (Real-Time Clock), achieved through NTP (Network Time Protocol) synchronization.
- Seamless integration with the ThingSpeak IoT platform.
- The hosting of a web server directly on the ESP32.


## Project Overview

- Utilizes two sensors: BMP280 for atmospheric pressure and altitude, and DHT11 for temperature and humidity measurements.
- Fetches accurate time and date data from the `pool.ntp.org` server, eliminating the need for an RTC module by connecting to a WiFi router in Station mode.
- Hosts a web server on the ESP32, allowing users to access and download captured data as a `.csv` file when connected to it as a SoftAP.
- Regularly sends sampled data to the ThingSpeak platform for real-time monitoring and analysis.
- Displays instantaneous readings on an OLED display.
- PCB layout, schematics, and Gerber files are provided for your convenience!

## License

This project is open-source and licensed under the [MIT License](LICENSE). Feel free to contribute, modify, or use it for educational purposes :)

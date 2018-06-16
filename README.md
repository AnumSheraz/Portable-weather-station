Portable Weather Station and Soil moinsture level sensor

Main parts of the code;
- Wifi management 
  Initially the code will search for previously stored Wifi credentials in EEPROM and tries to connect with it. If it cannot, 
  it will go into Wifi-Server mode running on 192.168.4.1. Go there and provide new Wifi SSID and password. Opeon restart, it will connect
  to new Wifi network.
- collects and sends data to PubNub channel `channel_chart`, via PubNub REST API.

Before cunning the code, make sure you have PubNub account already created and provide your publish and subscribe keys into the code.

circuit diagram

![](circut%20diagram.bmp)


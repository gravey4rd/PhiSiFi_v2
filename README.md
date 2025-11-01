# PhiSiFi_v2
*This project is a significantly improved version based on the original PhiSiFi project by p3tr0s. It builds upon the foundation of the original project, adding modern technologies and new features.

## Acknowledgements
A special thanks to p3tr0s for the foundational work on the original PhiSiFi project.

## FEATURES :
* The user interface and page methods have been improved.
* Fixed the issue where the deauthentication attack would stop upon refreshing the admin page.
* Resolved the problem where the deauthentication attack would halt if the modem was reset during the process, using a new method.
* The EvilTwin feature has been optimized.
* The notification prompting the victim to log in during an EvilTwin attack has been optimized.
* Fixed an issue to make the Evil Twin attack automatically shut down if the victim turns off the modem and attempts to enter a password.
*etc.

## DISCLAIMER
The source code given in this public repo is for educational use only and should only be used against your own networks and devices!<br>
Please check the legal regulations in your country before using it.

## Install using Arduino IDE
1. Install Arduino IDE
2. In Arduino go to `File` -> `Preferences` add this URL to `Additional Boards Manager URLs` ->
   `https://raw.githubusercontent.com/SpacehuhnTech/arduino/main/package_spacehuhn_index.json`  
3. In Arduino go to `Tools` -> `Board` -> `Boards Manager` search for and install the `deauther` package  
4. Download and open [PhiSiFi](https://github.com/p3tr0s/PhiSiFi/blob/main/ESP8266_PhiSiFi.ino) with Arduino IDE
6. Select an `ESP8266 Deauther` board in Arduino under `tools` -> `board`
7. Connect your device and select the serial port in Arduino under `tools` -> `port`
8. Click Upload button

# How to use:
- Connect to the AP named `PhiSiFi_v2` with password `gravey4rd` from your phone/PC.
- Select the target AP you want to attack (list of available APs refreshes every 15secs - page reload is required).
- Click the Start Deauthing button to start kicking devices off the selected network.
- Click the Start Evil-Twin button and optionally reconnect to the newly created AP named same as your target (will be open).
- You can stop any of the attacks by visiting `192.168.4.1/admin` while conected to Evil-Twin AP or by resetting the ESP8266.
- Once a correct password is found, AP will be restarted with default ssid `PhiSiFi_v2` / `gravey4rd` and at the bottom of a table you should be able to see something like "Successfully got password for - `TARGET_SSID` - `PASSWORD`
   - If you power down / hard reset the gathered info will be lost

## Credits:
* https://github.com/p3tr0s/PhiSiFi
* https://github.com/SpacehuhnTech/esp8266_deauther
* https://github.com/M1z23R/ESP8266-EvilTwin
* https://github.com/adamff1/ESP8266-Captive-Portal


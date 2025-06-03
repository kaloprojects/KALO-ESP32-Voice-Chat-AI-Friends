ESP32-audioI2S-3.0.11g.zip (older AUDIO.H library) is intended only for ESP32 without PSRAM. 
ESP32 with PSRAM: Download latest AUDIO.H version here: https://github.com/schreibfaul1/ESP32-audioI2S 

Recommendation (after installing zip file):
Do NOT rely on the displayed INSTALLED version in Arduino IDE > Libray manager!. In case you ever installed a older AUDIO.H library it might still display a wrong version number (e.g. 2.0.0 after succesful 3.0.11g installation). Reason: Older AUDIO.H libraries did not maintain the correct version ID in the related files 'library.json and libray.properties'. Better to double check your installed libraries in '..\Users\xyAdmin\Documents\Arduino\libraries manually. The file ..\ESP32-audioI2S-master\src\audio.cpp header shows the correct installed version.

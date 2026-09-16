# AUDIO.H library mirror (to older version 3.0.11g) <br>only needed for ESP32 without PSRAM
ESP32-audioI2S-3.0.11g.zip (older AUDIO.H library from July 18, 2024) is needed for _ESP32 without PSRAM_. Reason of this mirror site: Later AUDIO.H versions don't support ESP32 without PSRAM any longer and won't work properly ! (e.g. Open AI TTS not speaking audio). 
- ESP32 with PSRAM: Download always latest AUDIO.H version from @Schreibfaul1's github here: https://github.com/schreibfaul1/ESP32-audioI2S 

Recommendation (after installing zip file):
Do NOT rely on the displayed INSTALLED version in Arduino IDE > Library manager!. In case you ever installed an older AUDIO.H library it might still display a wrong version number (e.g. 2.0.0 after successful 3.0.11g installation). Reason: Older AUDIO.H libraries did not maintain the correct version ID in the related files 'library.json and libray.properties'. Proposal: Double check your installed libraries in '..\Users\xy\Documents\Arduino\libraries\ manually. The file ..\ESP32-audioI2S-master\src\audio.cpp header shows the correct installed version.

_Add-on (Sept. 2026):_
- AUDIO.H is only needed for [<KALO_ESP32_Voice_Chat_AI-Friends>](https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/KALO_ESP32_Voice_Chat_AI_Friends),
- the **NEW** [<KALO_Realtime_AI_friends_**LIGHT**>](https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/KALO_Realtime_AI_Friends_LIGHT) does NOT need (#include) any AUDIO.H library. _So my preferred recommendation to user with ESP32 (without PSRAM): Just use the LIGHT code (no 3.0.1g workarounds needed at all ;-)_.


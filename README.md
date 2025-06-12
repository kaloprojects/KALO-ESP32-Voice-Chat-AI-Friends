# Summary
Prototype of a ESP32 based voice chat dialog device, _similar Open AI ChatGPT via Voice_. Just as an use case of my earlier published [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant) and the additional 'lib_OpenAI_Chat.ino'. 

User can ask questions and following conversation via microphone _(pressing a button or touch pin as long speaking)_, Open AI bot will repeat via AI voice. _Code supports ongoing dialog sessions_, keeping & sending the complete chat history. 'Chat Completions' workflow allows 'human-like' ongoing dialogs, supporting chat history & follow up questions. Example: Q1: _"who was Albert Einstein?"_ - and later (after OpenAI response) - Q2: _"was he also a musician and _did he_ have kids?"_. 

With latest code update also actual and location related Live Information requests (Real-time web searches) supported. User defined key word (e.g. GOOGLE) toggles LLM to web search models as part of the memorized chat dialog. Example: _"will it rain in my region tomorrow?, please ask Google! "_, or _"Please check with GOOGLE, what are the latest projections for the elections tomorrow?"_. Mixed model usage of both models supported, allowing follow up requests (e.g. _"Please summarize the search in few sentences, skip any boring details!"_) also to previous web searches.

User can define a own 'AI System Prompt' (in header of library 'lib_OpenAI_Chat.ino'), allowing to build ESP32 chat devices with a self defined 'personality' for dedicated use cases/bots (default in code: role of a 'good old friend'). You could start a virtual conversation e.g. with a human warm up question: _"Hi my friend, tell me, how was your week, any exiting stories?"_.

**Architecture:** All is coded in C++ native (no server based components needed, no Node.JS or Python scripts or websockets used), audio recording and transcription are coded natively in C++ for I2S devices (microphone and speaker). ESP32 chat device (Wifi connected) can be used stand-alone (no Serial Monitor a/o keyboard a/o connected computer needed). Sketch might be also useful for a _Text ChatGPT_ device (no voice recording, no STT, no TTS needed) using a Terminal App (e.g. PuTTY) or the Serial Monitor to enter text requests. 

**Major Update Summaries:** Update _June 2025_ added **PSRAM support** (as alternative to SD Card), and **ElevenLabs STT** for **5-10x faster** SpeechToText transcription. Update _April 2025_ added Open AI Web Search LLM model added, supporting actual and location related **Live Information capabilities** (Real-time web searches) into chat dialogs. User queries with a user defined leading 'keyword' initiate a web search and embed the result in the ongoing chat. Update _March 2025_ added **hardware support** for [Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/).

# Workflow
Explore the details in the .ino libraries, summary in a nutshell:
- Recording user Voice with variable length (as long holding a btn), storing as .wav file (with 44 byte header) on SD card or (NEW) in PSRAM
- User can enter Open AI request also via text in Serial Monitor Input line or COM: Terminal Apps e.g. PuTTY)  
- Sending recorded WAV file to STT (SpeechToText) server, using Deepgram API or (NEW) ElevenLabs API (registration needed)
- Sending the received transcription text to Open AI (with user specified LLM models for CHAT and WEB SEARCH)
- Receiving AI response, printing in Serial Monitor, answering with a 'human' like (multi-lingual) Open AI voice
- RGB led indicating status: GREEN=Ready. -> RED=Recording -> CYAN=STT -> BLUE=Open AI CHAT -> PINK=Open AI WEB, YELLOW=Audio pending -> PINK=TTS Speaking. Short WHITE flashes indicate successful STT & Open AI response, short RED flash indicate successful keyword detection
- BUTTON: PRESS & HOLD for recording + _short_ PRESS stops Open AI voice (when speaking) - OR - repeats last answer (when silent)
- Pressing button again to proceed in loop for ongoing chat.

# Hardware requirements
Similar to my other project [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant):
- ESP32 (e.g. ESP32-WROOM-32) with connected Micro SD Card (VSPI Default pins 5,18,19,23)
- Alternatively _(NEW)_: ESP32 with PSRAM (tested with ESP32-WROVER), no longer SD Card reader needed
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- RGB status LED and optionally (recommended) an Analog Poti (for audio volume)
- Ready to Go hardware (example): [Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/).

# 3rd party Software licenses
- STT: **ElevenLabs** or alternatively **Deepgram** API KEY needed, Links: [ElevenLabs](https://elevenlabs.io/pricing#pricing-table) (STT 0$) / [Deepgram](https://console.deepgram.com/signup) (200$ free)
- LLM & TTS: **Open AI** API KEY needed (same API KEY for LLM & TTS), registration: [Open AI account](https://platform.openai.com) (5$ free).

# Library Dependencies
- KALO-ESP32-Voice-ChatGPT does _not_ need any 3rd party libraries _zip files_ to be installed (_except AUDIO.H!_), all functions in all lib_xy.ino’s are self-coded (WiFiClientSecure.h / i2s_std.h / SD.h are part of esp32 core libraries). AUDIO.H in main.ino is used for TTS playing audio (not needed for audio recording & transcription), no AUDIO.H needed in 'lib_xy.ino' libraries
- ESP32 core library (Arduino DIE): use latest [arduino-esp32](https://github.com/espressif/arduino-esp32) e.g. 3.2.0 (based on ESP-IDF 5.4.1) or later. Older 2.x ESP are _not_ supported.
- AUDIO.H library / ESP32 **with** PSRAM: Install latest [ESP32-audioIS](https://github.com/schreibfaul1/ESP32-audioI2S) zip, version 3.3.0 or later.
- AUDIO.H library / ESP32 **without** PSRAM: IMPORTANT! - Actual AUDIO.H libraries require PSRAM, ESP32 without PSRAM are no longer supported!. So you need to install last version which did not require PSRAM. Recommended version is **3.0.11g** (from July 18, 2024)!. Mirror link to 3.0.11g version [here]( https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/tree/main/libray_archive).
- Last-not-least: Sending a big THANK YOU and a huge shout out to @Schreibfaul1 for his great AUDIO.H library and all his support!. 
 
# Installation & Customizing
- Libraries see above. Use latest esp32 core for Arduino IDE: [arduino-esp32](https://github.com/espressif/arduino-esp32). AUDIO.H: Download the correct library zip file (with PSRAM [here](https://github.com/schreibfaul1/ESP32-audioI2S), without PSRAM [3.0.11g here]( https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/tree/main/libray_archive)), and add it in Arduino IDE via Sketch -> Include Library -> Add .ZIP Library 
- Copy all .ino files of into same folder (it is one sketch, split into multiple Arduino IDE tabs)
- Update your hardware pin assignments in sketches 'KALO_ESP32_Voice_ChatGPT.ino' & 'lib_audio_recording.ino'
- Define the hardware dependent setting (using PSRAM a/o SD Card) for the audio processing, update both #define RECORD_PSRAM & RECORD_SDCARD in 'lib_audio_recording.ino' header
- Insert your credentials (ssid, password, Open AI API KEY) and preferences (TTS voice) in header of _main sketch_ 'KALO_ESP32_Voice_ChatGPT.ino'
- Insert your SpeechToText API KEYS in header of main sketch. ElevenLabs -or- Deepgram API keys needed
- Define your own 'AI character' via SYSTEM PROMPT (String MESSAGES in header of 'lib_OpenAI_Chat.ino')
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage, limited info printed)
- Optional: Review recording parameter (SAMPLE_RATE, GAIN_BOOSTER_I2S) in 'lib_audio_recording.ino' header
- Optional: Copy Audio file 'Welcome.wav' to ESP32 SD card, played on Power On ('gong' sound)
- In case of COMPILER ERROR on _audio_play.openai_speech()_: Check/update the last line of code in main sketch (KALO-ESP32-Voice-ChatGPT.ino). Background: the amount of openai_speech() parameter changed with latest AUDIO.H versions.

# Known issues
- Script is tested with ESP32-WROOM-32 (no PSRAM) and ESP32-WROVER (with 4MB PSRAM), both working well. The **ESP32-S3** currently produces a Compiler Error ('_.. has no non-static data member named 'msb_right_'). 2025-06-12 Update: Issue found (reason was a slightly different i2s_std_slot_config_t struct definition on ESP32-S3), resolved in my latest (internal) code, _will be published asap (upcoming days)_
- Voice instruction not working and 'isRunning()' issue not solved (LED still indicated Playing 1-2 secs after audio finished): Both issues _are_ fixed already, but ESP32 with PSRAM needed (due AUDIO.H dependencies, see below)

# New features since 2025-06-05
- **PSRAM supported** for audio recording and transcription. SD Card no longer mandatory for ESP32 with PSRAM (tested with ESP32-WROVER). User defined #define settings for audio processing (#define RECORD_PSRAM & RECORD_SDCARD)
- Additional **parameter added** to Recording and transcription functions: 'Recording_Stop()' and 'SpeechToText_xy()'
- Additional STT added: supporting **ElevenLabs Scribe v1 SpeechToText** API (as alternative to Deepgram STT). Multilingual support (also mixed languages in same record supported), country codes no longer needed. Registration for API KEY needed [link](https://elevenlabs.io/de/pricing#pricing-table), cost free account supported 
- **Speed of SpeechToText significantly improved** (using Elevenlabs STT), in particular on **long sentences!**. Example: Short user voice recordings (e.g. 5 secs) are transcribed in ~ 0.5-2 secs (compared to ~5 sec with Deepgram), long user records (e.g. 20 secs!) transcribed in ~3 secs (compared to ~ 15 secs with Deepgram)  
- SpeechToText is **multi lingual**, detecting your spoken language automatically (~ 100 languages supported). As longer your spoken sentence (audio), as better the correct language detection 
- New Key word '**GOOGLE**': Activates Open AI **web search model** for Live Information requests 
- New Key word '**VOICE**': Enabling the new **Voice Instruction** parameter of Open AI TTS (user can force TTS character, e.g. "you are whispering"). _PSRAM needed_
- 'isRunning()' bug fix: Correct audio end detection (in past the LED still indicated Playing 1-2 secs after audio finished), solved with latest AUDIO.H version. _PSRAM needed_
- Updated to **latest models**. Open AI LLM: 'gpt-4.1-nano' & 'gpt-4o-mini-search-preview'. Deepgram STT: new MULTI lingual model 'nova-3-general' added, 'nova-2-general' still used for MONO lingual. ElevenLabs STT: 'scribe_v1'. Open AI TTS: 'gpt-4o-mini-tts' (for voice instruction) and 'tts-1' (default)
- Open AI TTS improved: **Audio streaming quality improved** (no longer clicking artefacts on beginning, letter cut offs at end resolved). _PSRAM needed_
- minor bugs resolved, added more detailed comments into sketch, code cleaned up.

# Updates history:
- **2025-06-05:** Major update, detail see above (**PSRAM** support, **ElevenLabs Scrivbe v1 STT** added support, STT **performance** increased)
- **2025-04-04:** Live Information Request capabilities added (supporting new **Open AI web search features**). Mixed support of chat model (e.g. 'gpt-4o-mini') and web search models (e.g. 'gpt-4o-mini-search-preview'). User queries with a user defined keyword initiate a web search and embed the result in the ongoing chat. Minor changes: all user specific credits are moved to header of main.ino sketch (KALO_ESP32_Voice_ChatGPT_20250404.ino), additional parameter added to function Open_AI(..) and SpeechToText_Deepgram(..). Code further cleaned up, detailed comments added in 'lib_OpenAI_Chat.ino'
- **2025-03-14:** Major enhancements: **Supporting techiesms hardware/pcb** [Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/). Code Insights: New toggle '#define TECHIESMS_PCB true' assigns all specific pins automatically (no user code changes needed). Minor enhancements: Welcome Voice (Open AI) added, RGB led colors updated, code clean up done
- **2025-01-26:** First drop, already working, not finally cleaned up (just posted this drop on some folks request).


.
.
. <br>
<br>
# Demo Videos
5 minute example chat. Using code default settings, System Prompt: Role of a 'good old friend'. 
- Deepgram STT language: English (en-US)
- TTS voice: Multilingual (Open AI default), used voice in video: 'onyx'
- Overlay window: Serial Monitor I/O in real time (using Terminal App PuTTY, just for demo purposes)
- Details of interest (m:ss): 1:35 (BTN stops Audio), 2:05 (Radio gimmick), 3:15 (multi-lingual capabilities)
<br>

[![Video - KALO-ESP32-Voice-ChatGPT](https://github.com/user-attachments/assets/8f236399-ff71-4dc3-9563-46cfe4e7fa91)](https://dark-controller.com/wp-content/uploads/2025/01/KALO-ESP32-Voice-ChatGPT-GQ.mp4)


.
.
.

Links of interest, featuring friend’s projects:
- Advanced ESP32 AI device, using streaming sockets for lowest latency: [Github ElatoAI](https://github.com/akdeb/ElatoAI/tree/main/firmware-arduino), [Github StarmoonAI](https://github.com/StarmoonAI/Starmoon)
- Ready to Go hardware (just upload my latest Github code instead) [Techiesms Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/)

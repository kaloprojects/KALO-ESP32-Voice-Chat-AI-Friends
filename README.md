# Summary
Prototype of a ESP32 based voice chat dialog device, _similar Open AI ChatGPT via Voice_. Just as an use case of my earlier published [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant). User can ask questions with a microphone _(pressing a button or touch pin as long speaking)_, Open AI bot will answer via voice. 

Code Insights: After releasing Recording button ESP32 is sending recorded audio.wav to _STT (SpeechToText) Deepgram server_ for text transcription, then sending this text as request to _Open AI server_ (LLM model e.g. "gpt-4o-mini"), finally converting the received Open AI answer with _TTS (TextToSpeech) server_ with an AI voice back to AUDIO and playing response via speaker on ESP32.

Code supports ongoing dialog sessions, keeping & sending the complete chat history. This allows follow-up dialogs, example: User Q1: 'who was Albert Einstein?' - and later (after OpenAI response) - User Q2: 'was _he_ also a musician and _did he_ have kids?'). 

User can define his own "AI System Prompt" (in header of library 'lib_OpenAI_Chat.ino'), allowing to build a ESP32 chat device with a self defined 'personality' for dedicated use cases/bots.

Architecture: All is coded in C++ native (no server based components needed, no Node.JS or Python scripts or websockets used), AUDIO handling coded natively in C++ for I2S devices (microphone and speaker). Just copy all .ino files into one folder and insert your personal credentials into the header sections.

ESP32 chat device (Wifi connected) can be triggered by voice completely standalone (no Serial Monitor a/o keyboard a/o connected computer needed). Serial Monitor is optional, useful in case the device is used as _Text ChatGPT_ device only (no voice recording, no Deepgram registration needed).

# Workflow
Explore the details in the .ino code, summary in a nutshell:
- Recording user Voice with variable length (recording as long a button is pressed), storing as .wav file (with 44 byte header) on SD card
- (Alternative: User can enter Open AI request via text in Serial Monitor Input line)  
- Sending recorded WAV file to STT (SpeechToText) server (using Deepgram API service, registration needed)
- Sending the received transcription text to Open AI (with user specified LLM model, e.g. "gpt-4o-mini")
- Receiving AI response, printing in Serial Monitor, speaking the answer with a 'human' sounding voice (using multilingual Open AI voices)
- All steps are indicated with RGB led (GREEN=Ready / RED=Recording / CYAN=STT&TTS / BLUE=OpenAI / MAGENTA=Speaking)
- Pressing record button again to proceed in loop for ongoing chat ...

# Hardware requirements
Same as in my other project [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant):
- ESP32 development board (e.g. ESP32-WROOM-32), connected to Wifi
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB LED (status indicator) and optionally an Analog Poti (for audio volume)

# 3rd party Software licenses
- STT: Deepgram API service for user voice transcription ([Registration free account](https://console.deepgram.com/signup) needed for personal API key)
- LLM & TTS: Open AI Registration needed (needed for personal API key, same account for LLM & TTS), not free.

# Installation & Customizing
- Required (for i2s_std.h): Arduino IDE with ESP32 lib 3.1.x (based on ESP-IDF 5.3.x). Older 2.x ESP (i2s.h) are not supported.
- Required (for TTS): [AUDIO.H library ESP32-audioI2S.zip](https://github.com/schreibfaul1/ESP32-audioI2S), version 3.0.11g or newer.
- Copy all .ino files of into same folder (it is one sketch, split into multiple Arduino IDE tabs)
- Update your pin assignments & wlan settings (ssid, password) in the .ino header files
- Update headers with personal credentials (Deepgram API key, OpenAI API key)
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)
- Review the default recording parameter (SAMPLE_RATE, GAIN_BOOSTER_I2S etc) in lib_audio_recording.ino header
- Define your STT language (Deepgram STT in lib_audio_transcription.ino header)
- Define 'your' preferred "System Prompt" in global String MESSAGES (in lib_OpenAI_Chat.ino header)

# Known issues
- N/A

# Updates
- 2025-01-18: First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- Cleaning code, detail adjustments


.
.
.

# Demo Videos
- Coming next

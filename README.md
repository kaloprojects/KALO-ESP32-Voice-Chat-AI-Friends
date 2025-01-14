-------------------- # KALO-ESP32-Voice-ChatGPT#
# Summary
Code snippets showing how to _record I2S audio_ and store as .wav file on ESP32 with SD card, how to _transcribe_ pre-recorded audio via _STT (SpeechToText)_ Deepgram API, how to _generate audio_ from text via _TTS (TextToSpeech)_ API from Google TTS or OpenAI TTS or (new) SpeechGen.IO. Triggering ESP32 actions via Voice.

The repository contains the Demo main sketch  'KALO_ESP32_Voice_Assistant.ino', demonstrating different use case of my libraries 'lib_audio_recording.ino', 'lib_audio_transcription.ino' and (new) 'lib_TTS_SpeechGen.ino' 

# Features
Explore the demo use case examples in main sketch, summary:
- Recording and playing audio are working offline, online connection needed for STT, TTS and streaming services
- Recording Voice Audio with variable length (recording as long a button is pressed), storing as .wav file (with 44 byte header) on SD card  
- Replay your recorded audio (using Schreibfaul1 <audio.h> library) 
- Playing Audio streams (e.g. playing music via radio streams with <audio.h> library)
- Triggering ESP actions via voice (e.g. triggering GPIO LED pins, addressing dedicated voices by calling their name, playing music on request)
- STT (SpeechToText): Deepgram API service (registration needed)  
- TTS (TextToSpeech): Google TTS free API (no registration needed)  
- TTS (TextToSpeech): Open AI API (6 multilingual voices, registration needed)
- TTS (TextToSpeech): #NEW#: SpeechGen.IO voices (many voices, not free, payment needed)  

# Hardware
- ESP32 development board (e.g. ESP32-WROOM-32), connected to Wifi
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB LED (status indicator) and Analog Poti (audio volume)

# Installation & Customizing
- Required (Jan. 2025): Arduino IDE with ESP32 lib 3.1.x (based on ESP-IDF 5.3.x). Older 2.x ESP framework fail because new I2S driver missed
- Required (for playing Audio on ESP32): AUDIO.H library [ESP32-audioI2S.zip](https://github.com/schreibfaul1/ESP32-audioI2S). Install latest zip  (3.0.11g from July 18, 2024 or newer)
- Copy all .ino files of 'KALO-ESP32-Voice-Assistant' into same folder (it is one sketch, split into multiple Arduino IDE tabs)
- Update your pin assignments & wlan settings (ssid, password) in the .ino header files
- Update headers with personal credentials (Deepgram API key, optional: OpenAI API key, SpeechGen Token)
- Define your favorite recording settings (SAMPLE_RATE, BITS_PER_SAMPLE, GAIN_BOOSTER_I2S) in lib_audio_recording.ino header
- Define your language settings (Google TTS in KALO_ESP32_Voice_Assistant.ino, Deepgram STT in lib_audio_transcription.ino header)
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)

# Known issues
- Earlier WifiClientSecure connection issues seem solved (with KALO 2025-01-06 Update & arduino-esp32 3.1.x)
- Google TTS support short sentences only (Google limitation), non-free services (OpenAI and SpeechGen.IO) are not limited in length.
- TTS: Only OpenAI voices are multi-lingual (supporting multiple languages in same request), Google & SpeechGen.IO request language specific parameter/voices

# Updates
- 2025-01-06: NEW library for TTS Speechgen.IO (hundreds of voices) 
- 2025-01-06: Cleaned code, connection reliability issues solved, response time improved
- 2024-07-22: Misc. enhancements, WifiClientSecure reliability workarounds, code cleaned up
- 2024-07-18: 'Play 8bit audio' issue solved (latest AUDIO.H support 8bit wav format)
- 2024-07-14: WifiClientSecure connection reliablility improved (still not perfect)
- 2024-07-14: STT Deepgram response faster (typical response time on e.g. 5 sec voice record: ~ 2.5 sec).
- 2024-07-08: First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- currently no major updates planned, enjoy the libraries :)
- I will add another project (repository) soon: 'ESP32 Voice OpenAI ChatGPT device' (using same C libraries)


.
.
.

# Demo Videos
Short video clip, presenting Recording & SpeechToText & TextToSpeech (without Open AI, ESP32 is not 'answering', just parroting my voice with free Google TTS voice). Workflow: 
- Recording user voice, storing audio .wav file (8KHz/8bit) to SD card,
- STT: transcribe pre-recorded voice via Deepgram API,
- TTS: repeat spoken sentence with Goggle TTS voice (a/o triggering e.g. LED via voice):

[![Video Screenshot](https://github.com/user-attachments/assets/038905a8-3064-44c1-8eb7-e14cc6da94ab)](https://dark-controller.com/wp-content/uploads/2024/07/KALO_VoiceAssitant_Video01.mp4)

<br>
Featured video from other users & friends:<br>
@techiesms: using my Deepgram transcription STT library in his IoT projects: <br>https://www.youtube.com/watch?v=j0EEFXmikvk- 



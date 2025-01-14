# Summary
Prototype of a ESP32 based voice chat dialog device, _similar Open AI ChatGPT via Voice_. Just as an use case of my earlier published [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant). User can record questions with a microphone _(pressing a button as long speaking)_. On button release: ESP32 sending audio to _STT (SpeechToText) Deepgram server_, sending transcription to _Open AI server_ (LLM model e.g. "gpt-4o-mini"), finally converting LLM answer with _TTS (TextToSpeech) server_ into AUDIO and playing response via speaker on ESP32.

Code supports an ongoing dialog seesions, means LLM memories the chat history. This allows follow-up dialogs (e.g. 'who was Albert Einstein?' .. and later (after OpenAI response): 'was he also a musician?'). All is coded in C++ native (no server based components neded, no Node.JS or Phyton scripts or websockets used), AUDIO handling coded nativelly in C++ for I2S devices (microphone and speaker).

ESP32 chat device (Wifi connected) can be triggered by voice completelly (no Serial Monitor a/o keyboard a/o connected computer needed). Serial Monitor is optional (for alternative chats).

# Workflow
Explore the details in the .ino code, summary in a nutshell:
- Recording user Voice with variable length (recording as long a button is pressed), storing as .wav file (with 44 byte header) on SD card  
- Sending recorded WAV file to STT (SpeechToText) server (using Deepgram API service, registration needed)
- Sending the received transcription text to Open AI (with user specified LLM model, e.g. "gpt-4o-mini")
- Receiving AI response, generating 'human' sounding spoken answer (using the 9 multilingual Open AI voices)
- Pressing record button again to proceed ongoing chat ...

Alternativelly (in addition / no STT & TTS needed): Enter any Open AI request in the Serial Monitor Input line, receiving text response in Serial Monitor

The RGB led indicates the current status, examples: GREEN: Ready,  RED: Recording,  CYAN: STT&TTS,  BLUE: Open AI LLM,  MAGENTA: Speaking .. etc.

# 3rd party Software licenses
- STT: Deepgram API service Registration needed (for personal API key), not free.
- LLM & TTS: Open AI Registration needed (for personal API, same key for LLM & TTS), not free.
  
# Hardware requirements
Same as in my other project [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant):
- ESP32 development board (e.g. ESP32-WROOM-32), connected to Wifi
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB LED (status indicator) and optionally an Analog Poti (for audio volume)

# Installation & Customizing
- Required (Jan. 2025): Arduino IDE with ESP32 lib 3.1.x (based on ESP-IDF 5.3.x). Older 2.x ESP framework are not supported.
- Required (for playing Audio on ESP32): AUDIO.H library [ESP32-audioI2S.zip](https://github.com/schreibfaul1/ESP32-audioI2S). Install latest zip  (3.0.11g from July 18, 2024 or newer)
- Copy all .ino files of into same folder (it is one sketch, split into multiple Arduino IDE tabs)
- Update your pin assignments & wlan settings (ssid, password) in the .ino header files
- Update headers with personal credentials (Deepgram API key, OpenAI API key)
- Define your favorite recording settings (SAMPLE_RATE, BITS_PER_SAMPLE, GAIN_BOOSTER_I2S) in lib_audio_recording.ino header
- Define your language settings (Deepgram STT in lib_audio_transcription.ino header)
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)

# Known issues
- N/A

# Updates
- 2025-01-14: First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- Cleaning code, detail adjustments
.
.
.

# Demo Videos
- coming next


In addition: 
-
- Recording and playing audio are working offline, online connection needed for STT, TTS and streaming services
- Replay your recorded audio (using Schreibfaul1 <audio.h> library) 
- Playing Audio streams (e.g. playing music via radio streams with <audio.h> library)
- Triggering ESP actions via voice (e.g. triggering GPIO LED pins, addressing dedicated voices by calling their name, playing music on request)
- STT (SpeechToText): Deepgram API service (registration needed)  
- TTS (TextToSpeech): Google TTS free API (no registration needed)  
- TTS (TextToSpeech): Open AI API (6 multilingual voices, registration needed)
- TTS (TextToSpeech): #NEW#: SpeechGen.IO voices (many voices, not free, payment needed)

- chat to enter 

Microphone 


User  
User can ask questions, sending to OpenAI API, speaking  
, using 
Code snippets showing how to _record I2S audio_ and store as .wav file on ESP32 with SD card, how to _transcribe_ pre-recorded audio via _STT (SpeechToText)_ Deepgram API, how to _generate audio_ from text via _TTS (TextToSpeech)_ API from Google TTS or OpenAI TTS or (new) SpeechGen.IO. Triggering ESP32 actions via Voice.

The repository contains the Demo main sketch  'KALO_ESP32_Voice_Assistant.ino', demonstrating different use case of my libraries 'lib_audio_recording.ino', 'lib_audio_transcription.ino' and (new) 'lib_TTS_SpeechGen.ino' 

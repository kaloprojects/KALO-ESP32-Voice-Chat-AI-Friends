# Summary
Prototype of a ESP32 based voice chat dialog device, _similar Open AI ChatGPT via Voice_. Just as an use case of my earlier published [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant) and the additional 'lib_OpenAI_Chat.ino'. 

User can ask questions and following conversation via microphone _(pressing a button or touch pin as long speaking)_, Open AI bot will repeat via AI voice. 

_Code supports ongoing dialog sessions, keeping & sending the complete chat history (during session). This 'Chat Completions' workflow allows 'human-like' ongoing dialogs_. Also useful for simple 'facts' requests, as this supports follow up questions, example: User Q1: 'who was Albert Einstein?' - and later (after OpenAI response) - User Q2: 'was _he_ also a musician and _did he_ have kids?'). 

User can define his own "AI System Prompt" (in header of library 'lib_OpenAI_Chat.ino'), allowing to build a ESP32 chat device with a self defined 'personality' for dedicated use cases/bots (default in code: role of a 'good old friend')

Architecture: All is coded in C++ native (no server based components needed, no Node.JS or Python scripts or websockets used), AUDIO handling coded natively in C++ for I2S devices (microphone and speaker). Just copy all .ino files into one folder and insert your personal credentials into the header sections.

Code Insights: After releasing Recording button, ESP32 is sending recorded audio.wav to _STT (SpeechToText) Deepgram server_ for text transcription, then sending this text as request to _Open AI server_ (LLM model e.g. "gpt-4o-mini"), finally converting the received Open AI answer with _TTS (TextToSpeech) server_ with an AI voice back to AUDIO and playing response via speaker on ESP32.

ESP32 chat device (Wifi connected) can be triggered by voice completely standalone (no Serial Monitor a/o keyboard a/o connected computer needed). Serial Monitor is optional, useful in case the device is used as _Text ChatGPT_ device only (no voice recording, no Deepgram registration needed). Hint: Using a Terminal App (e.g. PuTTY) allows to use the ESP323 as Text Chat device without any IDE Serial Monitor. 

**Update (April 4, 2025): Open AI Web Search LLM model added**, this allows to include _actual and location related Live Information (Real-time web searches) requests_ into chat dialogs. 
User queries with a user defined leading 'keyword' initiate a web search and embed the result in the ongoing chat. Examples: using keyword 'Question': _'Question: what date is today?'_ or _'Question: will it rain in my region tomorrow?'_. Mixed model usage of both models supported (default chat & new web search model), this allows follow up requests (e.g. _'please summarize!'_) also to previous web search questions.

# Workflow
Explore the details in the .ino code, summary in a nutshell:
- Recording user Voice with variable length (as long holding a btn), storing as .wav file (with 44 byte header) on SD card
- (Alternative: User can enter Open AI request via text in Serial Monitor Input line or COM: Terminal Apps e.g. PuTTY)  
- Sending recorded WAV file to STT (SpeechToText) server (using Deepgram API service, registration needed)
- Sending the received transcription text to Open AI (with user specified LLM model, e.g. "gpt-4o-mini")
- Receiving AI response, printing in Serial Monitor, answering with a 'human' sounding voice (multilingual Open AI voices)
- RGB led: GREEN=Ready, RED=Recording, CYAN=STT, BLUE=OpenAI CHAT, _new_: PINK=Open AI WEB, YELLOW=TTS start, PINK=Speaking. Short WHITE flashes after successfull STT & OpenAI LLM response
- BUTTON: PRESS & HOLD for recording + PRESS stops Open AI voice (when speaking) or repeats last answer (when silent)
- NEW: Start a sentence with 'Question..' to launch web search with TTS response
- Pressing button again to proceed in loop for ongoing chat ...

# Hardware requirements
Same as in my other project [KALO-ESP32-Voice-Assistant Libraries](https://github.com/kaloprojects/KALO-ESP32-Voice-Assistant):
- ESP32 development board (e.g. ESP32-WROOM-32), connected to Wifi
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB status LED and optionally an Analog Poti (for audio volume)
- NEW: Ready to Go hardware (example): [Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/).

# 3rd party Software licenses
- STT: Deepgram API service for user voice transcription ([Registration free account](https://console.deepgram.com/signup) needed for personal API key)
- LLM & TTS: Open AI Registration needed (needed for personal API key, same account for LLM & TTS), not free.

# Installation & Customizing
- Required (for i2s_std.h): Arduino IDE with ESP32 lib 3.1.x (based on ESP-IDF 5.3.x). Older 2.x ESP (i2s.h) are not supported.
- Required (for TTS): [AUDIO.H library ESP32-audioI2S.zip](https://github.com/schreibfaul1/ESP32-audioI2S), version 3.0.11g or newer.
- Copy all .ino files of into same folder (it is one sketch, split into multiple Arduino IDE tabs)
- Copy Audio file 'Welcome.wav' to ESP32 SD card, played on Power On (posted welcome.wav is a 'gong') 
- Update your hardware pin assignments in sketches 'KALO_ESP32_Voice_ChatGPT.ino' & 'lib_audio_recording.ino'
- NEW: Update your credentials and all user settings in header of main sketch 'KALO_ESP32_Voice_ChatGPT.ino'
- Define your own 'AI character' via "System Prompt" (String MESSAGES in header of 'lib_OpenAI_Chat.ino')
- Optional: Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)
- Optional: Review default recording parameter (SAMPLE_RATE, GAIN_BOOSTER_I2S etc) in lib_audio_recording.ino header


# Known issues
- Total Response time (Latency): STT (Deepgram transcription response) is pretty fast already (for prerecorded .wav, no streaming), typically ~ 0.5-1 sec. (after file sent). To visualize latency: count the '.' in demo video below (printing a '.' each 250ms). The reason why total SST time still seems high is the time consuming sending of .wav data (e.g. file bulk reading of a 10 sec record with 8bit/16kHz from SD card needs about 5 secs). Open AI response time (model gpt-4o-mini) also improved (typically less 2-3 secs.). Still a bit annoying (in my opinion) is the TTS latency (voice starts often 1-2 seconds delayed)
- Minor issue: 'audio.isRunning()' keeps true some moments after PLAY Audio done (AUDIO.H issue, led 'pink') 

# Updates
- **2025-04-04:** Live Information Request capabilities added (supporting new Open AI web search features). Mixed support of chat model (e.g. "gpt-4o-mini") and web search models (e.g. "gpt-4o-mini-search-preview"). User queries with the leading keyword 'Question..' initiate a web search and embed the result in the ongoing chat. Minor changes: all user specific credits are moved to header of Main sketch (KALO_ESP32_Voice_ChatGPT_20250404.ino), additional parameter added to function Open_AI(..) and SpeechToText_Deepgram(..). Code further cleaned up, detailed comments added in 'lib_OpenAI_Chat.ino'.
- **2025-03-14:** Major enhancements: Supporting techiesms's hardware/pcb [Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/). Code Insights: New toggle '#define TECHIESMS_PCB true' assigns all specific pins automatically (no user code changes needed). Minor enhancements: Welcome Voice (Open AI) added, RGB led colors updated, code clean up done
- **2025-01-26:** First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- Plan is to add Live Information Request capabilities (Real-time web search feature) via keyword (e.g. 'Question:..'). Supporting user requests like _'Question: what date is today?'_ or _'Question: will it rain in my region tomorrow?'_. -> Update: DONE with latest version (April 4, 2025)
- Currently thinking about a further version using PSRAM (instead SD card reader) for recording & transcription. This could speed up the STT transcription. -> Update (April 28, 2025): _Good news: PSRAM working well (on my internal updates), no longer need of a SD card, but unfortunalty speed of STT not significantly improved_, bottleneck seems an issue in ESP32 WifiClientSecure.write(). I will discuss my tests with Espressif ESP32 folks next. Ongoing.
- Additional Speech-To-Text (STT) function will be added. Supporting ElevenLabs Scribe STT (high performance, high accuracy, multilingual, using free API key)

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

Links of interest, featuring friends projects:
- Advanced ESP32 AI device, using streaming sockets for lowest latency: [Github StarmoonAI/Starmoon project](https://github.com/StarmoonAI/Starmoon) 


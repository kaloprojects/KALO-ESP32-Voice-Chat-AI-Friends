# Summary

ESP32 based voice chat dialog device, successor of the earlier project KALO-ESP32-Voice-_ChatGPT_. Chatting with _multiple custom chatbot FRIENDS_ (similar to Open AI's Custom GPT's or Google's Gems). Just call any FRIEND by name, the device will activate the AI personality (custom system prompt) and answer with friend’s assigned voice. 

User can ask questions and following conversation via microphone _(pressing a button or touch pin as long speaking, no length limit, dynamic duration)_ - or **NEW** -  _without pressing any button_. Code supports ongoing dialog sessions, keeping & sending the complete chat history. 'Chat Completions' workflow allows 'human-like' ongoing dialogs, supporting chat history & follow up questions. Example: Q1: _"who was Albert Einstein?"_ - and later (after LLM response) - Q2: _"was he also a musician and _did he_ have kids?"_. Live Information requests (Real-time web searches, e.g weather forecast, political news etc.) supported.

The device works _multi-lingual_ by default, i.e. each chatbot/FRIEND can automatically _understand and speak multiple languages_. No changes in code (or system prompts) needed. Also mixed usage is supported (changing language in same dialog session). List of supported languages (Aug. 2025): 99 languages in [STT](https://elevenlabs.io/docs/capabilities/speech-to-text#supported-languages), 57 languages for [TTS](https://platform.openai.com/docs/guides/text-to-speech#supported-languages).

NEW since Sept. 2026: **Open AI Realtime mode** (voice-to-voice) added. _Automated user voice detection, no longer necessary to press a record button while speaking_. Fast response (low latency), emotional feedback, calling another friend's name on the fly. Already existing **features** (e.g. key commands, friend switch, url/music streaming, internet web-search request etc.) are **embedded and supported** in REALTIME_MODE too !.

NEW since January 2026: **Hardware DIY template (PCB Source and Gerber files) published**. Additional PCB templates added.

New since September 2025: **Chat history can be sent as email** to any user email accounts. Purpose: Archiving of interesting chats, in particular for mobile AI devices (similar to manual copy/paste from Serial Monitor on cable connected devices). Example, just say _"Hey, can you send me all via email?"_, the complete chat will pop up immediately in your Inbox. 

New since August 2025: **supporting 1-N chatbots/FRIENDS with user defined personality and voice** (System Prompts), custom defined TTS voice parameter allow to assign _different voices_ to each friend. The LLM AI response latency significantly improved (using **GroqCloud** API services). Just **say a FRIEND's name** during a conversation to activate the character. The included chatbot friends serve as template for your own custom chatbots, coded examples: _ONYX_ (role of a 'good old friend'), _FRED_ (a constantly annoyed guy), _GlaDOS_ (the aggressive egocentric bot), or _VEGGI_ (best friend of vegan and healthy food). You could start a virtual conversation e.g. with a human warm up question: _"Hi my friend, tell me, how was your week, any exiting stories?"_. or **waking up another friend calling his name**, e.g. with a statement like _"Hi_ **FRED**, _are you online?"_ 

Since June 2025: Added **PSRAM support** (as alternative to SD Card). **ESP32-S3** support and **ElevenLabs STT for 5-10x faster STT** (SpeechToText) transcription. **Live Information requests (Real-time web searches)** supported. User defined key word (e.g. **GOOGLE**) toggles LLM to web search models as part of the memorized chat dialog. Example: _"will it rain in my region tomorrow?, please ask Google! "_, or _"Please check with Google, what are the latest projections for the elections tomorrow?"_. **Mixed model usage of both models supported** (web searches are part of the chat history), allowing follow up requests (e.g. _"Please summarize the search in few sentences, skip any boring details!"_) also to previous web searches. Key word GOOGLE works with all AI chatbots/FRIENDS.

**Architecture**: All is coded in C++ native (no server based components needed, no Node.JS or Python scripts or websockets used), audio recording and transcription are coded natively in C++ for I2S devices (microphone and speaker). ESP32 chat device (Wifi connected) can be used stand-alone (no Serial Monitor a/o keyboard a/o connected computer needed). Sketch might be also useful for a _Text Chat_ device (no voice recording, no STT, no TTS needed) using a Terminal App (e.g. PuTTY) or the Serial Monitor to enter text requests. 

# Workflow
Explore the details in the .ino libraries, summary in a nutshell:

INPUT_REALTIME_MODE (_NEW_):
- Auto Recording (detection) of user voice (no length limitation, continuous background streaming to Open AI server)
- Auto silence detection, LLM starting after user speaking finished (user defined silence duration, e.g. after 600ms)
- Voice-to-Voice chat, printing transcription in Serial Monitor, emotional 'human' like (multi-lingual) Open AI voices
- User can enter LLM AI request alternatively also via text in Serial Monitor (or via any COM: Terminal Apps e.g. PuTTY)  
- RGB led indicating status: WHITE=Ready/Waiting -> RED=UserVoiceAutoRecording -> BLUE=Open AI Speaking. <br>CYAN=WEBSEARCH. RED flashes indicate e.g. keyword detection or friend changes
- Pressing (previous record) button allows to STOP (interupt) Open AI voice at any time.
  
INPUT_BUTTON_MODE:
- Recording user Voice with variable length (holding a btn), storing as .wav (with 44 byte header) in PSRAM or SD
- Sending recorded WAV file to STT (SpeechToText) server, using fast ElevenLabs API (or slower Deepgram)
- Sending transcription to Open AI or Groq server (with user specified LLM models) for CHAT and WEB SEARCH
- Receiving AI response, printing in Serial Monitor, speaking with a 'human' like (multi-lingual) Open AI voice
- User can enter LLM AI request alternatively also via text in Serial Monitor (or via any COM: Terminal Apps e.g. PuTTY) 
- RGB led indicating status: GREEN=Ready -> RED=Recording -> CYAN=STT -> BLUE=LLM AI CHAT -> PINK=Open AI WEB -> YELLOW=Audio pending -> PINK=TTS Speaking. Short WHITE flashes indicate success, RED flashes indicate keyword detection, double RED flashes on waking up another FRIEND
- Button: PRESS & HOLD for recording + _short_ PRESS interrupts TTS/Audio OR repeats last answer (when silent).
 
# Installation & Customizing
- Copy all .ino files of into same folder (it is one sketch, split into multiple Arduino IDE tabs)
- Insert your credentials (ssid, password) and user settings in header of main sketch <KALO_ESP32_Voice_AI_Friends.ino>
- Select one of the PCB templates or insert your own hardware pin assignments in main sketch <KALO_ESP32_Voice_AI_Friends.ino>
- Create your own 1-N 'AI Friends' character' in _new_ file <lib_friends.ino>
- Optional: Review default settings in header of each .ino, e.g. DEBUG toggle in <KALO_ESP32_Voice_AI_Friends.ino>
- Optional: Copy Audio file 'Welcome.wav' to ESP32 SD card, played on Power On ('gong' sound)
- Library <AUDIO.H>: Needed for music or url streaming (via key commands), mandatory for INPUT_BUTTON_MODE (TTS), optional (for music streaming) in new REALTIME_MODE. Download the library zip file (with PSRAM [here](https://github.com/schreibfaul1/ESP32-audioI2S). Install in Arduino IDE via Sketch -> Include Library. IMPORTANT hint for **ESP32 without PSRAM** (see 'Known Issues'): using older AUDIO.H version [3.0.11g here]( https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/tree/main/libray_archive)) 
- Library <ReadyMail.h> optional, just used to archive INPUT_BUTTON_MODE conversations (sending CHAT history via EMail). Download and install latest mobizt ReadyMail library [zip file](https://github.com/mobizt/ReadyMail). Create a GMAIL account _with App password_ for the ESP32 device (How-to see: [here](https://theorycircuit.com/esp32-projects/simple-way-to-send-email-using-esp32/)), enter your personal credentials in header of <lib_openai_groq_chat.ino>.

# Hardware requirements
- Recommended: ESP32 or ESP32-S3 with (!) PSRAM (tested with ESP32-WROVER and ESP32-S3), no SD Card needed
- Alternatively (partially working) ESP32 without PSRAM (e.g. ESP32-WROOM-32), SD Card Module mandatory
- Older ESP32 (without PSRAM, no S3, no SD card) still working flawless in pure REALTIME_MODE (without music streaming). <br>
_Example code snippet will come next (TBD)_
- I2S digital microphone (e.g. INMP441) & I2S audio amplifier (e.g. MAX98357A) with speaker
- RGB status LED and optionally (recommended) an Analog Poti (for audio volume)
- Ready to Go devices (examples) with ESP32 & SD card reader: [Techiesms Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/)
- Ready to Go devices (examples) with ESP32-S3 (PSRAM): [Elato AI DIY](https://github.com/akdeb/ElatoAI), [Elato AI devices](https://www.elatoai.com)
- NEW (2026-01-06): **Published self-made PCB (Printed Circuit Board) with all Source and Gerber files as template for user DIY projects**. Feel free to use the files for own manufactoring orders or for own modifications. See Github folder [KALO AI Board](https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb).

# API Keys (Registration needed)
- LLM & TTS: **Open AI** API KEY needed (for BUTTON and REALTIME mode), registration: [Open AI account](https://platform.openai.com) (5$ free)
- INPUT_BUTTON_MODE only: GroqCloud LLM (fast): **GroqCloud** API KEY needed, registration: [groqcloud](https://console.groq.com/login) (using free account, token limited)
- INPUT_BUTTON_MODE only: STT (fast): **ElevenLabs** API KEY or STT (slow) **Deepgram** API KEY needed
- INPUT_BUTTON_MODE only: Optional **GMAIL** account for ESP32 device with App password (sending chat conversation via EMAIL).

# Known issues (PSRAM)
- _<AUDIO.H>_ library issue: ESP32 **without** PSRAM are no longer supported from current [AUDIO.H](https://github.com/schreibfaul1/ESP32-audioI2S) library (due limited heap), so the posted complete code won't work completelly on those ESP32 (e.g. Open AI voice response missed). <br>
Info: <Audio.h> is included just for 3 reasons: (1) music or url streaming, (2) playing music from SD card and (3) most important: Open AI TTS voice in INPUT_BUTTON_MODE. **Good news**: AUDIO.H library is NO LONGER needed for Open AI conversations in new REALTIME_MODE (reason: Audio I/O handling is completely native coded in <lib_openai_realtime.ino>).
- Solution for ESP32 without PSRAM: Option (A) - _using last (old) Audio.h library with NO-PSRAM support_: Version **3.0.11g** from July 18, 2024. Mirror link to 3.0.11g version [here]( https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/tree/main/libray_audioH_archive). Be aware: INPUT_BUTTON-MODE with Open AI friends working well (_but_ music streaming still won't work!). Option (B) - my recommendation for older ESP32 - _using new INPUT_REALTIME_MODE only_ (remove AUDIO.H library, no BUTTON_MODE, no music streaming). Benefit: Most reliable, elegant and compact code (any ESP32, no PSRAM or SD card needed at all). <br>
_I will share an example code next (link: TBD)_.  

# New features since Sept. 2026 (Major Update)
- Added new **Open AI REALTIME_MODE** -> Chatting with Open AI Realtime Server (Audio To Audio) **without pressing any Recording Button**, benefits: (+) faster AI response (low latency), (+) user emotion detection with emotional voice response, (+) context sensitive and significantly improved speech recognition quality, (+) lower demands on hardware, (+) easy to use (just speak & listen).
- **INPUT_REALTIME_MODE** architecture: Streaming User Request Audio to Open AI server & receiving LLM Respond AUDIO continuously via websockets stream. No PSRAM, neither SD Card or Audio.H library needed for this new INPUT_REALTIME_MODE.
- Using Palahis awesome library **<lib_openai_realtime.ino>**, _SHOUT-OUT and THANK YOU_ **@Palahis92** _for this great library!_. 
- Previous workflow (Recording via button, SST via ElevenLabs, LLM & TTS via OpenAI/Groq) still supported, labeled as **INPUT_BUTTON_MODE**
- Known **main features** from INPUT_BUTTON_MODE (e.g. calling/waking up other friends by name, local action key commands, url and music streaming, web-search on request) are **supported /embedded in new REALTIME_MODE too**. 

# New features since August 2025
- Supporting multiple custom chatbots/FRIENDS, activating any friend by call his/her name
- Each chatbot can be assigned with different TTS parameter (voice characteristics) via FRIENDS[] Agent structure
- Faster LLM AI response since supporting fast GroqCloud server API websockets (~ 2x faster than Open AI)
- GroqCloud server API allows to use LLM models from various provider (e.g. Meta, OpenAI, DeepSeek, PlayAI, Alibaba etc.), more details here: [models](https://console.groq.com/docs/models). Posted code (default settings): using Meta "llama-3.1-8b-instant" as CHAT model (low costs, high performance), for WEBSEARCH using Open AI 'gpt-4o..search' models
- New Commands, e.g. “DEBUG ON|OFF” to toggle print details, speaking "HASHTAG" to trigger "#" command
- Several minor bug fixes, e.g.: Sending LLM AI payload in chunks, keeping websockets open on ESP32 with PSRAM
- Cleaning up user specific settings in header of .ino files.

# New features since June 2025
- **PSRAM supported** for audio recording and transcription. SD Card no longer needed for ESP32 with PSRAM (tested with **ESP32**-WROVER and **ESP32-S3**). User #define settings for audio processing (#define RECORD_PSRAM / SDCARD)
- Additional **parameter added** to Recording and transcription functions: 'Recording_Stop()' and 'SpeechToText_xy()'
- Additional STT added: supporting **ElevenLabs Scribe v1 SpeechToText** API (as alternative to Deepgram STT). Multilingual support (also mixed languages in same record supported), country codes no longer needed Registration for API KEY needed [link](https://elevenlabs.io/de/pricing#pricing-table), cost free account supported 
- **Speed of SpeechToText significantly improved** (using ElevenLabs STT), in particular on **long sentences!**. Example: Short user voice recordings (e.g. 5 secs) are transcribed in ~ 0.5-2 secs (compared to ~5 sec with Deepgram), long user records (e.g. 20 secs!) transcribed in ~3 secs (compared to ~ 15 secs with Deepgram)  
- SpeechToText is **multi lingual**, detecting your spoken language automatically (~ 100 languages supported). As longer your spoken sentence (audio), as better the correct language detection 
- New Key word '**GOOGLE**': Activates Open AI **web search model** for Live Information requests 
- New Key word '**VOICE**': Enabling the new **Voice Instruction** parameter of Open AI TTS (user can force TTS character, e.g. "you are whispering"). _PSRAM needed_
- 'isRunning()' bug fix: Correct audio end detection (in past the LED still indicated Playing 1-2 secs after audio finished), solved with latest AUDIO.H version. _PSRAM needed_
- Updated to **latest models**. Open AI LLM: 'gpt-4.1-nano' & 'gpt-4o-mini-search-preview'. Deepgram STT: new MULTI lingual model 'nova-3-general' added, 'nova-2-general' still used for MONO lingual. ElevenLabs STT: 'scribe_v1'. Open AI TTS: 'gpt-4o-mini-tts' (for voice instruction) and 'tts-1' (default)
- Open AI TTS improved: **Audio streaming quality improved** (no longer clicking artefacts on beginning, letter cut offs at end resolved). _PSRAM needed_
- minor bugs resolved, added more detailed comments into sketch, code cleaned up.

# Github Updates
- **2026-09-12:** Major update. Continuous (button-less) Audio Recording & **Streaming to Open AI Realtime API** added (model: gpt-realtime-2.1-mini), embedded in Main.ino via new **INPUT_REALTIME_MODE** toggle (alternative previous INPUT_BUTTON_MODE still available). <br> 
_Library (NEW)_ **<lib_openai_realtime.ino>**: handling all new RealtimeAPI functions (SST, LLM, TTS) <br>
_Library (NEW)_ **<lib_friends.ino>**: Collecting all user defined FRIEND personalities. <br> 
_Library <lib_openai_groq_chat.ino>_: Added versatile **Web Search Integration**, embedding in REALTIME conversations too. **LLM models updated**, using 'GROQ openai/gpt-oss-20b', 'Open AI gpt-5-search-api'. <br> 
_Main sketch <KALO_ESP32_Voice_Chat_AI_Friends_RT_20260908>_: **loop() restructured** into smaller logical sections (supporting earlier INPUT_BUTTON_MODE - and - new INPUT_REALTIME_MODE). New **commands, e.g. Jukebox** (searching & playing a song from SD card via voice request). **Updated PCB templates**, additional devices supported. Bugfixes (VOL Poti, RGB LED updates, RealtimeAPI calls, and more)

- **2026-01-06:** GPIO pin assignments for **DFRobot ESP32-S3** (FireBeetle 2) added in main.ino -> supporting the **new PCB** [KALO AI Board](https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb) by default. Latest code supports SD modules with user custom pin assignments (beyond VSPI/HSPI defaults). Also moved all _I2S microphone_ pin assignments from _lib_audio_recording.ino_ to the _main.ino_ (KALO_ESP32_Voice_Chat_AI_Friends_20260106.ino) to collect ALL GPIO pin assignments AND user credentials on one central place in code. Addon (2026-01-06 to **2026-01-18** changes): SD modul bug fixed
- **2025-09-22:** **Email smtp send** feature added, purpose: archiving interesting CHAT dialogs. Updated 2 .ino files: lib_openai_groq_chat.ino and main.ino (KALO_ESP32_Voice..ino), no changes in other .ino files
- **2025-08-11:** Major update (see above). Supporting **custom chatbots/FRIENDS**, LLM AI response **2x faster (Groq)**
- **2025-06-28:** Hardware **pin assignments** cleaned up (3 PCB templates), Techiesms [Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/) supported by default (#define TECHIESMS_PCB removed). Added audio **VOL_BTN** to all devices without POTI
- **2025-06-19:** Supporting **ESP32-S3** I2S audio recording, supporting **Elato AI devices** [DIY pcb](https://github.com/akdeb/ElatoAI), [Elato AI products](https://www.elatoai.com)
- **2025-06-05:** Major update, detail see above (**PSRAM** support, **ElevenLabs Scribe v1 STT** added, **faster STT**)
- **2025-04-04:** Live Information Request capabilities added (supporting new **Open AI web search features**). Mixed usage of chat model and web search model supported. User queries with a user defined keyword initiate a web search and embed the result in the ongoing chat. Minor changes: all user specific credits are moved to header of main.ino sketch (KALO_ESP32_Voice_ChatGPT_20250404.ino), additional parameter added to function Open_AI(..) and SpeechToText_Deepgram(..). Code further cleaned up, detailed comments added in 'lib_OpenAI_Chat.ino'
- **2025-03-14:** Major enhancements: **Supporting techiesms hardware/pcb** [Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/). Code Insights: New toggle '#define TECHIESMS_PCB true' assigns all specific pins automatically (no user code changes needed). Minor enhancements: Welcome Voice (Open AI) added, RGB led colors updated, code clean up done
- **2025-01-26:** First drop, already working, not finally cleaned up (just posted this drop on some folks request).

# 
 

. . .

# Demo Videos
Video 01 (Jan. 27, 2025) – 1st drop:

[![Video - KALO-ESP32-Voice-ChatGPT](https://github.com/user-attachments/assets/8f236399-ff71-4dc3-9563-46cfe4e7fa91)](https://dark-controller.com/wp-content/uploads/2025/01/KALO-ESP32-Voice-ChatGPT-GQ.mp4)

_5 minute example chat. Using code default settings, System Prompt: Role of a 'good old friend'._
- Deepgram STT language: English (en-US)
- TTS voice: Multilingual (Open AI default), used voice in video: 'onyx'
- Overlay window: Serial Monitor I/O in real time (using Terminal App PuTTY, just for demo purposes)
- Details of interest (m:ss): 1:35 (BTN stops Audio), 2:05 (Radio gimmick), 3:15 (multi-lingual capabilities)

#
Video 02 (June 19, 2025) – Faster STT (SpeechToText via ElevenLabs) & Open AI Realtime websearch:

[![2025-06-19 Video - KALO-ESP32-Voice-ChatGPT - Picture_800](https://github.com/user-attachments/assets/a02cb561-a114-4a9a-9787-796239f79f71)](https://dark-controller.com/wp-content/uploads/2025/06/2025-06-19-Video-KALO-ESP32-Voice-ChatGPT-FINAL.mp4)

_3:30 minute dialog, using latest code with default settings. Same system prompt (role of a 'good old friend'). <br>
STT: Faster (multilingual) ElevenLabs scribe v1, TTS: Open AI voice 'onyx'. Open AI LLM: web search included._
- Hardware: Using the battery powered [Techiesms Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/) with ESP32 & SD card. Mounted on a Lasercut Acryl chassis (3.7V Li-Po battery placed inside double bottom), added a metal control (fixed steel button) as TOUCH button (connected to GPIO-12)
- Details of interest: Playing gong audio (welcome.wav on SD card), then real-time created (German) TTS welcome voice. Multilingual: Requesting Open AI to jump from (my) default 'German' to English language (min:sec 0:25). Recording long sentences, STT transcription still  < 2 secs (confirmed with short white led flashes). Live Information request: Key word 'Google' (min:sec 1:55) activates Open AI web search model _once_ (weather forecast 'today', June 19), embedded into the follow up dialog until end of session. Detail (min:sec 2:32) short record touch 'interrupts' TTS (2nd touch would 'repeat' last TTS again).

#
_**NEW**_ Video 03 (August 15, 2025) – **AI Friends** update (_multiple_ chatbots) & LLM AI speed improved (Groq API):

[![2025-08-15 Video - KALO-ESP32-Voice-AI-Friends (Elato ESP32S3)](https://github.com/user-attachments/assets/340eb127-321d-4918-ad55-a041cb0d44e2)](https://dark-controller.com/wp-content/uploads/2025/08/2025-08-15-Video-KALO-ESP32-Voice-AI-Friends-Elato-ESP32S3.mp4)

_6:13 minute dialog, latest August code with multiple AI friends on an ESP32S3 Elato device (default code settings)._ <br>
- Hardware: Using a battery powered [older version](https://github.com/StarmoonAI/Starmoon/blob/main/usecases.png) of the [Elato AI](https://www.elatoai.com) device, ESP32S3 with PSRAM (no SD card). Hint: Mounted a tiny metal screw as TOUCH record button (original side btn on side is used for audio volume)
- Demo: Chatting with 3 of my friends (Onyx, Veggi, Fred), waking up the buddies with calling by name, web search included. Using PSRAM and latest AUDIO.H allows to create voices with emotions
- Details of interest: Jumping from 'good old default friend' ONYX to the 'food specialist' VEGGI (min:sec 0:28), interrupting VEGGI on demand (1:57), waking-up FRED (the annoyed and aggressive buddy with an aggressive voice (2:00), listen to his emotions in voice! e.g. on 3:30 (hint: PSRAM is mandatory), waking up my earlier friendly ONYX again (fails on 3:50, no big deal: just calling him again on 4:09), and finally starting Google Search by request (4:18) to request latest data from TODAY (August 15!). Also might be of interest: Google websearch is embedded into Chat (ONYX himself speaks about Google on 5:30).
- RGB color (recap):  STT (cyan) -> LLM AI (blue) -> TTS (yellow>pink, looks white in video).  WHITE flash = Success. _NEW_: Double RED flash = waking up another _FRIEND_.

. . .

#
Links of interest, featuring friend’s projects:
- Advanced ESP32 AI service, using streaming sockets for lowest latency: [Github ElatoAI](https://github.com/akdeb/ElatoAI/tree/main/firmware-arduino), [Github StarmoonAI](https://github.com/StarmoonAI/Starmoon)
- Ready to Go hardware: [Techiesms Portable AI Voice Assistant](https://techiesms.com/product/portable-ai-voice-assistant/) 
- Ready to Go hardware: [Elato AI DIY](https://github.com/akdeb/ElatoAI), [Elato AI products](https://www.elatoai.com)

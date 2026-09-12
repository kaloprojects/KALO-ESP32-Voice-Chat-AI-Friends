
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                             KALO_ESP32_Voice_Chat_AI_Friends                             ------------------
// ------------------          ESP32 voice dialog device to chat with Multiple AI characters (FRIENDS)         ------------------
// ------------------                             Latest Update: Sept. 12, 2026                                ------------------
// ------------------                                     Coded by KALO                                        ------------------
// ------------------                                                                                          ------------------
// ------------------                             > INPUT_BUTTON_MODE Workflow <                               ------------------
// ------------------              Entering User request via Voice or via keyboard (Serial Monitor)            ------------------
// ------------------        Pre-recorded workflow (Voice Audio RECORDING on holding BTN) / no streaming       ------------------
// ------------------     [STT: Elevenlabs a/o Deepgram | LLM: OpenAI or GroqCloud | TTS: Open AI Voices]      ------------------
// ------------------     [LLM: any user favorites, e.g.: openai/gpt-oss-20b, gpt-4o-mini-search-preview]      ------------------
// ------------------                                                                                          ------------------
// ------------------              > ** [NEW since Sept. 2026] INPUT_REALTIME_MODE Workflow ** <               ------------------
// ------------------     ** Thanks so much to @Palahis92 for his awesome <lib_openai_realtime.ino> !! **      ------------------
// ------------------         Open AI Realtime STREAMING (with local key commands & web search support)        ------------------
// ------------------            (+) no button needed (+) FAST AI response (+) Emotional STT/LLM/TTS           ------------------
// ------------------                 Realtime enhancements in code are tagged with #REALTIME#                 ------------------
// ------------------                                                                                          ------------------
// ------------------                           > Hardware Requirements / Circuit <                            ------------------
// ------------------       ESP32/ESP32-S3 with PSRAM (optional SD Card) + I2S microphone + I2S Amplifier      ------------------
// ------------------               KALO AI Board PCB (DIY template), Source & Gerber files:                   ------------------
// ----------------    https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb   ----------------
// ------------------                                                                                          ------------------
// ------------------             other supported 'Ready to Go' devices (see PIN templates below):             ------------------
// ------------------              Elato AI, TechieSMS Assistant V1, TechieSMS Smart Assistant V3              ------------------
// ------------------------------------------------------------------------------------------------------------------------------


// *** Install hints:
// 1. in case of an 'Sketch too Large' Compiler Warning/ERROR in Arduino IDE (ESP32 Dev Module):
//    -> select a larger 'Partition Scheme' via menu > tools: e.g. using 'No OTA (2MB APP / 2MB SPIFFS) ***
// 2. TECHIESMS pcb: In case PC does not detect ESP32 Serial Port, then install CH340 driver (missed on older Windows versions)
//    -> driver links here: https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all#windows-710
// 3. Library dependencies:
//    - ESP32 with PSRAM:    use latest arduino-esp32 core (3.1 / ESP-IDF 5.3 or later) + latest AUDIO.H (3.2.0 or later)
//    - ESP21 without PSRAM: use latest arduino-esp32 core (3.1 / ESP-IDF 5.3 or later) + AUDIO.H (3.0.11g) !
//      Mirror to older AUDIO.H (3.0.11g): https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/tree/main/libray_archive


#define  VERSION                 "\n== KALO_ESP32_Voice_Chat_AI_Friends / Open AI REALTIME added (Sept. 12, 2026) ===========\n"
#define  VERSION_DATE            "20260912"   // format YYYYMMDD, used e.g. in EMail subject


// --- includes ----------------

#include <WiFi.h>                // only included here
#include <SD.h>                  // also needed in other tabs (.ino)

#include <WebSocketsClient.h>    // needed for #REALTIME (listed in main.ino sketch due Arduino WStype_t for realtime callback)

#include <WiFiClientSecure.h>    // only needed in other tabs (.ino)
 
#include <Audio.h>        // @Schreibfaul1 library, used for object 'audio_play' (SD/Radio streaming, Open AI TTS, Speechgen)
                          // [INPUT_BUTTON_MODE  ]: Audio.H needed for OpenAI TTS & music, not for: Audio Recording, STT, LLM
                          // [INPUT_REALTIME_MODE]: Audio.H NOT needed. Only needed for launching url/radio or SD music commands]

                          // - Hint: last AUDIO.H ver. 3.0.11g for older ESP32 without PSRAM, library mirror:
                          //   https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/libray_audioH_archive
                          // - latest AudioH for ESP32/S3 & PSRAM (multi-core chips like ESP32-S3, ESP32-S31 and ESP32-P4):
                          //   https://github.com/schreibfaul1/ESP32-audioI2S


// --- defines & macros -------- // DEBUG Toggle: 'true' enables, 'false' disables printing additional details in Serial Monitor

bool    DEBUG = true;            // <- Your Preference on Power On, can also be toggled during runtime via command 'DEBUG ON|OFF'
#define DebugPrint(x);           if(DEBUG){Serial.print(x);}
#define DebugPrintln(x);         if(DEBUG){Serial.println(x);}


// === PRIVATE credentials =====

const char* ssid =               "...";         // ### INSERT your wlan ssid    [mandatory]
const char* password =           "...";         // ### INSERT your password     [mandatory]

const char* OPENAI_KEY =         "...";         // LLM & TTS  - ### INSERT your OpenAI API KEY     [mandatory]
const char* GROQ_KEY =           "...";         // LLM (fast) - ### INSERT your GroqCloud API KEY  [mandatory]
const char* ELEVENLABS_KEY =     "...";         // STT (fast) - ### INSERT your ElevenLabs KEY     [mandatory]
const char* DEEPGRAM_KEY =       "";            // STT (slow) - ### INSERT your Deepgram KEY       [optional]   

// === user local settings =====

#define WEB_SEARCH_USER_CITY     "Bangkok"      // ### INSERT your user city, optimizes OpenAI 'websearch' [optional]
#define HELLO_TO_MY_FRIEND       "Hey there!"   // ### INSERT your inital 'Hello' to REALTIME (use your favorite language) 


// === user system settings ====

#define WELCOME_FILE    "/Welcome.wav"   // optional: 'Hello' file on SD will be played once on start (e.g. a gong)

                                         // Audio Volume for "Audio.H" 'audio_play' (BUTTON_MODE_TTS, URL Radio/TV, SD music)
                                         // #INDEPENDENT# to REALTIME_OUTPUT_GAIN for REALTIME AUDIO TTS <lib_openai_realtime>:
int AUDIOH_VOL_INIT =           21;      // <- Initial AudioH volume on Power On / possible values: 0-21 (e.g. start with MAX)
int AUDIOH_VOL_STEPS[]= {4,8,16,21};     // <- used for VOL_BTN only: user defined volumes (X steps possible, 4 just as example)


// === HARDWARE settings ======= // SELECT (uncomment) 1 of the pre-defined PCB (Printed Circuit Board) or use own settings ...

/* --- PCB: Luca 2.0 Duo/R ----- // ESP32 WROOM (N4R0, no PSRAM) & SD card -or- ESP32-WROVER-E (N8R8, 4MB PSRAM limited, w/wo SD)
// Reminder AUDIO.H:             // ESP32 WROOM (no PSRAM) needs AUDIO.H 3.0.11g, ESP32-WROVER-E (PSRAM) supports latest AUDIO.H
#define pin_I2S_DOUT    25       // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     26
#define pin_I2S_BCLK    27
#define I2S_WS          22       // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          35
#define I2S_SCK         33
#define I2S_LR          HIGH     // HIGH because L/R pin of INMP441 is connected to Vcc (RIGHT channel)
#define RECORD_PSRAM    false    // [INPUT_BUTTON_MODE only] true -> store recorded Audio in PSRAM (prerequisite: PSRAM)
#define RECORD_SDCARD   true     // [INPUT_BUTTON_MODE only] true -> store recorded Audio on SD Card (prerequisite: SD Card)
// -- optional controls:
#define NO_PIN          -1       // ALIAS definition for 'not used', supported on all optional control pins
#define SD_CS           5        // 4 x SD Card Reader pins, using VSPI defaults
#define SD_SCK          18
#define SD_MISO         19
#define SD_MOSI         23
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: [TECHIESMS]: w/wo Resistor + common GND, Analog)
#define pin_LED_RED     15       // [optional] 3 x Status LED R-G-B pins (rule in code: LOW switches led color ON)
#define pin_LED_GREEN   21       // [optional] Examples: KALO PCB 15-21-4, KALO Proto 15-2-0, TECHIESMS 15-2-4
#define pin_LED_BLUE    4
#define pin_RECORD_BTN  36       // [optional] RECORD tactile button on PCB (Printed Circuit Board), PULL-UP soldered
#define pin_TOUCH       13       // [optional] RECORD TOUCH control on TOP panel
#define pin_VOL_BTN     NO_PIN   // [optional] not needed (adjusting audio volume with continuous VOL_POTI)
#define pin_VOL_POTI    34       // [optional] Analogue wheel POTI as continuous audio volume control */

/* --- PCB: TECHIESMS V1 ------- // ESP32 WROOM (N4R0, no PSRAM) & SD: https://techiesms.com/product/portable-ai-voice-assistant/
// Reminder AUDIO.H:             // ESP32 WROOM (no PSRAM) on Techiesms device needs older AUDIO.H 3.0.11g
#define pin_I2S_DOUT    25       // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     26
#define pin_I2S_BCLK    27
#define I2S_WS          22       // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          35
#define I2S_SCK         33
#define I2S_LR          HIGH     // HIGH because L/R pin of INMP441 is connected to Vcc (RIGHT channel)
#define RECORD_PSRAM    false    // [INPUT_BUTTON_MODE only] true -> store recorded Audio in PSRAM (prerequisite: PSRAM)
#define RECORD_SDCARD   true     // [INPUT_BUTTON_MODE only] true -> store recorded Audio on SD Card (prerequisite: SD Card)
// -- optional controls:
#define NO_PIN          -1       // ALIAS definition for 'not used', supported on all optional control pins
#define SD_CS           5        // 4 x SD Card Reader pins, using VSPI defaults
#define SD_SCK          18
#define SD_MISO         19
#define SD_MOSI         23
#define flg_LED_DIGITAL false    // (!) FALSE: [TECHIESMS]: no Resistor + common GND (Analog) (TRUE: Serial Resistor, common Vcc)
#define pin_LED_RED     15       // [optional] 3 x Status LED R-G-B pins (rule in code: LOW switches led color ON)
#define pin_LED_GREEN   2
#define pin_LED_BLUE    4
#define pin_RECORD_BTN  36       // [optional] RECORD tactile button on PCB (Printed Circuit Board), PULL-UP soldered
#define pin_TOUCH       12       // [optional] #KALO# mod - additional RECORD TOUCH control on TOP panel (soldered to pin 12)
#define pin_VOL_BTN     13       // [optional] #KALO# mod - using side button for audio volume (orig. REPEAT button no longer needed)
#define pin_VOL_POTI    NO_PIN   // [optional] no wheel POTI available, using VOL_BTN for audio volume control */

/* --- PCB: TECHIESMS V3 ------- // ESP32-S3 Dev (N16R8, 8MB PSRAM, no SD Card) .. same chip as in ElatoAI (below):
// Techiesms details link:       // https://techiesms.com/product/xiaozhi-ai-voice-assistant-esp32-s3-based-smart-assistant/
#define pin_I2S_DOUT    7        // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     16
#define pin_I2S_BCLK    15
#define I2S_WS          4        // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          6
#define I2S_SCK         5
#define I2S_LR          LOW      // LOW (L/R pin GND, default LEFT channel)
#define RECORD_PSRAM    true     // [INPUT_BUTTON_MODE only] true -> store recorded Audio in PSRAM (prerequisite: PSRAM)
#define RECORD_SDCARD   false    // [INPUT_BUTTON_MODE only] true -> store recorded Audio on SD Card (prerequisite: SD Card)
// -- optional controls:
#define NO_PIN          -1       // ALIAS definition for 'not used', supported on all optional control pins
#define SD_CS           NO_PIN   // [optional] no SD card reader connected
#define SD_SCK          NO_PIN
#define SD_MISO         NO_PIN
#define SD_MOSI         NO_PIN
#define flg_LED_DIGITAL false    // (!) FALSE: [TECHIESMS]: no Resistor + common GND (Analog) (TRUE: Serial Resistor, common Vcc)
#define pin_LED_RED     8        // [optional] 3 x Status LED R-G-B pins (rule in code: LOW switches led color ON)
#define pin_LED_GREEN   19       // <- unfortunately connected to USB D-/D+ (pins 19/20) -> GREEN sometimes won't work on all pcb
#define pin_LED_BLUE    20       // <- solution: replace with a reliable LED (common GND), e.g. Kingbright WP154A4SUREQBFZGC
#define pin_RECORD_BTN  14       // [optional] using REPEAT side button
#define pin_TOUCH       NO_PIN   // [optional] not available
#define pin_VOL_BTN     40       // [optional] Volume increase control [VOL- Button 39 currently not used]
#define pin_VOL_POTI    NO_PIN   // [optional] no wheel POTI available, using VOL_BTN for audio volume control */

/* --- PCB: ElatoAI ------------ // ESP32-S3 Dev (N16R8, 8MB PSRAM): https://github.com/akdeb/ElatoAI / https://www.elatoai.com/
#define pin_I2S_DOUT    7        // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     5
#define pin_I2S_BCLK    6
#define I2S_WS          4        // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          14
#define I2S_SCK         1
#define I2S_LR          LOW      // LOW (L/R pin GND, default LEFT channel)
#define RECORD_PSRAM    true     // [INPUT_BUTTON_MODE only] true -> store recorded Audio in PSRAM (prerequisite: PSRAM)
#define RECORD_SDCARD   false    // [INPUT_BUTTON_MODE only] true -> store recorded Audio on SD Card (prerequisite: SD Card)
// -- optional controls:
#define NO_PIN          -1       // ALIAS definition for 'not used', supported on all optional control pins
#define SD_CS           NO_PIN   // [optional] no SD card reader connected
#define SD_SCK          NO_PIN
#define SD_MISO         NO_PIN
#define SD_MOSI         NO_PIN
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: without resistor, common GND, analogue)
#define pin_LED_RED     9        // [optional] 3 x Status LED R-G-B pins (rule in code: LOW switches led color ON)
#define pin_LED_GREEN   8
#define pin_LED_BLUE    13
#define pin_RECORD_BTN  NO_PIN   // [optional] #KALO# mod - no 2nd button available (RECORD side btn on pin 2 used as VOL_BTN
#define pin_TOUCH       3        // [optional] #KALO# mod - self soldered TOUCH (free: ESP32 12,14,32 / ESP32-S3: 3,12,14)
#define pin_VOL_BTN     2        // [optional] #KALO# mod - using original RECORD side button as audio volume control
#define pin_VOL_POTI    NO_PIN   // [optional] no wheel POTI available, using VOL_BTN for audio volume control */

// --- PCB: KALO AI BOARD ------ // DFRobot FireBeetle2 ESP32-S3 WROOM-1(N16R8, 8MB PSRAM) - KALO AI Board PCB:
// PCB Layout (battery powered): // https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb
// WIKI DFRobot Fire Beetle 2:   // https://wiki.dfrobot.com/SKU_DFR0975_FireBeetle_2_Board_ESP32_S3
#define pin_I2S_DOUT    4        // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     5
#define pin_I2S_BCLK    6
#define I2S_WS          8        // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          11
#define I2S_SCK         10
#define I2S_LR          LOW      // LOW (L/R pin GND, default LEFT channel)
#define RECORD_PSRAM    true     // [INPUT_BUTTON_MODE only] true -> store recorded Audio in PSRAM (prerequisite: PSRAM)
#define RECORD_SDCARD   false    // [INPUT_BUTTON_MODE only] true -> store recorded Audio on SD Card (prerequisite: SD Card)
// -- optional controls:
#define NO_PIN          -1       // ALIAS definition for 'not used', supported on all optional control pins
#define SD_CS           15       // [optional] 4 x SD Card Reader pins, optional (e.g. for playing music files)
#define SD_SCK          14
#define SD_MISO         12
#define SD_MOSI         13
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: without resistor, common GND, analogue)
#define pin_LED_RED     0        // [optional] 3 x Status LED R-G-B pins (rule in code: LOW switches led color ON)
#define pin_LED_GREEN   9
#define pin_LED_BLUE    18
#define pin_RECORD_BTN  38       // [optional] 1st Recording & STOP control: PUSH button
#define pin_TOUCH       7        // [optional] 2nd Recording & STOP control: TOUCH button
#define pin_VOL_BTN     NO_PIN   // [optional] not needed (adjusting audio volume with continuous VOL_POTI)
#define pin_VOL_POTI    3        // [optional] adjusting audio volume via poti


// --- global Objects ----------

Audio audio_play;                // AUDIO.H object for I2S stream

uint32_t gl_TOUCH_RELEASED;      // idle value (untouched), read once on Init, used internally as reference in loop()

enum AssistantInputMode {        // [NEW] - #REALTIME# toggle
     INPUT_BUTTON_MODE,
     INPUT_REALTIME_MODE
};
AssistantInputMode assistantInputMode = INPUT_BUTTON_MODE;    // Starting in earlier BUTTON mode (using button for recording,
                                                              // Elevenlabs for STT, Open AI TTS) - NO REALTIME on INIT


// -- REDUCE stack / increase HEAP ('magic line' might help on ESP32 without PSRAM (use ONLY IF url streaming/radio don't work !)
// Background: streaming audio via AUDIO.H without (!) PSRAM reaches the ESP32 heap limit, reducing 1K stack increases 4K heap
// Thanks for the idea to @Schreibfaul1 (AUDIO.H author), link: https://github.com/schreibfaul1/ESP32-audioI2S/issues/1039
// do NOT use this trick if PSRAM available (to keep full 8K STACK size, AUDIO.H streaming with PSRAM does NOT stress the heap)
/* SET_LOOP_TASK_STACK_SIZE(7 * 1024);    // reducing STACK (from default 8K) to 7K or 6K. Reduction of 1K gives 4kB more heap */


// --- Declaration of functions

void   UpdateAudioH_Volume_21(int maxVol = 21);      // Declaration for OPTIONAL parameter maxVol [Default 21]
void   led_RGB( bool red, bool green, bool blue );   // Declaration for auto recognition in other libraries (via weak)

String __attribute__((weak)) voice_SpeechGen(String t, String v, String p, String s, String e);  // [opt] <lib_tts_speechgen.ino>
String __attribute__((weak)) WebSearchResult(String Request, const char* open_key, bool flgEmbedding, const char* groq_key);


// --- Declaration of functions in other modules (not mandatory but ensures compiler checks correctly)
// splitting Sketch into multiple tabs see e.g. here: https://www.youtube.com/watch?v=HtYlQXt14zU

bool   I2S_Recording_Init();
bool   Recording_Loop();
bool   Recording_Stop( String* filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec );

String SpeechToText_Deepgram(   String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key );
String SpeechToText_ElevenLabs( String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key );

String OpenAI_Groq_LLM( String UserRequest, const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key );
void   get_tts_param( int* id, String* names, String* model, String* voice, String* vspeed, String* inst, String* hello );
void   Send_MESSAGES_SerialMonitor();
bool   Send_MESSAGES_Email();

// [NEW] #REALTIME# declarations - listed called functions only, all functions see new TAB: 'lib_openai_realtime.ino'

bool   RealtimeAPI_Start(const char* api_key);
void   RealtimeAPI_Stop();
String RealtimeAPI_Loop();
bool   RealtimeAPI_WaitUntilReady(uint32_t timeout_ms);
bool   RealtimeAPI_AddUserText(String text);
bool   RealtimeAPI_RestartForCurrentCharacter(const char* api_key, uint32_t timeout_ms);
bool   RealtimeAPI_SwitchToCharacterByName(String UserRequest);
String RealtimeAPI_CurrentCharacterName();

// [NEW] additional #REALTIME# declarations (coded by #KALO#):

bool   RealtimeAPI_isListening();
bool   RealtimeAPI_StartLLM_GetAudio(uint32_t timeout_ms, bool printLive);
String RealtimeAPI_GetAudioTranscription();
String RealtimeAPI_CurrentCharacterVoice();
bool   RealtimeAPI_DeleteLastConversationItem();



// ******************************************************************************************************************************

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  Serial.setTimeout(100);    // 10 times faster reaction after CR entered (default is 1000ms)

  // Pin assignments (Hint: Some ESP32 pins do NOT support INPUT_PULLUP, requiring external R (e.g. pins 34-39)
  if (pin_LED_RED    != NO_PIN)  { pinMode(pin_LED_RED,   OUTPUT); }
  if (pin_LED_GREEN  != NO_PIN)  { pinMode(pin_LED_GREEN, OUTPUT); }
  if (pin_LED_BLUE   != NO_PIN)  { pinMode(pin_LED_BLUE,  OUTPUT); }
  if (pin_VOL_BTN    != NO_PIN)  { pinMode(pin_VOL_BTN,   INPUT_PULLUP); }
  if (pin_RECORD_BTN != NO_PIN)  { pinMode(pin_RECORD_BTN,INPUT_PULLUP); }
  led_RGB(LOW,LOW,LOW); led_RGB(HIGH,HIGH,HIGH);  // init [ON|OFF]

  // on INIT: walk 1 sec thru 3 RGB colors (RED -> GREEN -> BLUE) .. YELLOW until WiFi connected
  led_RGB(LOW,HIGH,HIGH); delay (330);  // ## RED
  led_RGB(HIGH,LOW,HIGH); delay (330);  // ## GREEN
  led_RGB(HIGH,HIGH,LOW); delay (330);  // ## BLUE
  led_RGB(LOW,LOW,HIGH);                // ## YELLOW ...

  // Init TOUCH idle value (untouched)
  gl_TOUCH_RELEASED = (pin_TOUCH != NO_PIN) ? (touchRead(pin_TOUCH)) : 0;

  // INIT microphone INMP441
  I2S_Recording_Init();

  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT );

  // Hello World
  Serial.println( VERSION );

  // Connecting to WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("> Connecting WLAN " );
  while (WiFi.status() != WL_CONNECTED)
  { Serial.print(".");  delay(500);
  }
  Serial.println(". Done, device connected.");
  led_RGB( HIGH,LOW,HIGH );   // LED ## GREEN: device connected


  // INIT [OPTIONAL] SD Card Reader - SPI.begin() & SD.begin() for optional SD PINS
  bool flg_SD_found = false;
  if (SD_SCK != NO_PIN && SD_MISO != NO_PIN && SD_MOSI != NO_PIN && SD_CS != NO_PIN)
  {  pinMode(SD_CS, OUTPUT);  digitalWrite(SD_CS, HIGH); // Ensure SD card is disabled until SPI init is done
     SPI.begin( SD_SCK, SD_MISO, SD_MOSI, SD_CS );       // Using predefined SPI instance (VSPI bus): no 'var SPIClass' needed
     delay(100);                                         // Waiting until Vcc 100% stable on SD card module
     flg_SD_found = SD.begin( SD_CS, SPI, 10000000 );    // Reduce SD module speed to reliable 10 Mhz (instead default 40/80)
     /*
     Alternative: using 2nd SPI controller (HSPI bus) for SD card in case VSPI needed for e.g. display connector):
     -> SPIClass spiSD(HSPI) ... spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS) ... SD.begin(SD_CS, spiSD, 10000000)
     Info link:  https://randomnerdtutorials.com/esp32-microsd-card-arduino/#sdcardcustompins
     Global SPI: https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/I2Saudio_SD/I2Saudio_SD.cpp  */
  }  /* bug fix: removed default 'SD.begin()' -> solved the missed BLUE LED issue (Elato AI pcb)! */

  // HELLO - Optional: Playing a optional WELCOME_FILE wav file from SD Card once (with reduced volume)
  // feature can be used on any ESP32 with SD card (independent of Recording settings RECORD_PSRAM vs. RECORD_SDCARD)
  if ( flg_SD_found )
  {  if ( SD.exists( WELCOME_FILE ) )
     {  audio_play.connecttoFS( SD, WELCOME_FILE );
        while (audio_play.isRunning())         // Trick: waiting in setup() until PLAY is done
        {  UpdateAudioH_Volume_21(12);         // reduced volume (e.g. 12 of 21), scaling optional VOL POTI
           audio_play.loop();
        }
     }
  }

  // INIT done, starting user interaction
  Serial.println( "\nWorkflow:" );
  Serial.println( "> Hold or Touch button during recording voice -OR- enter request in Serial Monitor" );
  Serial.println( "> -OR- [NEW]: key word [REALTIME, German UNTERHALTUNG] toggles BUTTON <-> REALTIME mode" );
  Serial.println( "> Select another AI FRIEND by calling his NAME, example: \"Hi FRED, are you online?\"" );
  Serial.println( "> Key word [GOOGLE | INTERNET] inside request: toggle LLM to Open AI web search model" );
  Serial.println( "> Key word [RADIO | DAILY NEWS | TAGESSCHAU] inside request: start audio url streaming" );
  Serial.println( "> Key word [PLAY|SONG|MUSIC|MUSIK|TRACK|LIED|JUKEBOX]..xy: Play file *xy* from SD card" );
  Serial.println( "> Command  [RESET]: reset (initialize and reboot) ESP32" );
  Serial.println( "> Command  [#] or speaking [HASHTAG]: print CHAT history & Friends list in Serial Monitor" );
  Serial.println( "> Command  [@] or Key word [EMAIL] inside request: send CHAT history to user via email" );
  Serial.println( "> Command  [DEBUG ON|OFF]: enable|disable workflow details in Serial Monitor\n" );
  Serial.println( "=========================================================================================\n");

  // HELLO - Speak the FRIEND (Agent) specific a FRIENDS[].welcome sentence (with FRIEND specific settings/voice)
  TextToSpeech( "#WELCOME" );      // '#WELCOME' triggers an internal command in TextToSpeech()
}



// ******************************************************************************************************************************

void loop()
{
  String UserRequest = "";              // IMPORTANT hint: all vars (requests & responses) set "" for each new loop iteration
  String RealtimeUserRequest = "";      // [NEW] user request via Realtime API, initialized new each loop pass
  String LLM_Feedback = "";             // LLM AI response
  static String LLM_Feedback_before;    // static var to keep information from last request (as alternative to global var)

  String   record_SDfile;               // 4 vars are used for receiving recording details
  uint8_t* record_buffer;
  long     record_bytes;
  float    record_seconds;


  // ------ Read USER INPUT: Option (1) - Enter in Serial Monitor via KEYBOARD --------------------------------------------------
  // ------ Use case example: Using ESP32 as AI TEXT Chat device, text chatting or triggering key commands

  while (Serial.available() > 0)                        // Definition: returns numbers ob chars after CR done
  {                                                     // NON blocking 'while loop' (ending up here only after CR done)
    UserRequest = Serial.readStringUntil('\n');
    UserRequest.replace("\r", "");
    UserRequest.replace("\n", "");
    UserRequest.trim();

    if (UserRequest != "")
    {  // usability: stop Audio after keyboard CR (launch key commands or chats immediately)
       if (audio_play.isRunning()) {audio_play.stopSong();}

       Serial.println( "\nYou> [" + UserRequest + "]" );
    }
  }


  // ------ USER INPUT Option (2) [INPUT_BUTTON MODE] - Press button for VOICE RECORDORDING & TRANSCRIPTION (ElevenLabs STT) ----
  // ------ Use case: ESP32 as VOICE chat device: Recording (as long pressing or touching a button) -> Transcription on Release
  // ------ 3 different BTN actions: PRESS & HOLD for recording || STOP LLM AI speaking || REPEAT last LLM AI answer

  if (assistantInputMode == INPUT_BUTTON_MODE && isButtonActive())  // Recording started, supporting btn and touch sensor
  {  if (audio_play.isRunning()) { audio_play.stopSong(); }         // double check - but shouldn't happen anyhow, see eof loop()

     Recording_Loop();                                              // !! Main task: Recording AUDIO (ongoing) !! (not blocking)
  }

  if (assistantInputMode == INPUT_BUTTON_MODE && !isButtonActive()) // Recording not started yet OR stopped (on release)
  {
     // now we check if RECORDING is done, we receive recording details (length etc..) via &pointer
     // hint: Recording_Stop() is true ONCE when recording finalized and .wav is available

     if (Recording_Stop( &record_SDfile, &record_buffer, &record_bytes, &record_seconds ))
     {  if (record_seconds > 0.4)                                 // using short btn TOUCH (<0.4 secs) for other actions
        {  led_RGB(HIGH,LOW,LOW);                                 // LED [Update]: ## CYAN indicating STT (or WEB SEARCH)
           Serial.print( "\nYou {STT}> " );                       // function SpeechToText_Deepgram will append '...'

           // Action happens here! -> Launching SpeechToText (STT) transcription (WAITING until done)
           // using ElevenLabs STT as default (best performance, multi-lingual, high accuracy (language & word detection)
           // Reminder: as longer the spoken sentence, as better the results in language and word detection ;)

           UserRequest = SpeechToText_ElevenLabs( record_SDfile, record_buffer, record_bytes, "", ELEVENLABS_KEY );
           /* // alternatives (for user with DEEPGRAM API key):
           // MULTI-lingual:   .. = SpeechToText_Deepgram( record_SDfile, record_buffer, record_bytes, "",   DEEPGRAM_KEY );
           // Single language: .. = SpeechToText_Deepgram( record_SDfile, record_buffer, record_bytes, "en", DEEPGRAM_KEY );*/

           if (UserRequest != "")                                 // Done!. In case we got a valid spoken transcription:
           {  led_RGB(LOW,LOW,LOW); delay(200);                   // LED: ## WHITE FLASH (200ms) indicating success
              led_RGB(HIGH,HIGH,HIGH); delay(100);                // LED: ## OFF (until update at eof loop()
           }
           Serial.println( "[" + UserRequest + "]" );             // printing result in Serial Monitor always
        }
        else                                                      // additional Actions on short button PRESS (< 0.4 secs):
        {
           Serial.println( "\n< REPEAT TTS >" );                  // REPEAT last LLM answer (if audio currently not playing)
           led_RGB(LOW,LOW,HIGH);                                 // LED: ## YELLOW indicating 'TTS' audio pending
           TextToSpeech( LLM_Feedback_before );                   // launching TTS with previous answer
        }
     }
  }


  // ------ [NEW] #REALTIME# USER INPUT Option (3) [INPUT_REALTIME_MODE] - Open AI Realtime (no BTN needed) - FAST & EMOTIONAL
  // ------ Using new library lib_openai_realtime - Coded by @Palahis92 [** Huge SHOUT-OUT and THANK YOU to @Palahis92 !! **]
  // ------ Toggling between previous INPUT_BUTTON_MODE and new INPUT_REALTIME_MODE via keyword "REALTIME ON|OFF"

  if (assistantInputMode == INPUT_REALTIME_MODE)
  {  if (!audio_play.isRunning())                   // special case: do not start REALTIME microfone if Audio.H is playing music
     {  RealtimeUserRequest = RealtimeAPI_Loop();   // Default: RECORD User AUDIO (non blocking in bkgd) + receive STT response
        if (RealtimeUserRequest != "")
        {  Serial.println( "\n[RT] You> [" + RealtimeUserRequest + "]" );
           UserRequest = RealtimeUserRequest;       // UserRequest: used for CMD keywords check (instead TTS)
        }                                           // RealtimeUserRequest: used to repeat Request if needed (e.g. FRIEND changes)
     }
  }


  // ------ USER REQUEST DONE -> Checking KEYWORDS (supporting all 3 User request options above) --------------------------------
  // Hint: Commands are enabled for #REALTIME# mode too !! :)

  if ( Keyword_Commands( UserRequest ) )                          // Parse for keywords and launch actions in Keyword_Commands()
  {  UserRequest = "";  // to skip any further LLM actions
  }


  // ====== [NEW] #REALTIME# [INPUT_REALTIME_MODE]: LLM & TTS -> Launching Realtime LLM + AUDIO OUT (TTS) =======================

  if (assistantInputMode == INPUT_REALTIME_MODE)
  {
     // ------ (1) SYNC User VOICE requests & KEYBOARD requests

     if (UserRequest != "" && RealtimeUserRequest == "")  {RealtimeAPI_AddUserText(UserRequest);}      // keyboard used
     if (UserRequest == "" && RealtimeUserRequest != "")  {RealtimeAPI_DeleteLastConversationItem();}  // ignore local actions


     //  ------ (2) Enable 'Actual Web Search Requests' into REALTIME (a bit tricky workflow inside WebSearchResult() ;) ..
     // .. asking RT for a 'wait a moment..' voice (TTS) -> then checking web myself -> assemble new request (with encapsulated
     // web search result) and return String. Then send (via RealtimeAPI_AddUserText) as new request to RT LLM -> LLM & TTS

     // [hint: Triggering whole workflow with user defined keywords 'inside' request. 'GOOGLE' is just my keyword (it's NOT
     // a Google search). User could define any keywords, e.g. GOOGLE, INTERNET, WEATHER, {German} WETTER, ACTUAL NEWS' etc.)

     if (UserRequest != "" && WebSearchResult)     // !! check if WebSearchResult() exists  <lib_openai_groq_chat.ino> or NULL
     {   String cmd = UserRequest; cmd.toUpperCase(); cmd.replace(" ", "");
         if (cmd.indexOf("GOOGLE") >=0 || cmd.indexOf("INTERNET") >=0 || cmd.indexOf("WEATHER") >=0 || cmd.indexOf("WETTER") >=0)
         {  LLM_Feedback = WebSearchResult( UserRequest, OPENAI_KEY, true, GROQ_KEY );   // true for encapsulated web search
            if (LLM_Feedback != "")
            {  Serial.print( "\n[SEARCH]");                  // .. CRLF after walking Search dots '.... & Prefix to [RT] below
               RealtimeAPI_AddUserText(LLM_Feedback);        // hint: 'RealtimeAPI_AddUserText()' simulates a spoken instruction
            }                                                // .. [RT!] LLM answer creation and TTS speech happens in (4) below
         }
     }

     // ------ (3) Check if user 'calls' a new FRIEND in REALTIME mode (hint: also possible via keyboard / UserRequest != "")

     if (UserRequest != "")
     {  if (RealtimeAPI_SwitchToCharacterByName(UserRequest))  // true if OTHER friend (alias) called (updates gl_CURR_FRIEND)
        {  String new_friend_name = RealtimeAPI_CurrentCharacterName();
           Serial.println( "[RT] < REALTIME CHARACTER: Restarting session with [" + new_friend_name + "] >" );

           // flashing LED: ## RED / BLACK  twice (to indicate successful friend change)
           led_RGB(LOW,HIGH,HIGH); delay(100); led_RGB(HIGH,HIGH,HIGH); delay(100);
           led_RGB(LOW,HIGH,HIGH); delay(100); led_RGB(HIGH,HIGH,HIGH); delay(100);
           led_RGB(LOW,LOW,HIGH);  // at end -> LED: ## YELLOW indicating 'waiting' (to avoid a black moment)

           RealtimeAPI_RestartForCurrentCharacter(OPENAI_KEY, 10000);    // start new session
           RealtimeAPI_AddUserText(UserRequest);                         // Trick: repeat earlier User request to new friend
        }
     }

     // ------ (4) Main Task: Launching Realtime LLM + Realtime AUDIO OUT (TTS) of LLM response (blocking tasks)

     if (UserRequest != "")
     {  String friend_name  = RealtimeAPI_CurrentCharacterName();
        String friend_voice = RealtimeAPI_CurrentCharacterVoice();
        Serial.print( "[RT] Livestream [" + friend_name + "|" + friend_voice + "] Live Stream> ");

        led_RGB(HIGH,HIGH,LOW); // #BLUE = speaking
        RealtimeAPI_StartLLM_GetAudio( 45000, true);   // true= Live Transcription     // LLM & Audio response (pending function)
        /* waiting here .. until done (last word spoken) */

        Serial.println();
        String aiText = RealtimeAPI_GetAudioTranscription();
        aiText.replace(". ",". \n");  // Adding CRLF for better readability
        Serial.println( "[RT] LLM [" + friend_name + "]> [" + aiText + "]" );          // LLM Text Response [optional]
        UserRequest = "";
     }

  }  // ====== eof [NEW] #REALTIME# enhancements ================================================================================


  // ------ INPUT_BUTTON_MODE: LLM & TTS -> Call OpenAI_Groq_LLM (Chat | websearch), play response TTS (Audio.H) ----------------
  // Calling LLM in Chat Completion OR Web Search mode. Utilizing web search feature via keyword ( e.g. "GOOGLE")
  // Recap: OpenAI_Groq_LLM() remembers complete history (appending prompts) to support ongoing dialogs (web searches included)

  if (UserRequest != "" && assistantInputMode == INPUT_BUTTON_MODE)
  {
    // ------ (1) LLM mode WEBSEARCH: launch Open AI WEB SEARCH feature if user request includes the keyword 'Google'
    // supporting User requests like 'Will it rain tomorrow in my region?, please ask Google'
    // [hint: Triggering whole workflow with user defined keywords 'inside' request. 'GOOGLE' is just my keyword (it's NOT
    // a Google search). User could define any keywords, e.g.'WEB SEARCH', 'INTERNET', 'WEATHER', 'ACTUAL NEWS' etc.]

    String cmd = UserRequest; cmd.toUpperCase(); cmd.replace(" ", "");
    if ( WebSearchResult &&        // check if WebSearchResult() exists in <lib_openai_groq_chat.ino> or NULL
       ( cmd.indexOf("GOOGLE") >=0 || cmd.indexOf("INTERNET") >=0 || cmd.indexOf("WEATHER") >=0 || cmd.indexOf("WETTER") >=0) )
    {  Serial.print( "LLM AI WEB SEARCH> " );                     // function OpenAI_Groq_LLM() will Serial.print '...'
       LLM_Feedback = WebSearchResult( UserRequest, OPENAI_KEY, false, GROQ_KEY );   // false for no Realtime embedding
    }
    else

    // ------ (2) LLM mode CHAT [DEFAULT]: LLM chat completion model (human like conversations)
    {  led_RGB(HIGH,HIGH,LOW);                                    // LED: ## BLUE indicating LLM AI CHAT Request starting
       Serial.print( "LLM AI CHAT> " );                           // function OpenAI_Groq_LLM() will Serial.print '...'
       LLM_Feedback = OpenAI_Groq_LLM( UserRequest, OPENAI_KEY, false, GROQ_KEY );  // 'false' means: default CHAT model
    }

    // ------ final tasks (always):

    if (LLM_Feedback != "")                                       // in case we got any valid feedback ..
    { led_RGB(LOW,LOW,LOW); delay(200);                           // -> LED: ## WHITE FLASH (200ms)
      led_RGB(HIGH,HIGH,HIGH); delay(100);
      // getting name of current LLM agent (friend):
      int id;  String names, model, voice, vspeed, instruction, welcome;
      get_tts_param( &id, &names, &model, &voice, &vspeed, &instruction, &welcome );
      Serial.println( " [" + names + "]" + " [" + LLM_Feedback + "]" );
      LLM_Feedback_before = LLM_Feedback;
    }
    else Serial.print("\n");

    // ------ TTS -> Speak LLM answer (using Open AI voices by default, voice settings done in TextToSpeech()

    if (LLM_Feedback != "")
    {  led_RGB(LOW,LOW,HIGH);                // LED: ## YELLOW indicating 'TTS' audio pending
       TextToSpeech( LLM_Feedback );         // TextToSpeech() manages voice parameter for current active AI agent (FRIEND[x])
     }
  }


  // ----------------------------------------------------------------------------------------------------------------------------
  // AUDIO.H (Schreibfaul1) loop for Play Audio (details here: https://github.com/schreibfaul1/ESP32-audioI2S))


  // Continuous Audio.H Volume Update
  if (assistantInputMode == INPUT_BUTTON_MODE || audio_play.isRunning())    // if false: using CONTROLS for REALTIME Audio ..
  {  // Adjusting AUDIO-H Volume (via AUDIOH_VOL_INIT | POTI | BTN )        // [inside RealtimeAPI_Loop() + 'Real..GetAudio()]
     UpdateAudioH_Volume_21();
  }

  audio_play.loop();
  vTaskDelay(1);

  if ( isButtonActive() && audio_play.isRunning() )       // use RECORD button to stop any ongoing Audio
  {  audio_play.stopSong();
     Serial.println( "\n< STOP AUDIO >" );
     delay(30);  // unbouncing
     while (isButtonActive());   // wait until release (to avoid new recording)
  }


  // ----------------------------------------------------------------------------------------------------------------------------
  // update permanent LED status always (in addition to momentary LED events, eg. WHITE or RED flashes, YELLOW, CYAN etc.)

  if (assistantInputMode == INPUT_BUTTON_MODE)
  {  if (isButtonActive())               {led_RGB(LOW,HIGH,HIGH);} // led RED as long the record ongoing
     else if (audio_play.isRunning())    {led_RGB(LOW,HIGH,LOW); } // led MAGENTA if Open AI voice is speaking (or url streaming)
     else                                {led_RGB(HIGH,LOW,HIGH);} // led GREEN (default): ready for next request
  }

  if (assistantInputMode == INPUT_REALTIME_MODE) // [NEW]
  {  if (audio_play.isRunning())         {led_RGB(LOW,HIGH,LOW); } // MAGENTA (spec.case: Audio.H music or url during REALTIME)
     else if (RealtimeAPI_isListening()) {led_RGB(LOW,HIGH,HIGH);} // REALTIME - RED:   automated user audio recording (VAD open)
     else                                {led_RGB(LOW,LOW, LOW );} // REALTIME - WHITE: default Ready & waiting in REALTIME
  }

}

// end of LOOP() ****************************************************************************************************************



// Helper functions:



// ------------------------------------------------------------------------------------------------------------------------------
// audio_info(const char *info) is an inbuilt (optional) Callback function in AUDIO.H, printing details of played Audio events
// ------------------------------------------------------------------------------------------------------------------------------
// Disabled by default - uncomment in case any AUDIO debugging needed:
/* void audio_info(const char *info)
{  // printing AUDIO.H details in DEBUG mode (except rarely warning "webfile chunked: not enough bytes available for skipCRLF".
   // Details: https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/issues/4
   // uncomment this line to receive AUDIO.H details in DEBUG mode):
   String info_str = (String) info;
   if ( info_str.indexOf("skipCRLF") == -1 ) { DebugPrintln( "AUDIO.H info: " + info_str ); }
} */



// ------------------------------------------------------------------------------------------------------------------------------
// RealtimeAPI_RestoreAudioH(): OPTIONAL (WEAK). Declaration and calls in <lib_openai_realtime.ino> when Realtime Audio finished
// - Info: Optional function is called twice inside function 'RealtimeAPI_StartLLM_GetAudio()' in  <lib_openai_realtime.ino>
// - Function is NOT needed (and can be removed) in case no <Audio.h> #included / no audio objects used [we use audio_play.xy()]
// ------------------------------------------------------------------------------------------------------------------------------

void RealtimeAPI_RestoreAudioH()
{ audio_play.stopSong();
  delay(80);
  audio_play.setPinout(pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT);
  delay(20);
  DebugPrintln("< RealtimeAPI_RestoreAudioH() [optional in main.ino]: Audio.h I2S settings restored >");
}



// ------------------------------------------------------------------------------------------------------------------------------
// Updating LED with RGB on/off values
// ------------------------------------------------------------------------------------------------------------------------------

void led_RGB( bool red, bool green, bool blue )
{ // general usage: using LOW in code switches the LED color on, e.g. led_RGB(LOW,HIGH,HIGH) means RED on

  static bool red_before=LOW, green_before=LOW, blue_before=LOW;  // memo: LOW = false = ON  (ESP32 default on Power On)
  if (flg_LED_DIGITAL)   // Default [KALO pcb or Elato AI]: using complete Vcc (soldered serial resistors exist), common Anode
  {  if (red   != red_before   && pin_LED_RED   != NO_PIN) {digitalWrite(pin_LED_RED,red);     red_before=red;    }
     if (green != green_before && pin_LED_GREEN != NO_PIN) {digitalWrite(pin_LED_GREEN,green); green_before=green;}
     if (blue  != blue_before  && pin_LED_BLUE  != NO_PIN) {digitalWrite(pin_LED_BLUE,blue);   blue_before=blue;  }
  }
  if (!flg_LED_DIGITAL)  // Exception [e.g. TECHIESMS pcb]: LED with common GND, missed resistors (so we must reduce voltage)
  {  // writing analog values 0 or max. ~40 (not 255!, means no digitalWrite!)
     if (red   != red_before   && pin_LED_RED   != NO_PIN) {analogWrite(pin_LED_RED,  (red==LOW)?   40:0 ); red_before=red;    }
     if (green != green_before && pin_LED_GREEN != NO_PIN) {analogWrite(pin_LED_GREEN,(green==LOW)? 40:0 ); green_before=green;}
     if (blue  != blue_before  && pin_LED_BLUE  != NO_PIN) {analogWrite(pin_LED_BLUE, (blue==LOW)?  40:0 ); blue_before=blue;  }
  }
}



// ------------------------------------------------------------------------------------------------------------------------------
// TextToSpeech() - Using (by default) OpenAI TTS services via function 'openai_speech()' in @Schreibfaul1's AUDIO.H library.
// - Automatically using tts parameter from current active AI FRIEND (calling 'get_tts_param()' in lib_openai_groq.ini)
// - Any other TTS service (beyond Open AI) could be added below, see example snippets for free Google TTS and SpeechGen TTS
// - IMPORTANT: Be aware of AUDIO.H dependency -> CHECK bottom line (UPDATE in case older AUDIO.H used) to avoid COMPILER ERRORS!
// ------------------------------------------------------------------------------------------------------------------------------

void TextToSpeech( String p_request )
{
   // OpenAI voices are multi-lingual :) .. 9 tts-1 voices (August 2025): alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer
   // Supported audio formats (response): aac | mp3 | wav (sample rate issue) | flac (PSRAM needed)
   // Known AUDIO.H issue: Latency delay (~1 sec) before voice starts speaking (performance improved with latest AUDIO.H)

   // - Link OpenAI TTS doc: https://platform.openai.com/docs/guides/text-to-speech/text-to-speech
   // - Link OpenAI TTS API reference: https://platform.openai.com/docs/api-reference/audio/createSpeech
   // - Link to test voices: https://platform.openai.com/playground/tts
   // - Link to more complex voice instruction prompts: https://www.openai.fm/

   // Params of AUDIO.H .openai_speech():
   // - model:          Available TTS models: tts-1 | tts-1-hd | gpt-4o-mini-tts (gpt models needed for 'voice instruct' !)
   // - request:        The text to generate audio for. The maximum length is 4096 characters.
   // - voice_instruct: (Optional) forcing voice style (e.g. "you are whispering"). Needs AUDIO.H >= 3.1.0 & gpt-xy-tts !
   // - voice:          Voice name (multilingual), May 2025: alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer
   // - format:         supported audio formats: aac | mp3 | wav (sample rate issue) | flac (PSRAM needed)
   // - vspeed:         The speed of the generated voice. Select a value from 0.25 to 4.0. DEFAULT: 1.0

   static int id_before;
   int id; String names, model, voice, vspeed, instruction, welcome;
   get_tts_param( &id, &names, &model, &voice, &vspeed, &instruction, &welcome );  // requesting tts values for current FRIEND

   if (p_request == "#WELCOME") {p_request = welcome;}              // intrinsic # command: 'speak' FRIENDS[].welcome entry

   Serial.print( "TTS Audio [" + names + "|" + voice + "|" + model + "]" );  // e.g. TTS AUDIO [SUSAN | nova/gpt-4o-mini-tts]
   Serial.println();

   if (model == "SPEECHGEN")     // Special Case 1: optionally using SpeechGen.io TTS (instead OpenAI) on dedicated voices
   {  // using my function 'voice_SpeechGen(..)' of <lib_tts_speechgen.ino> link: older 'kaloprojects/KALO-ESP32-Voice-Assistant'
      // I am using it for a dedicated 'CHILD' voice (German 'Gisela' or English 'Anny' or e.g. "Kasper", "Hannah plus" etc)

      if (voice_SpeechGen)       // check if [weak] function SpeechGen() exists -> just add <lib_tts_speechgen.ino> to enable
      {  String mp3_url = voice_SpeechGen( p_request, voice, "1", vspeed, "good" );  // request/voice/pitch/speed/emotion
         if (mp3_url != "")
         {  audio_play.connecttohost( mp3_url.c_str() );
         }
      }
      return;   // DONE.
   }

   if (model == "GOOGLE_TTS")    // Special Case 2: using free Google TTS (just for demo purposes, I am not using, too limited)
   {  // in case you prefer free GOOGLE TTS (free of usage, but less 'human' & LIMITED in length / long sentences won't work !!)
      // - Google TTS are mono lingual only, language tag needed, e.g. en, en-US, en-IN, en-BG, en-AU, de-DE, th-TH etc.
      // - https://cloud.google.com/text-to-speech/docs/voices
      // - using this TTS on friend X: initialize 'Agents FRIENDSFriends[X].tts_model' with "GOOGLE_TTS" (in lib_OpenAI_Chat.ino)
      audio_play.connecttospeech( p_request.c_str(), "en");
      return;   // DONE.
   }

   // using Open AI TTS voice by DEFAULT:
   /* - AUDIO.H dependencies (IMPORTANT): The amount of .openai_speech() parameters changed in AUDIO.H library versions
   // - Background: since AUDIO.H ver. '3.1.0u' a 4th parameter 'voice instruction' is added (usage e.g.: "you are whispering")
   // - choose the correct amount of parameter to avoid COMPILATION ERRORS (and set the wrong one into comment) .. */
   // ==> So CHECK your AUDIO.H library & update this line in code if needed ! ....

   audio_play.openai_speech( OPENAI_KEY, model, p_request, instruction, voice, "aac", vspeed);  // <- use if version >= 3.1.0u
/* audio_play.openai_speech( OPENAI_KEY, model, p_request,              voice, "aac", vspeed);  // <- use this for 3.0.11g ! */
}



// ## NEW in 2026-04 ------------------------------------------------------------------------------------------------------------
// bool JukeboxCommandsSuccessful( String cmd )
// ------------------------------------------------------------------------------------------------------------------------------
// SEARCH & PLAY audio file (mp3/wav) from SD card (folder \music), multiple keywords/synonyms (filename pattern search)
// Current limitation: Plays 1st matching song only (stops on song end, not playing 'next' song')
//
// Params:   User request String: searching for keywords, if found -> extract rest of String (behind keyword) for song search
// - user is saying "Jukebox Billie"         -> ESP finds/plays e.g. song file: [Michael Jackson - Billie Jean.mp3]
// - or e.g.:  user is saying "Music Abba"   -> ESP finds/plays e.g. song file: [Abba - Best of (2000).mp3]
// - "Play me a track from Depeche Mode"     -> ESP finds/plays e.g. song file: [Depeche Mode - In Your Room.mp3]
// - "I wish the song 'Freestyler' "         -> ESP finds/plays e.g. song file: [Bomfunk - Mc S Freestyler Bomfunk.mp3]
// - "Do you have a song from Seal?"         -> ESP finds/plays e.g. song file: [Jakatta feat. Seal - My Vision.mp3]
// - etc.                                    IMPORTANT: PSRAM is mandatory (AUDIO.H issue)
//
// RETURN:   true: if song found any playing started (used in calling function to avoid further LLM workflows)
// ------------------------------------------------------------------------------------------------------------------------------
//

bool JukeboxCommandsSuccessful( String cmd )
{
  String music_folder = "/music" ;              // name of music sub folder (alternative: using "/" for root)

  bool flg_FoundAndPlaying = false;

  // Searching for any leading 'PLAY _ MUSIC' commands (supporting synonyms) .. if found: replace with marker, e.g. '*'
  cmd.replace("*","");   // just to make sure the cmd did not have any marker yet
  cmd.replace("PLAY ","*");    cmd.replace("SONG ","*");  cmd.replace("MUSIC ","*"); cmd.replace("TRACK ","*");
  cmd.replace("JUKEBOX ","*"); cmd.replace("LIED ","*");  cmd.replace("MUSIK ","*");

  // removing most common punctuation marks (and filling words after commands)
  cmd.replace(".","");    cmd.replace(",","");   cmd.replace("?","");
  cmd.replace("\"","");   cmd.replace("\'","");  cmd.replace("\\","");
  cmd.replace("FROM",""); cmd.replace("VON","");

  // finally cut all spoken words prior marker (so any human 'bla bla' prior keyword are ignored)
  cmd = cmd.substring( cmd.lastIndexOf("*")  );

  if ( cmd.indexOf("*") >= 0 )                  // PLAY MUSIC commands found -> start searching for files on SD card
  {  cmd.replace("*",""); cmd.trim();           // finally remove keyword marker
     led_RGB(LOW,HIGH,HIGH); delay(200);        // LED: ## RED Flash (200ms) on detected KEYWORD

     Serial.println( "< PLAY MUSIC command detected, searching for [" + cmd + "] >" );

     // search for any matching file on SD card
     String foundfile = "";
     File root = SD.open( music_folder + "/");
     while ( File file = root.openNextFile() )
     { String fname = file.name();
       String fname_upper = fname; fname_upper.toUpperCase();
       if (fname_upper.indexOf(cmd) >= 0) { foundfile = fname; }
       file.close();
       if (foundfile != "")
       {  break;
       }
     } root.close();

     if ( foundfile != "" )                                                       // matching song found -> PLAY Audio
     {  led_RGB(LOW,LOW,LOW); delay(200); led_RGB(HIGH,HIGH,HIGH); delay(100);    // -> LED: ## WHITE FLASH (200ms)
        foundfile = music_folder + "/" + foundfile;
        Serial.println( "< Playing Song: " + foundfile + " >");
        audio_play.connecttoFS( SD, foundfile.c_str() );
        flg_FoundAndPlaying = true;
     }
     else
     {  Serial.println( "< No matching file found >");
     }
  }

  return ( flg_FoundAndPlaying );
}



// ------------------------------------------------------------------------------------------------------------------------------
// UpdateAudioH_Volume_21( [optionalMax] ): Updating Audio Out (values 0-21) for AudioH objects (TTS, Streaming, SD files)
// - Updating Audio.H object volume at any time, multi control support (VOL POTI | VOL BTN & LED FLASH | NONE with fixed INIT)
// - optionalMax: optional limiting & scaling VOL POTI and AUDIOH_VOL_INIT (Button values AUDIOH_VOL_STEPS[] not modified)
//   [use case for Max limiting: e.g. UpdateAudioH_Volume_21(10) reduces welcome file volume (still using whole POTI range]
// ------------------------------------------------------------------------------------------------------------------------------

void UpdateAudioH_Volume_21( int optionalMax )
{
  static int volume_before = -1;

  optionalMax = constrain(optionalMax, 0, 21);                 // no (or wrong) parameter: Maximum 21 (see function declaration)

  if (volume_before == -1)  // init once                       // default volume (if no controls exist or not used yet) to ..
  {  int initVol = map(AUDIOH_VOL_INIT,0,21,0,optionalMax);    // .. AUDIOH_VOL_INIT (or less if optionalMax used)
     audio_play.setVolume(AUDIOH_VOL_INIT);
     volume_before = AUDIOH_VOL_INIT;
  }

  if (pin_VOL_POTI != NO_PIN)     // --- KALO default: if available then using an POTI to adjust Audio Volume each 100 ms
  {  static long millis_before = millis();
     static uint32_t adc_filtered = 0;

     // reading POTI, rarely only (e.g. 10 times/sec) to avoid unnecessary audio flickering & actions
     if (millis() > (millis_before + 100))                     // [NEW] 100ms instead earlier 250ms
     { millis_before = millis();

       int raw = analogRead(pin_VOL_POTI);                     // using a tiny smoothing algorythm (avoid flickering)
       if (adc_filtered == 0) adc_filtered = raw;              // Init first run
       adc_filtered = (adc_filtered * 3 + raw) >> 2;           // smoothing signal, weight: 3/4 old + 1/4 new
       int volume = map( adc_filtered,0,4095,0,optionalMax );  // POTI top view: tuning clockwise for Volume 0 -> 21

       if ( volume != volume_before )
       {  Serial.print(   "New Audio.H Volume: [" + (String) volume + "]. " );
          if (pin_TOUCH != NO_PIN)
          {  Serial.println("Touch Value: [" + (String)touchRead(pin_TOUCH) + "] (Threshold: " +(String)gl_TOUCH_RELEASED+ ")");
          }
          audio_play.setVolume(volume);  // AUDIO: values from 0 to 21
          volume_before = volume;
       }
     }
  }

  if (pin_VOL_BTN != NO_PIN)      // --- Alternative: using a VOL_BTN to toggle thru N values (e.g. TECHISMS or Elato AI pcb)
  {  static bool flg_volume_updated = false;
     static int volume_level = -1;
     if (digitalRead(pin_VOL_BTN) == LOW && !flg_volume_updated)
     {  int steps = sizeof(AUDIOH_VOL_STEPS) / sizeof(AUDIOH_VOL_STEPS[0]);
        volume_level = ((volume_level+1) % steps);  // walking in circle (starting with 0): e.g. 0 -> 1 -> 2 -> 0 ..
        Serial.print( "New Audio.H Volume: [" + (String) volume_level + "] = " + (String) AUDIOH_VOL_STEPS[volume_level] + ". ");
        if (pin_TOUCH != NO_PIN)
        {  Serial.println( "Touch Value: [" + (String)touchRead(pin_TOUCH) + "] (Threshold: " +(String)gl_TOUCH_RELEASED+ ")");
        }
        // visualize 'new level' with 1-N FASHES in Audio.H color schemata GREEN/MAGENTA (hint: Realtime Audio volume using RED)
        for (int i=0; i<=volume_level; i++)
        {   if( audio_play.isRunning()) {led_RGB(HIGH,LOW,HIGH); delay(20); led_RGB(HIGH,HIGH,HIGH); delay(40);}  // ## GREEN
            if(!audio_play.isRunning()) {led_RGB(LOW, HIGH,LOW); delay(20); led_RGB(HIGH,HIGH,HIGH); delay(40);}  // ## MAGENTA
        }

        audio_play.setVolume( AUDIOH_VOL_STEPS[volume_level] );
        volume_before = AUDIOH_VOL_STEPS[volume_level];
        flg_volume_updated = true;
    }
    if (digitalRead(pin_VOL_BTN) == HIGH && flg_volume_updated)
    {  flg_volume_updated = false;
    }
  }
}



// ------------------------------------------------------------------------------------------------------------------------------
// isButtonActive(): Return status of AUDIO RECORDING controls (PUSH or TOUCH button)
// ------------------------------------------------------------------------------------------------------------------------------

bool isButtonActive()
{
  bool flg_RECORD_BTN;               // both flags used to trigger recording AND for RGB led status at eof loop()
  bool flg_RECORD_TOUCH;

  // 1. PUSH BUTTON (if available) - check RECORD BUTTON status LOW & HIGH (result: flg_RECORD_BTN is LOW | HIGH)

  if (pin_RECORD_BTN != NO_PIN)
  {  flg_RECORD_BTN = (digitalRead(pin_RECORD_BTN) == LOW)? true : false;
  }  else flg_RECORD_BTN = false;  // no button available -> never pressed

  // 2. TOUCH BUTTON (if available) - check if finger is touching button (result: flg_RECORD_TOUCH is true | false)

  /* Detail: ESP32 and ESP32-S3 return totally different touch values !, code below should handle both scenarios
  // ESP32:    low  uint16_t values !, examples: idle values (UN-touched) ~ 70-80, TOUCHED: 'falls down to' about 10-40
  // ESP32-S3: high uint36_t values !, examples: idle values (UN-touched) ~ 22.000-28.000, TOUCHED: rises to 35.000-120.000! */

  uint32_t current_touch;
  if (pin_TOUCH != NO_PIN)
  {  current_touch = touchRead(pin_TOUCH);
     if (current_touch < 16383 )  // ESP32: idle value examples (UN-touched) ~ 70-80, TOUCHED: down to 10-40
     {  flg_RECORD_TOUCH = (current_touch <= (uint32_t) (gl_TOUCH_RELEASED * 0.9)) ? true : false; // ESP32 rule: more than 10%
     }
     else  // ESP32-S3: idle values (UN-touched) ~ 22.000-28.000, TOUCHED: up to 30.000-120.000  // ESP32-S3: rising 10% or more
     {  flg_RECORD_TOUCH = (current_touch >  (uint32_t) (gl_TOUCH_RELEASED * 1.1)) ? true : false;
     }
  }  else flg_RECORD_TOUCH = false;  // no touch button available -> never touched

  return (flg_RECORD_BTN || flg_RECORD_TOUCH);
}



// ------------------------------------------------------------------------------------------------------------------------------
// Keyword_Commands(String UserRequest): Parsing UserRequest and launching Action (supported in #REALTIME# mode too ! :)
// ------------------------------------------------------------------------------------------------------------------------------

bool Keyword_Commands( String UserRequest )
{
  // Hint: Commands are supported also in #REALTIME# mode ! :)

  String cmd = UserRequest;
  cmd.toUpperCase(); cmd.replace(".", "");

  // 1. keyword 'RADIO' inside the user request -> Playing German RADIO Live Stream: SWR3
  // Use case example (Recording request): "Please play radio for me, thanks" -> Streaming launched

  if (cmd.indexOf("RADIO") >=0 )
  {  Serial.println( "< Streaming German RADIO: SWR3 >" );
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD
     led_RGB(LOW,LOW,HIGH);                                       // LED: ## YELLOW indicating 'Stream' pending -> .. MAGENTA
     // HINT !: the streaming can fail on some ESP32 without PSRAM (AUDIO.H issue!), in this case: deactivate/remove next line:
     audio_play.connecttohost( "https://liveradio.swr.de/sw282p3/swr3/play.mp3" );
     return (true);
  }

  // 2. keyword 'DAILY NEWS' or German 'TAGESSCHAU' inside request-> Playing German TV News: Tagesschau24
  // Use case example (Recording request): "Please stream daily news for me!" -> Streaming launched

  if (cmd.indexOf("DAILY NEWS") >=0 || cmd.indexOf("TAGESSCHAU") >=0 )
  {  Serial.println( "< Streaming German Daily News TV: Tagesschau24 >" );
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD
     led_RGB(LOW,LOW,HIGH);                                       // LED: ## YELLOW indicating 'Stream' pending -> .. MAGENTA
     // HINT !: the streaming can fail on some ESP32 without PSRAM (AUDIO.H issue!), in this case: deactivate/remove next line:
     audio_play.connecttohost( "https://icecast.tagesschau.de/ndr/tagesschau24/live/mp3/128/stream.mp3"  );
     return (true);
  }

  // 3. key COMMAND 'RESET' (command vs. keyword: exact single word) -> Reset ESP32 (my use cases: Print firmware details)
  if (cmd == "RESET")
  {  ESP.restart();
  }

  // 4. Toggling DEBUG mode ON|OFF during runtime (ON: enable progress details in Serial Monitor, OFF: minimize Serial I/O)
  if (cmd.indexOf("DEBUG ON") >=0)
  {  Serial.println( "< DEBUG ON >");                             // toggling DEBUG mode 'ON' via command (keyboard or STT)
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD
     DEBUG = true;
     return (true);
  }
  if (cmd.indexOf("DEBUG OFF") >=0)
  {  Serial.println( "< DEBUG OFF >");                            // toggling DEBUG 'OFF'
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD
     DEBUG = false;
     return (true);
  }

  // 5. key COMMAND '#' or speaking SINGLE word 'HASHTAG' -> Serial Monitor Print of complete CHAT history
  // -- available in INPUT_BUTTON_MODE only, because MESSAGES history String not used for REALTIME conversations --
  if (cmd == "#" || cmd == "HASHTAG")
  {  if (assistantInputMode == INPUT_BUTTON_MODE)                 // MESSAGES are used in BUTTON mode only
     {  led_RGB(LOW,HIGH,HIGH); delay(200);                       // LED: ## RED Flash (200ms) on detected KEYWORD
        Send_MESSAGES_SerialMonitor();                            // Getter function (printing var MESSAGES in lib..chat.ino)
        return (true);
     }
  }

  // 6. key COMMAND '@' or keyword 'EMAIL' INSIDE request sentence -> sending complete CHAT history to user email account
  // -- available in INPUT_BUTTON_MODE only, because MESSAGES history String not used for REALTIME conversations --
  cmd.replace("E-MAIL","EMAIL"); cmd.replace("E MAIL", "EMAIL");
  if (cmd == "@" || cmd.indexOf("EMAIL") >=0)
  {  if (assistantInputMode == INPUT_BUTTON_MODE)                 // MESSAGES are used in BUTTON mode only
     {  Serial.println( "< Email request, sending complete chat history to smtp server >");
        led_RGB(LOW,HIGH,HIGH); delay(200);                       // LED: ## RED Flash (200ms) on detected KEYWORD
        Send_MESSAGES_Email();                                    // Sending var MESSAGES in lib..chat.ino via Email
        Serial.println( "< Done. Email sent >");
        TextToSpeech("OK. Done");                                 // Audio feedback confirmation (only on EMAIL command)
        return (true);
     }
  }

  // 7. [NEW]: keywords [PLAY|SONG|MUSIC|MUSIK|TRACK|LIED|JUKEBOX] -> Search & Play one called song from SD card
  if ( JukeboxCommandsSuccessful(cmd) )                           //  [NEW] since April 2026
  {   return (true);
  }

  // 9. [NEW]: #REALTIME# add-on (Toogling between INPUT_BUTTON_MODE and INPUT_REALTIME_MODE (using same toggle command):
  if (cmd.indexOf("REALTIME") >=0 || cmd.indexOf("REAL-TIME") >=0 || /*German*/ cmd.indexOf("UNTERHALTUNG") >=0 )
  {
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD

     if (assistantInputMode == INPUT_REALTIME_MODE)
     {  RealtimeAPI_Stop();
        assistantInputMode = INPUT_BUTTON_MODE;
        Serial.println( "< REALTIME OFF > " );
        TextToSpeech( "#WELCOME" );                               // '#WELCOME' triggers the internal HELLO in TextToSpeech()
     }
     else
     {  RealtimeAPI_Start(OPENAI_KEY);
        assistantInputMode = INPUT_REALTIME_MODE;
        Serial.println( "< REALTIME ON >  " );
        led_RGB(LOW,LOW,HIGH);                                    // LED: ## YELLOW (waiting Realtime server connection)
        RealtimeAPI_WaitUntilReady(10000) ;
        RealtimeAPI_AddUserText( HELLO_TO_MY_FRIEND );            // forcing Realtime API server for a welcome Audio HELLO
        led_RGB(HIGH,HIGH,LOW);                                   // LED: ## BLUE (Realtime Speaking)
        RealtimeAPI_StartLLM_GetAudio( 45000, true);              // LLM & TTS (true = Serial.println "Live" Transcription)
        /* waiting here .. until Audio done) */
     }
     return (true);  // skip any upcoming LLM a/o TTS
  }

  /* 8. web search keywords e.g. 'GOOGLE', 'INTERNET' etc -> launching Open AI WEB SEARCH feature (embedded in conversation)
  // ... handled in LLM calls (INPUT_BUTTON_MODE and INPUT_REALTIME_MODE supported) */

  return (false);
}

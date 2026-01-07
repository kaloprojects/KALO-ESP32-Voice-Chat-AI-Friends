
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                            KALO_ESP32_Voice_Chat_AI_Friends                              ------------------
// ------------------        ESP32 voice dialog device to chat with Multiple AI characters (FRIENDS)           ------------------
// ------------------                      [Update to previous KALO_ESP32_Voice_ChatGPT]                       ------------------
// ------------------                             Latest Update: Jan. 6, 2026                                  ------------------
// ------------------                                   Coded by KALO                                          ------------------
// ------------------                                                                                          ------------------
// ------------------     Self coded libs: Voice Recording, Transcription, Chat with LLM, TTS via AUDIO.H      ------------------
// ------------------     [STT: Elevenlabs a/o Deepgram | LLM: OpenAI or GroqCloud | TTS: Open AI Voices]      ------------------
// ------------------     [LLM: any user favorites, e.g. llama-3.1-8b-instant, OpenAI realtime websearch]      ------------------
// ------------------                                                                                          ------------------
// ------------------                                    > Workflow <                                          ------------------
// ------------------             Entering User request via Voice or via keyboard (Serial Monitor)             ------------------
// ------------------      Pre-recorded workflow (Voice Audio RECORDING on holding BTN) / no streaming         ------------------
// ------------------                                                                                          ------------------
// ------------------                         > Hardware Requirements / Circuit <                              ------------------
// ------------------        ESP32/ESP32-S3 with PSRAM (or SD Card) + I2S microphone + I2S Amplifier           ------------------
// ------------------                         (see pin examples/templates below)                               ------------------
// ------------------                                                                                          ------------------
// ------------------                      > Hardware PCB (Printed Circuit Boards) <                           ------------------
// ------------------            [NEW]: KALO AI Board PCB (DIY template), Source & Gerber files:               ------------------
// ------------------  https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb ------------------
// ------------------                         other supported 'Ready to Go' devices:                           ------------------   
// ------------------     Elato AI: DIY https://github.com/akdeb/ElatoAI | home: https://www.elatoai.com/      ------------------
// ------------------     TechieSMS Assistant: https://techiesms.com/product/portable-ai-voice-assistant/      ------------------
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


#define  VERSION                 "\n== KALO_ESP32_Voice_Chat_AI_Friends (last update: Jan. 6, 2026) ========================\n"   
#define  VERSION_DATE            "20260106"   // format YYYYMMDD, used e.g. in EMail subject


// --- includes ----------------

#include <WiFi.h>                // only included here
#include <SD.h>                  // also needed in other tabs (.ino) 
#include <WiFiClientSecure.h>    // only needed in other tabs (.ino)

#include <Audio.h>               // @Schreibfaul1 library, used for PLAYING Audio (via I2S Amplifier) -> mandatory for TTS only
                                 // [not needed for: Audio Recording - Audio STT Transcription - AI LLM Chat]
                                 // - Reminder: older AUDIO.H ver. 3.0.11g needed for all ESP32 without PSRAM, library mirror:
                                 //   https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/libray_archive 
                                 //
                                 // AUDIO History - bug fixes / chats of interest for this ESP32_Voice_ChatGPT project:
                                 // - 3.0.11g [mandatory]: 8bit wav fix https://github.com/schreibfaul1/ESP32-audioI2S/issues/786
                                 //                        non PSRAM: https://github.com/schreibfaul1/ESP32-audioI2S/issues/1039
                                 // - 3.1.0u  [features]:  @akdeb added OpenAI voice instruction parameter (PSRAM needed !)
                                 //                        https://github.com/schreibfaul1/ESP32-audioI2S/pull/1003
                                 // - 3.1.0w  [bug fix]:   .isRunning fix (led indicates playing ~ 1-2 too long (PSRAM needed!)
                                 //                        https://github.com/schreibfaul1/ESP32-audioI2S/issues/1020
                                 // - 3.3.0a  [bug fix]:   .openai_speech() supporting Voice Instructions (PSRAM needed !)  
                                 //                        .openai_speech() playing flawless (nothing cut on audio start or end) 
                                 //                        https://github.com/schreibfaul1/ESP32-audioI2S/issues/1050  
                               

// --- defines & macros -------- // DEBUG Toggle: 'true' enables, 'false' disables printing additional details in Serial Monitor

bool    DEBUG = true;            // <- Your Preference on Power On, can also be toggled during runtime via command 'DEBUG ON|OFF'
#define DebugPrint(x);           if(DEBUG){Serial.print(x);}   
#define DebugPrintln(x);         if(DEBUG){Serial.println(x);}  


// === PRIVATE credentials =====

const char* ssid =               "...";   // ### INSERT your wlan ssid    [mandatory]
const char* password =           "...";   // ### INSERT your password     [mandatory]

const char* OPENAI_KEY =         "...";   // LLM & TTS  - ### INSERT your OpenAI API KEY     [mandatory]
const char* GROQ_KEY =           "...";   // LLM (fast) - ### INSERT your GroqCloud API KEY  [mandatory]
const char* ELEVENLABS_KEY =     "...";   // STT (fast) - ### INSERT your ElevenLabs KEY     [mandatory]
const char* DEEPGRAM_KEY =       "";      // STT (slow) - ### INSERT your Deepgram KEY       [optional]   

 
// === user settings =========== 

#define WELCOME_FILE             "/Welcome.wav"  // optional: 'Hello' file on SD will be played once on start (e.g. a gong)

int gl_VOL_INIT =       21;      // Initial Audio volume on Power On / possible values: 0-21 (e.g. start with max. Volume 21) 
int gl_VOL_STEPS[]= {11,16,21};  // used for VOL_BTN only: user defined audio volume steps (X steps possible, 3 just as example),
                                 // 1st click: STEPS[0]. High values best for TTS, lower volumes for e.g. Radio streaming   


// === HARDWARE settings ======= // SELECT (uncomment) 1 of the pre-defined PCB (Printed Circuit Board) or use own settings ... 
// ## NEW [2026-01-06]: KALO AI Board PCB added + SD Card custom pins + I2S micro pins (no longer in lib_audio_recording.ino) 
                                 
/* --- PCB: Luca 2.0 Duo/R ----  // ESP32 WROOM (N4R0, no PSRAM) & SD card -or- ESP32-WROVER-E (N8R8, 4MB PSRAM limited, w/wo SD) 
// Reminder AUDIO.H:             // ESP32 WROOM (no PSRAM) needs AUDIO.H 3.0.11g, ESP32-WROVER-E (PSRAM) supports latest AUDIO.H 
#define NO_PIN          -1       // ALIAS 'not used' -> supported on optional pins (4xSD, RECORD_BTN, TOUCH, VOL_POTI, VOL_BTN)  
#define SD_CS           5        // 4 x SD Card Reader pins, using VSPI defaults 
#define SD_SCK          18       
#define SD_MISO         19       
#define SD_MOSI         23   
#define I2S_WS          22       // 4 x I2S Audio input pins for microphone INMP441 (referenced in 'lib_audio_recording.ino')     
#define I2S_SD          35           
#define I2S_SCK         33   
#define I2S_LR          HIGH     // HIGH because L/R pin of INMP441 is connected to Vcc (RIGHT channel)
#define pin_I2S_DOUT    25       // 3 x I2S Audio output pins (e.g. for I2S Amplifier MAX98357) 
#define pin_I2S_LRC     26       
#define pin_I2S_BCLK    27        
#define pin_LED_RED     15       // Status R-G-B pins (Examples: KALO PCB 15-21-4,  KALO Proto 15-2-0, TECHIESMS 15-2-4) 
#define pin_LED_GREEN   21       // .. rule in code below: LOW switches led color ON 
#define pin_LED_BLUE    4  
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: [TECHIESMS]: w/wo Resistor + common GND, Analog)
#define pin_RECORD_BTN  36       // RECORD tactile button on PCB (Printed Circuit Board), PULL-UP soldered
#define pin_TOUCH       13       // RECORD TOUCH control on TOP panel
#define pin_VOL_POTI    34       // Analogue wheel POTI as continuous audio volume control
#define pin_VOL_BTN     NO_PIN   // not needed (adjusting audio volume with continuous VOL_POTI) */ 

/* --- PCB: TECHIESMS device --- // ESP32 WROOM (N4R0, no PSRAM) & SD: https://techiesms.com/product/portable-ai-voice-assistant/
// Reminder AUDIO.H:             // ESP32 WROOM (no PSRAM) on Techiesms device needs older AUDIO.H 3.0.11g 
#define NO_PIN          -1       // #ALIAS 'not used' -> supported on optional pins (4xSD, RECORD_BTN, TOUCH, VOL_POTI, VOL_BTN)  
#define SD_CS           5        // 4 x SD Card Reader pins, using VSPI defaults 
#define SD_SCK          18       
#define SD_MISO         19       
#define SD_MOSI         23     
#define I2S_WS          22       // 4 x I2S Audio input pins for microphone INMP441 (referenced in 'lib_audio_recording.ino')     
#define I2S_SD          35           
#define I2S_SCK         33   
#define I2S_LR          HIGH     // HIGH because L/R pin of INMP441 is connected to Vcc (RIGHT channel)
#define pin_I2S_DOUT    25       // 3 x I2S Audio output pins (e.g. for I2S Amplifier MAX98357)  
#define pin_I2S_LRC     26       
#define pin_I2S_BCLK    27        
#define pin_LED_RED     15       // Status LED R-G-B pins (Examples: KALO PCB 15-21-4,  KALO Proto 15-2-0, TECHIESMS 15-2-4) 
#define pin_LED_GREEN   2        // .. rule in code below: LOW switches led color ON 
#define pin_LED_BLUE    4  
#define flg_LED_DIGITAL false    // (!) FALSE: [TECHIESMS]: no Resistor + common GND (Analog) (TRUE: Serial Resistor, common Vcc)
#define pin_RECORD_BTN  36       // RECORD tactile button on PCB (Printed Circuit Board), PULL-UP soldered
#define pin_TOUCH       12       // # KALO mod # - additional RECORD TOUCH control on TOP panel (soldered to pin 12)
#define pin_VOL_POTI    NO_PIN   // no analogue wheel POTI available, using VOL_BTN for audio volume control  
#define pin_VOL_BTN     13       // # KALO mod # - using side button for audio volume (orig. REPEAT button no longer needed) */

/* --- PCB: ElatoAI ------------ // ESP32-S3 Dev (N16R8, 8MB PSRAM): https://github.com/akdeb/ElatoAI / https://www.elatoai.com/  
#define NO_PIN          -1       // ALIAS 'not used' -> supported on optional pins (4xSD, RECORD_BTN, TOUCH, VOL_POTI, VOL_BTN)  
#define SD_CS           NO_PIN   // no SD card reader connected 
#define SD_SCK          NO_PIN           
#define SD_MISO         NO_PIN        
#define SD_MOSI         NO_PIN       
#define I2S_WS          4        // 4 x I2S Audio input pins for microphone INMP441 (referenced in 'lib_audio_recording.ino')     
#define I2S_SD          14           
#define I2S_SCK         1   
#define I2S_LR          LOW      // LOW because L/R pin of INMP441 is connected to GND (default LEFT channel)      
#define pin_I2S_DOUT    7        // 3 x I2S Audio output pins (e.g. for I2S Amplifier MAX98357)  
#define pin_I2S_LRC     5        
#define pin_I2S_BCLK    6        
#define pin_LED_RED     9        // Status LED R-G-B pins (rule in code below: LOW switches led color ON)  
#define pin_LED_GREEN   8        
#define pin_LED_BLUE    13  
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: without resistor, common GND, analogue)
#define pin_TOUCH       3        // # KALO mod # - self soldered RECORD TOUCH (free: ESP32 12,14,32 / ESP32-S3: 3,12,14) 
#define pin_VOL_POTI    NO_PIN   // no analogue wheel POTI available, using VOL_BTN for audio volume control 
#define pin_VOL_BTN     2        // # KALO mod # - using original RECORD side button used as audio volume control 
#define pin_RECORD_BTN  NO_PIN   // # KALO mod # - no 2nd button available (original RECORD side btn on pin 2 used as VOL_BTN) */

// --- PCB: KALO AI BOARD ------ // ## NEW - DFRobot FireBeetle2 ESP32-S3 WROOM-1(N16R8, 8MB PSRAM) - KALO AI Board PCB:
// PCB Layout (battery powered): // https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb
// WIKI DFRobot Fire Beetle 2:   // https://wiki.dfrobot.com/SKU_DFR0975_FireBeetle_2_Board_ESP32_S3
#define NO_PIN          -1       // ALIAS 'not used' -> supported on optional pins (4xSD, RECORD_BTN, TOUCH, VOL_POTI, VOL_BTN)  
#define SD_CS           15       // 4 x SD Card Reader pins, optional (e.g. for playing music files) 
#define SD_SCK          14          
#define SD_MISO         12        
#define SD_MOSI         13       
#define I2S_WS          8        // 4 x I2S Audio input pins for microphone INMP441 (referenced in 'lib_audio_recording.ino')     
#define I2S_SD          11           
#define I2S_SCK         10   
#define I2S_LR          LOW      // LOW because L/R pin of INMP441 is connected to GND (default LEFT channel)    
#define pin_I2S_DOUT    4        // 3 x I2S Audio output pins (for I2S Amplifier MAX98357) 
#define pin_I2S_LRC     5        
#define pin_I2S_BCLK    6        
#define pin_LED_RED     0        // Status LED R-G-B pins (rule in code below: LOW switches led color ON)  
#define pin_LED_GREEN   9       
#define pin_LED_BLUE    18  
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: without resistor, common GND, analogue)
#define pin_RECORD_BTN  38       // 1st recording control: push button 
#define pin_TOUCH       7        // 2nd recording control: touch button 
#define pin_VOL_POTI    3        // adjusting audio volume
#define pin_VOL_BTN     NO_PIN   // not needed (adjusting audio volume with continuous VOL_POTI) 


// --- global Objects ----------

Audio audio_play;                // AUDIO.H object for I2S stream

uint32_t gl_TOUCH_RELEASED;      // idle value (untouched), read once on Init, used internally as reference in loop()
String gl_voice_instruct;        // internally used for forced 'voice character' (via command "VOICE", erased on friend changes)
  

// -- REDUCE stack / increase HEAP ('magic line' might help on ESP32 without PSRAM (use ONLY IF url streaming/radio don't work !)
// Background: streaming audio via AUDIO.H without (!) PSRAM reaches the ESP32 heap limit, reducing 1K stack increases 4K heap
// Thanks for the idea to @Schreibfaul1 (AUDIO.H author), link: https://github.com/schreibfaul1/ESP32-audioI2S/issues/1039
// do NOT use this trick if PSRAM available (to keep full 8K STACK size, AUDIO.H streaming with PSRAM does NOT stress the heap)
/* SET_LOOP_TASK_STACK_SIZE(7 * 1024);    // reducing STACK (from default 8K) to 7K or 6K. Reduction of 1K gives 4kB more heap */

  
// --- last not least: declaration of functions in other modules (not mandatory but ensures compiler checks correctly)
// splitting Sketch into multiple tabs see e.g. here: https://www.youtube.com/watch?v=HtYlQXt14zU

bool   I2S_Recording_Init();   
bool   Recording_Loop();       
bool   Recording_Stop( String* filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec ); 

String SpeechToText_Deepgram(   String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key );
String SpeechToText_ElevenLabs( String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key );

String OpenAI_Groq_LLM( String UserRequest, const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key );
void   get_tts_param( int* id, String* names, String* model, String* voice, String* vspeed, String* inst, String* hello );




// ******************************************************************************************************************************

void setup() 
{   
  // Initialize serial communication
  Serial.begin(115200); 
  Serial.setTimeout(100);    // 10 times faster reaction after CR entered (default is 1000ms)

  // OUTPUT pin assignments / RGB led testing on Power ON (OUTPUT pins will be assigned below again due ESP32-S3 bug)
  pinMode(pin_LED_RED, OUTPUT);  pinMode(pin_LED_GREEN, OUTPUT);  pinMode(pin_LED_BLUE, OUTPUT);
  // on INIT: walk 1 sec thru 3 RGB colors (RED -> GREEN -> BLUE) .. then stay on YELLOW until WiFi connected
  led_RGB(LOW,LOW,LOW);   // init function led_RGB()  
  led_RGB(LOW,HIGH,HIGH); delay (330);  // ## RED 
  led_RGB(HIGH,LOW,HIGH); delay (330);  // ## GREEN
  led_RGB(HIGH,HIGH,LOW); delay (330);  // ## BLUE
  led_RGB(LOW,LOW,HIGH);                // ## YELLOW ...

  // Digital INPUT pin assignments (not needed for analogue pin_VOL_POTI & pin_TOUCH)
  // Detail: Some ESP32 pins do NOT support INPUT_PULLUP (e.g. pins 34-39), external resistor still needed
  if (pin_RECORD_BTN != NO_PIN) {pinMode(pin_RECORD_BTN,INPUT_PULLUP); }  
  if (pin_VOL_BTN    != NO_PIN) {pinMode(pin_VOL_BTN,   INPUT_PULLUP); }  
 
  // Calibration: measure initial TOUCH PIN idle value (untouched), used as reference to recognize TOUCH event
  gl_TOUCH_RELEASED = touchRead(pin_TOUCH);  
   
  // Hello World
  Serial.println( VERSION );  
  
  // I2S_Recording_Init() initializes KALO I2S Recording Services (don't forget!)
  // - function checks SD Card, allocates PSRAM buffer, init I2S assignments 
  // - in case of ERROR: print ERROR, then stop (staying there forever):
  // - also printing some hardware details (e.g PSRAM) in DEBUG mode
  
  I2S_Recording_Init();    
      
  // Connecting to WLAN
  WiFi.mode(WIFI_STA);                                 
  WiFi.begin(ssid, password);         
  Serial.print("> Connecting WLAN " );
  while (WiFi.status() != WL_CONNECTED)                 
  { Serial.print(".");  delay(500); 
  } 
  Serial.println(". Done, device connected.");
  led_RGB( HIGH,LOW,HIGH );   // LED ## GREEN: device connected
    
  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT );

  // INIT SD Card Reader [SPI.begin() & SD.begin()] for optional SD Card (## NEW / 2026-01 Update)
  bool flg_SD_found;   // check if SD Cards Reader with SD Card available (supporting custom pins)       
  if (SD_SCK != NO_PIN && SD_MISO != NO_PIN && SD_MOSI != NO_PIN && SD_CS != NO_PIN)    
  {  SPI.begin( SD_SCK, SD_MISO, SD_MOSI, SD_CS );       // using predefined SPI instance (VSPI bus): no 'var SPIClass' needed   
     flg_SD_found = SD.begin( SD_CS, SPI, 40000000 );    // using 40 MHz (more stable than 80)   
     /* Alternative: using 2nd SPI controller (HSPI bus) for SD card in case VSPI needed for e.g. display connctor): 
        .. SPIClass spiSD(HSPI) ... spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS) ... SD.begin(SD_CS, spiSD, 40000000)  
        Info link:  https://randomnerdtutorials.com/esp32-microsd-card-arduino/#sdcardcustompins 
        example global SPI: https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/I2Saudio_SD/I2Saudio_SD.cpp 
     */
  }  else { flg_SD_found = SD.begin(); }   // no custom pins -> initializing SD card with default VSPI or HSPI pins  
  
  // HELLO - Optional: Playing a optional WELCOME_FILE wav file on SD Card once (with reduced fixed volume)
  // feature can be used on any ESP32 with SD card (independent of Recording settings RECORD_PSRAM vs. RECORD_SDCARD)
  if ( flg_SD_found )   
  {  if ( SD.exists( WELCOME_FILE ) )
     {  audio_play.setVolume(8);    // supported values: 0-21. Playing Welcome file ALWAYS with reduced volume (e.g. 8)   
        audio_play.connecttoFS( SD, WELCOME_FILE );  
        // using 'isRunning()' trick to wait in setup() until PLAY is done (when done: proceed with next WELCOME_MSG below)
        while (audio_play.isRunning()) { audio_play.loop(); }
     }     
  } 

  // Next line looks strange: REPEATING output pinMode assigments ! / Reason: well known ESP32-S3 issue (S3 only):
  // .. certain ESP-S3 pins 'forget' OUTPUT assignments after I2S init, and frequently after launching 'SD.begin()'!
  pinMode(pin_LED_RED, OUTPUT);  pinMode(pin_LED_GREEN, OUTPUT);  pinMode(pin_LED_BLUE, OUTPUT);
  led_RGB( HIGH,HIGH,HIGH ); led_RGB( HIGH,LOW,HIGH );   // LED: init again, ## OFF -> back to ## GREEN

  // Initialize user defined Audio Volume (values 0-21), not relevant in case VOL_POTI used (bc. continous reading in loop())
  audio_play.setVolume( gl_VOL_INIT );  
  
  // HELLO - Speak the FRIEND (Agent) specific a FRIENDS[].welcome sentence (with FRIEND specific settings/voice)  
  TextToSpeech( "#WELCOME" );      // '#WELCOME' triggers an internal command in TextToSpeech()
  
  // INIT done, starting user interaction  
  Serial.println( "\nWorkflow:\n> Hold or Touch button during recording voice -OR- enter request in Serial Monitor" );  
  Serial.println( "> Select another AI FRIEND by calling his NAME, example: \"Hi FRED, are you online?\"" );  
  Serial.println( "> Key word [GOOGLE] inside request: toggle LLM to Open AI realtime web search model" );  
  Serial.println( "> Key word [VOICE]  inside request: use request as Open AI TTS 'voice instruction'" );
  Serial.println( "> Key word [RADIO | DAILY NEWS | TAGESSCHAU] inside request: start audio url streaming" );  
  Serial.println( "> Command [#] or speaking [HASHTAG]: print CHAT history & Friends list in Serial Monitor" );  
  Serial.println( "> Command [@] or Key word [EMAIL] inside request: send CHAT history to user via email" );  
  Serial.println( "> Command [DEBUG ON|OFF]: enable|disable workflow details in Serial Monitor\n" );  
  Serial.println( "========================================================================================\n"); 

}



// ******************************************************************************************************************************

void loop() 
{
  String UserRequest;                   // user request, initialized new each loop pass 
  String LLM_Feedback;                  // LLM AI response
  static String LLM_Feedback_before;    // static var to keep information from last request (as alternative to global var)
  static bool flg_UserStoppedAudio;     // both static vars are used for 'Press button' actions (stop Audio a/o repeat LLM)

  String   record_SDfile;               // 4 vars are used for receiving recording details               
  uint8_t* record_buffer;
  long     record_bytes;
  float    record_seconds;  
  
  
  // ------ Read USER INPUT via Serial Monitor (fill String UserRequest and ECHO after CR entered) ------------------------------
  // ESP32 as TEXT Chat device: this allows to use the Serial Monitor as an LLM AI text chat device  
  // HINT: hidden feature (covered in lib_openai_groq.ino): Keyword '#' in Serial Monitor prints the history of complete dialog
  
  while (Serial.available() > 0)                        // definition: returns numbers ob chars after CR done
  { // we end up here only after Input done             // this 'while loop' is a NOT blocking loop script :)
    UserRequest = Serial.readStringUntil('\n');                                                                     
   
    // Clean the input line first:
    UserRequest.replace("\r", "");  UserRequest.replace("\n", "");   UserRequest.trim();
    
    // then ECHO in monitor in [brackets] (in case the user entered more than spaces or CR only): 
    if (UserRequest != "")
    {  Serial.println( "\nYou> [" + UserRequest + "]" );      
    }  
  }  


  // ------ Check status of AUDIO RECORDING controls (supporting PUSH buttons and TOUCH buttons) --------------------------------
   
  bool flg_RECORD_BTN;               // both flags used to trigger recording AND for RGB led status at eof loop()
  bool flg_RECORD_TOUCH;
    
  // 1. PUSH BUTTON (if available) - check RECORD BUTTON status LOW & HIGH (result: flg_RECORD_BTN is LOW | HIGH) 
  
  if (pin_RECORD_BTN != NO_PIN)   
  {  flg_RECORD_BTN = digitalRead(pin_RECORD_BTN);
  }  else flg_RECORD_BTN = HIGH;  // no button available -> never pressed
  
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
    
  
  // ------ Read USER INPUT via Voice recording & launch Deepgram transcription -------------------------------------------------
  // ESP32 as VOICE chat device: Recording (as long pressing or touching RECORD) & Transcription on Release (result: UserRequest)
  // 3 different BTN actions:  PRESS & HOLD for recording || STOP (Interrupt) LLM AI speaking || REPEAT last LLM AI answer  

  if ( flg_RECORD_BTN == LOW || flg_RECORD_TOUCH )                // # Recording started, supporting btn and touch sensor
  {  delay(30);                                                   // unbouncing & suppressing finger button 'click' noise 
     if (audio_play.isRunning())                                  // Before we start any recording: always stop earlier Audio 
     {  audio_play.stopSong();                                    // [bug fix]: previous audio_play.connecttohost() won't work
        Serial.println( "\n< STOP AUDIO >" );
        flg_UserStoppedAudio = true;                              // to remember later that user stopped (distinguish to REPEAT)    
     }   
     // Now Starting Recording (no blocking, not waiting)
     Recording_Loop();                                            // that's the main task: Recording AUDIO (ongoing)  
  }

  if ( flg_RECORD_BTN == HIGH && !flg_RECORD_TOUCH )              // Recording not started yet OR stopped (on release button)
  {  
     // now we check if RECORDING is done, we receive recording details (length etc..) via &pointer
     // hint: Recording_Stop() is true ONCE when recording finalized and .wav is available

     if (Recording_Stop( &record_SDfile, &record_buffer, &record_bytes, &record_seconds )) 
     {  if (record_seconds > 0.4)                                 // using short btn TOUCH (<0.4 secs) for other actions
        {  led_RGB(HIGH,LOW,LOW);                                 // LED [Update]: ## CYAN indicating Deepgram STT starting 
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
        else                                                      // 2 additional Actions on short button PRESS (< 0.4 secs):
        { if (!flg_UserStoppedAudio)                              // - STOP AUDIO when playing (done above, if Btn == LOW)
          {  Serial.println( "< REPEAT TTS >" );                  // - REPEAT last LLM answer (if audio currently not playing)
             LLM_Feedback = LLM_Feedback_before;                  // hint: REPEAT is also helpful in the rare cases when Open AI
          }                                                       // TTS 'missed' speaking: just tip btn again for triggering    
          else 
          {  // Trick: allow <REPEAT TTS> on next BTN release (LOW->HIGH) after next short recording
             flg_UserStoppedAudio = false;                        
          }
        }
     }      
  }  
  

  // ------ USER REQUEST found -> Checking KEYWORDS first -----------------------------------------------------------------------
  
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
     UserRequest = "";  // do NOT start LLM
  } 

  // 2. keyword 'DAILY NEWS' or German 'TAGESSCHAU' inside request-> Playing German TV News: Tagesschau24
  // Use case example (Recording request): "Please stream daily news for me!" -> Streaming launched
  
  if (cmd.indexOf("DAILY NEWS") >=0 || cmd.indexOf("TAGESSCHAU") >=0 ) 
  {  Serial.println( "< Streaming German Daily News TV: Tagesschau24 >" );   
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD       
     led_RGB(LOW,LOW,HIGH);                                       // LED: ## YELLOW indicating 'Stream' pending -> .. MAGENTA
     // HINT !: the streaming can fail on some ESP32 without PSRAM (AUDIO.H issue!), in this case: deactivate/remove next line:
     audio_play.connecttohost( "https://icecast.tagesschau.de/ndr/tagesschau24/live/mp3/128/stream.mp3"  ); 
     UserRequest = "";  // do NOT start LLM
  }

  // 3. keyword 'VOICE' (or German 'STIMME') anywhere in sentence -> Use (and remember) USER statement as 'Voice Instruction'
  // Background: Open TTS API supports 'voice instruction' (kind of 'voice system prompt) for customizing voice 'character'
  // We utilize this capability via keyword 'VOICE' -> sending request to LLM + Instruction (latest AUDIO.H + PSRAM needed!)
  // Use case example (Recording request): "Can you speak in silent voice please or whispering ?" -> Test it once ;)
  
  if (cmd == "VOICE" || cmd == "STIMME")                          // single word 'VOICE' only: Reset voice instruction prompt
  {  Serial.println( "< Voice instruction removed >" );           // (global var, will also be erased in TTS if friend changed)
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD      
     gl_voice_instruct = ""; cmd = "";
     UserRequest = "";  // do NOT start LLM
  }
  if (cmd.indexOf("VOICE") >=0 || cmd.indexOf("STIMME") >=0 )     // request contains word 'VOICE' -> force voice instruction
  {  Serial.println( "< Voice instruction stored > [" + UserRequest + "]");   
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD  
     gl_voice_instruct = UserRequest;                             // global var, will be erased in TTS (in case friend changed)    
     // NOT erasing UserRequest -> starting LLM too
  }

  // 4. Toggling DEBUG mode ON|OFF during runtime (ON: enable progress details in Serial Monitor, OFF: minimize Serial I/O) 
  if (cmd.indexOf("DEBUG ON") >=0 )                               
  {  Serial.println( "< DEBUG ON >");                             // toggling DEBUG mode 'ON' via command (keyboard or STT)    
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD  
     DEBUG = true;
     UserRequest = "";  // do NOT start LLM
  }
  if (cmd.indexOf("DEBUG OFF") >=0 )                              
  {  Serial.println( "< DEBUG OFF >");                            // toggling DEBUG 'OFF'  
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD  
     DEBUG = false;
     UserRequest = "";  // do NOT start LLM
  }  

  // 5. key COMMAND '#' or speaking SINGLE word 'HASHTAG'         -> Serial Monitor Print of complete CHAT history 
  // logic is done in lib_openai.ino (in OpenAI_Groq_LLM()), here we trigger LED only prior launching LLM
  if (cmd == "#" || cmd == "HASHTAG")                             // this trick allows to trigger via voice (just say 'Hashtag')
  {  led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD  
     UserRequest = "#";                                           // rest of logic is done inside 'lib_openai..ino'
  }  

  // 6. key COMMAND '@' or keyword 'EMAIL' INSIDE request sentence -> sending complete CHAT history to user email account
  // logic is done in lib_openai.ino (in OpenAI_Groq_LLM()), here we trigger LED only prior launching LLM
  cmd.replace("E-MAIL","EMAIL"); cmd.replace("E MAIL", "EMAIL");  // cleaning STT variations
  if (cmd == "@" || cmd.indexOf("EMAIL") >=0 )                    // enter @ or speak sentence with 'EMAIL' included
  {  Serial.println( "< Email request, sending complete chat history to smtp server >");
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD  
     UserRequest = "@";                                           // rest of logic is done inside 'lib_openai..ino'
  }

  // 7. keyword 'GOOGLE' -> launching Open AI WEB SEARCH feature 
  // ... handled in LLM call below

   
  // ------ USER REQUEST found -> Call OpenAI_Groq_LLM() ------------------------------------------------------------------------
  // Utilizing new 'real-time' web search feature in case user request contains the keyword 'GOOGLE' 
  // [using same 'OpenAI_Groq_LLM(..)' function with additional parameter (function switches to dedicated LLM search model]
  // Recap: OpenAI_Groq_LLM() remembers complete history (appending prompts) to support ongoing dialogs (web searches included;)
  
  if (UserRequest != "" ) 
  { 
    // [bugfix/new]: ensure that all TTS websockets are closed prior open LLM websockets (otherwise LLM connection fails)
    audio_play.stopSong();    // stop potential audio (closing AUDIO.H TTS sockets to free up the HEAP) 
    
    // CASE 1: launch Open AI WEB SEARCH feature if user request includes the keyword 'Google'
    // supporting User requests like 'Will it rain tomorrow in my region?, please ask Google'
        
    if ( UserRequest.indexOf("Google") >= 0 || UserRequest.indexOf("GOOGLE") >= 0 )                          
    {  
       led_RGB(LOW,HIGH,LOW);                                     // LED: ## MAGENTA indicating Open AI WEB SEARCH Request 
       Serial.print( "LLM AI WEB SEARCH> " );                     // function OpenAI_Groq_LLM() will Serial.print '...'
       
       // 2 workarounds are recommended to utilize the new Open AI Web Search for TTS (speaking the response).
       // Background: New Open AI Web Search models are not intended for TTS (much too detailed, including lists & links etc.)
       // and they are also 'less' prompt sensitive, means ignoring earlier instructions (e.g. 'shorten please!') quite often 
       // so we use 2 tricks: 1. adding a 'default' instruction each time (forcing 'short answers') + 2. removing remaining links
       // KEEP in mind: SEARCH models are slower (response delayed) than CHAT models, so we use on (GOOGLE) demand only !
       
       // 1. forcing a short answer via appending prompt postfix - hard coded GOAL:
       String Postfix = ". Summarize in few sentences, do NOT use any enumerations, line breaks or web links!"; 
       String Prompt_Enhanced = UserRequest + Postfix;   
                                                                            
       // Action happens here! (WAITING until Open AI web search is done)        
       LLM_Feedback = OpenAI_Groq_LLM( Prompt_Enhanced, OPENAI_KEY, true, GROQ_KEY );   // 'true' means: launch WEB SEARCH model

       // 2. even in case WEB_SEARCH_ADDON contains a instruction 'no links please!: there are still rare situation that 
       // links are included (eof search results). So we cut them manually  (prior sending to TTS / Audio speaking)
       
       int any_links = LLM_Feedback.indexOf( "([" );                    // Trick 2: searching for potential links at the end
       if ( any_links > 0 )                                             // (they typically start with '([..'  
       {  Serial.println( "\n>>> RAW: [" + LLM_Feedback + "]" );        // Serial Monitor: printing both, TTS: uses cutted only  
          LLM_Feedback = LLM_Feedback.substring(0, any_links) + "|";    // ('|' just as 'cut' indicator for Serial Monitor)
          Serial.print( ">>> CUTTED for TTS:" );    
       } 
    }
    
    else  // CASE 2 [DEFAULT]: LLM chat completion model (for human like conversations)  
    
    {  led_RGB(HIGH,HIGH,LOW);                                    // LED: ## BLUE indicating LLM AI CHAT Request starting 
       Serial.print( "LLM AI CHAT> " );                           // function OpenAI_Groq_LLM() will Serial.print '...'
       
       // Action happens here! (WAITING until Open AI or Groq done)
       LLM_Feedback = OpenAI_Groq_LLM( UserRequest, OPENAI_KEY, false, GROQ_KEY );    // 'false' means: default CHAT model                                 
    }
          
    // final tasks (always):
    
    if (LLM_Feedback != "")                                       // in case we got any valid feedback ..  
    { led_RGB(LOW,LOW,LOW); delay(200);                           // -> LED: ## WHITE FLASH (200ms)    
      led_RGB(HIGH,HIGH,HIGH); delay(100);  
            
      int id;  String names, model, voice, vspeed, instruction, welcome;     // to get name of current LLM agent (friend)                  
      get_tts_param( &id, &names, &model, &voice, &vspeed, &instruction, &welcome );      
      Serial.println( " [" + names + "]" + " [" + LLM_Feedback + "]" );  
          
      LLM_Feedback_before = LLM_Feedback;                           
    }           
    else Serial.print("\n");   
  }


  // ------ Speak LLM answer (using Open AI voices by default, all voice settings done in TextToSpeech() ------------------------
 
  if (LLM_Feedback != "") 
  {  led_RGB(LOW,LOW,HIGH);                                       // LED: ## YELLOW indicating 'TTS' audio pending 

     // simple TTS call: TextToSpeech() manages all voice parameter for current active AI agent (FRIEND[x])
     TextToSpeech( LLM_Feedback );         
  }

    
  // Adjusting AUDIO Volume (either via Analogue POTI or via toggle button) -----------------------------------------------------
  
  if (pin_VOL_POTI != NO_PIN)     // --- KALO default: if available then using an POTI to adjust Audio Volume continuously 
  {  static long millis_before = millis();  
     static int  volume_before;
     // reading POTI, rarely only (e.g. 4 times/sec and only if diff >= 2) to avoid unnecessary audio flickering
     if (millis() > (millis_before + 250))  // each 250ms 
     { millis_before = millis(); 
       int volume = map( analogRead(pin_VOL_POTI), 0, 4095, 0, 21 );
       if ( abs(volume-volume_before) >= 1  )    
       {  volume_before = volume;
          Serial.println( "New Audio Volume: [" + (String) volume + "]" );   
          audio_play.setVolume(volume);  // AUDIO: values from 0 to 21          
       }    
     } 
  }
    
  if (pin_VOL_BTN != NO_PIN)      // --- Alternative: using a VOL_BTN to toggle thru N values (e.g. TECHISMS or Elato AI pcb)
  {  static bool flg_volume_updated = false; 
     static int volume_level = -1; 
     if (digitalRead(pin_VOL_BTN) == LOW && !flg_volume_updated)          
     {  int steps = sizeof(gl_VOL_STEPS) / sizeof(gl_VOL_STEPS[0]);  // typically: 3 steps (any arrays supported)
        volume_level = ((volume_level+1) % steps);  // walking in circle (starting with 0): e.g. 0 -> 1 -> 2 -> 0 ..
        Serial.println( "New Audio Volume: [" + (String) volume_level + "] = " + (String) gl_VOL_STEPS[volume_level] );
        // visualize to user with ## WHITE FLASHES (with OFF between flashes)
        for (int i=0; i<=volume_level; i++)  
        {  led_RGB(LOW,LOW,LOW); delay(80); led_RGB(HIGH,HIGH,HIGH); delay(120);  // ## WHITE flash 80ms, ## OFF 120ms                          
        }   
        flg_volume_updated = true;
        audio_play.setVolume( gl_VOL_STEPS[volume_level] );  
    }
    if (digitalRead(pin_VOL_BTN) == HIGH && flg_volume_updated)         
    {  flg_volume_updated = false;    
    }
  }
  
  
  // ----------------------------------------------------------------------------------------------------------------------------
  // Play AUDIO (Schreibfaul1 loop for Play Audio (details here: https://github.com/schreibfaul1/ESP32-audioI2S))
  // and updating LED status

  audio_play.loop();  
  vTaskDelay(1); 

  // update LED status always (in addition to WHITE flashes + YELLOW on STT + BLUE on LLM + CYAN on TTS)
  if (flg_RECORD_BTN==LOW || flg_RECORD_TOUCH) { led_RGB(LOW,HIGH,HIGH); }  // led RED as long the record ongoing
  else if (audio_play.isRunning())             { led_RGB(LOW,HIGH,LOW);  }  // led MAGENTA when Open AI voice is speaking 
  else                                         { led_RGB(HIGH,LOW,HIGH); }  // led GREEN (default) when ready for next request 
    
}

// end of LOOP() ****************************************************************************************************************




// ------------------------------------------------------------------------------------------------------------------------------
// Updating LED with RGB on/off values  
// ------------------------------------------------------------------------------------------------------------------------------

void led_RGB( bool red, bool green, bool blue ) 
{ // general usage: using LOW in code switches the LED color on, e.g. led_RGB(LOW,HIGH,HIGH) means RED on   
  // using static vars, reason: writing to real pin only if changed (increasing performance for frequently repeated calls)
  
  static bool red_before=HIGH, green_before=HIGH, blue_before=HIGH;  // memo: HIGH is same as true(1), LOW is false(0)

  if (flg_LED_DIGITAL)   // Default [KALO pcb or Elato AI]: using complete Vcc (soldered serial resistors exist), common Anode 
  {  if (red   != red_before)   { digitalWrite(pin_LED_RED,red);     red_before=red;     }     
     if (green != green_before) { digitalWrite(pin_LED_GREEN,green); green_before=green; }      
     if (blue  != blue_before)  { digitalWrite(pin_LED_BLUE,blue);   blue_before=blue;   }      
  }   
  if (!flg_LED_DIGITAL)  // Exception [e.g. TECHIESMS pcb]: LED with common GND, missed resistors (so we must reduce voltage)
  {  // writing analog values 0 or max. ~40 (not 255!, means no digitalWrite!) 
     if (red   != red_before)   { analogWrite(pin_LED_RED,  (red==LOW)?   40:0 );  red_before=red;     }      
     if (green != green_before) { analogWrite(pin_LED_GREEN,(green==LOW)? 40:0 );  green_before=green; }      
     if (blue  != blue_before)  { analogWrite(pin_LED_BLUE, (blue==LOW)?  40:0 );  blue_before=blue;   }   
  }   
}



// ------------------------------------------------------------------------------------------------------------------------------
// audio_info(const char *info) is an inbuilt (optional) Callback function in AUDIO.H, printing details of played Audio events
// ------------------------------------------------------------------------------------------------------------------------------

void audio_info(const char *info)
{  // printing AUDIO.H details in DEBUG mode (except rarely warning "webfile chunked: not enough bytes available for skipCRLF". 
   // Details: https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/issues/4
   // uncomment this line to receive AUDIO.H details in DEBUG mode):   
   String info_str = (String) info;  
   if ( info_str.indexOf("skipCRLF") == -1 ) { DebugPrintln( "AUDIO.H info: " + info_str ); }    
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
   
   if (id != id_before) { gl_voice_instruct = ""; id_before = id; } // reset forced voice instruction if FRIEND changed
   if (gl_voice_instruct != "" ) {instruction = gl_voice_instruct;} // user forced 'voice command' overrides FRIENDS[].instuct
   if (p_request == "#WELCOME") {p_request = welcome;}              // intrinsic # command: 'speak' FRIENDS[].welcome entry  
   
   Serial.print( "TTS Audio [" + names + "|" + voice + "|" + model + "]" );  // e.g. TTS AUDIO [SUSAN | nova/gpt-4o-mini-tts]  
   if ( gl_voice_instruct != "" ) { Serial.print( "\nTTS Voice Instruction [" + gl_voice_instruct + "]" ); }
   Serial.println();  

   if (model == "SPEECHGEN")     // Special Case 1: optionally using SpeechGen.io TTS (instead OpenAI) on dedicated voices
   {  // just use my function 'voice_SpeechGen(..)' from 'lib_TTS_SpeechGen.ino' in 'kaloprojects/KALO-ESP32-Voice-Assistant'
      // I am using it for a dedicated 'CHILD' voice (German 'Gisela' or English 'Anny' or e.g. "Kasper", "Hannah plus" etc)
      // uncomment the 2 lines below in case you want use SpeechGen voices (lib_TTS_SpeechGen.ino needed): 
      
      /* String mp3_url = voice_SpeechGen( p_request, voice, "1", vspeed, "good" );  // param: request/voice/pitch,speed,emotion
      audio_play.connecttohost( mp3_url.c_str() );  */
       
      return;   // DONE.
   }  

   if (model == "GOOGLE_TTS")    // Special Case 2: using free Google TTS (just for demo purposes, I am not using, too limited)
   {  // in case you prefer free GOOGLE TTS (free of usage, but less 'human' & LIMITED in length / long sentences won't work !!)
      // - Google TTS are mono lingual only, language tag needed, e.g. en, en-US, en-IN, en-BG, en-AU, de-DE, th-TH etc.
      // - https://cloud.google.com/text-to-speech/docs/voices 
      // - using this TTS on friend X: initalize 'Agents FRIENDSFriends[X].tts_model' with "GOOGLE_TTS" (in lib_OpenAI_Chat.ino)
      audio_play.connecttospeech( p_request.c_str(), "en");   
      return;   // DONE.   
   }

   // using Open AI TTS voice by DEFAULT:
   /* - AUDIO.H dependecies (IMPORTANT): The amount of .openai_speech() parameters changed in AUDIO.H library versions 
   // - Background: since AUDIO.H ver. '3.1.0u' a 4th parameter 'voice instruction' is added (usage e.g.: "you are whispering")
   // - choose the correct amout of parameter to avoid COMPILATION ERRORS (and set the wrong one into comment) .. */
   // ==> So CHECK your AUDIO.H library & update this last line in code if needed ! .... 

   audio_play.openai_speech( OPENAI_KEY, model, p_request, instruction, voice, "aac", vspeed);  // <- use if version >= 3.1.0u 
/* audio_play.openai_speech( OPENAI_KEY, model, p_request,              voice, "aac", vspeed);  // <- use this for 3.0.11g ! */
 
}

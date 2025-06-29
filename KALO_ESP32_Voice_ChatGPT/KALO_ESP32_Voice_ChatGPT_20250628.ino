
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                               KALO_ESP32_Voice_ChatGPT                                   ------------------
// ------------------                     ESP32 voice dialog device for Open AI (STT & TTS)                    ------------------
// ------------------         [based on KALO C++ libraries from github 'KALO-ESP32-Voice-Assistant']           ------------------
// ------------------                                                                                          ------------------
// ------------------                            Latest Update: June 28, 2025                                  ------------------
// ------------------                                   Coded by KALO                                          ------------------
// ------------------                                                                                          ------------------
// ------------------                                    > Workflow <                                          ------------------
// ------------------             Entering User request via keyboard (Serial Monitor) OR Voice                 ------------------
// ------------------         Voice RECORDING (on holding BTN) with variable length [KALO I2S code]            ------------------
// ------------------            STT SpeechToText [using Elevenlabs - or - Deepgram API service]               ------------------
// ------------------                Call OpenAI LLM CHAT model, remembering dialog history                    ------------------
// ------------------         KEYWORDS: GOOGLE (web search), VOICE (TTS character), RADIO, DAILY NEWS          ------------------
// ------------------                         TTS TextToSpeech with OpenAI voices                              ------------------
// ------------------             Short BTN Touch: STOP Audio or REPEAT last OpenAI response                   ------------------
// ------------------                                                                                          ------------------
// ------------------                        > Hardware Requirements / Circuit <                               ------------------
// ------------------           ESP32 with PSRAM - or - Micro SD Card (VSPI Default pins 5,18,19,23)           ------------------
// ------------------                   I2S Amplifier (e.g. MAX98357), pins see below                          ------------------
// ------------------                                                                                          ------------------
// ------------------            > Ready to Go Hardware (PCB / Printed Circuit Boards) - Examples <            ------------------
// ------------------     TechieSMS Assistant: https://techiesms.com/product/portable-ai-voice-assistant/      ------------------
// ------------------     Elato AI: DIY https://github.com/akdeb/ElatoAI | home: https://www.elatoai.com/      ------------------
// ------------------------------------------------------------------------------------------------------------------------------


// *** Install hints: 
// 1. in case of an 'Sketch too Large' Compiler Warning/ERROR in Arduino IDE (ESP32 Dev Module):
//    -> select a larger 'Partition Scheme' via menu > tools: e.g. using 'No OTA (2MB APP / 2MB SPIFFS) ***
// 2. TECHIESMS pcb: In case PC does not detect ESP32 Serial Port, then install CH340 driver (missed on older Windows versions)
//    -> driver links here: https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all#windows-710
// 3. Library dependencies [## NEW ##]: 
//    - ESP32 with PSRAM:    use latest arduino-esp32 core (3.1 / ESP-IDF 5.3 or later) + latest AUDIO.H (3.2.0 or later)
//    - ESP21 without PSRAM: use latest arduino-esp32 core (3.1 / ESP-IDF 5.3 or later) + AUDIO.H (3.0.11g) ! 
//      Mirror to older AUDIO.H (3.0.11g): https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT/tree/main/libray_archive


#define  VERSION                 "\n== KALO ESP32 Voice ChatGPT (last update: June 28, 2025) =================================\n"   


// --- includes ----------------

#include <WiFi.h>                // only included here
#include <SD.h>                  // also needed in other tabs (.ino) 
#include <WiFiClientSecure.h>    // only needed in other tabs (.ino)

#include <Audio.h>               // @Schreibfaul1 library, used for PLAYING Audio (via I2S Amplifier) -> mandatory for TTS 
                                 // [not needed for: Audio Recording - Audio STT Transcription - Open AI LLM Chat]
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
                               

// --- defines & macros -------- // DEBUG Toggle enables/disables printing progress in Serial Monitor)

#ifndef DEBUG                    // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG true             // <- define your preference here [true activates printing INFO details]  <- test both !
#  define DebugPrint(x);         if(DEBUG){Serial.print(x);}   // do not touch
#  define DebugPrintln(x);       if(DEBUG){Serial.println(x);} // do not touch 
#endif 

// === PRIVATE credentials =====

const char* ssid =               "...";     // ### INSERT your wlan ssid 
const char* password =           "...";     // ### INSERT your password 

const char* OPENAI_KEY =         "...";     // ### INSERT your OpenAI key

const char* ELEVENLABS_KEY =     "...";     // ### INSERT your ElevenLabs API key
const char* DEEPGRAM_KEY =       "...";     // ### STT ALTERNATIVE: Deepgram API key

 
// === user settings ========== 

#define FAVORITE_VOICE_TTS       "onyx"  // forcing voice (Demo video: "onyx"), keep EMPTY "" if you want to test all by random
                                         // OpenAI tts-1 Voices (June 2025): alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer

#define WELCOME_MSG              "Hi my friend, welcome back. How can i help you ?"      // Played on Power on (optional) 
/*#define WELCOME_MSG            "Hallo mein Freund, schön das du wieder da bist. Wie kann ich dir heute helfen ?"  // German */

#define WEB_SEARCH_ADDON         ". Summarize in few sentences, do NOT use any enumerations, line breaks or web links!"
#define WEB_SEARCH_USER_CITY     "Berlin"     // optional (recommended): User location optimizes OpenAI 'web search' response 

#define WELCOME_FILE             "/Welcome.wav"  // optional: 'Hello' file on SD will be played once on start (e.g. a gong)

int gl_VOL_INIT =       21;      // Initial Audio volume on Power On / possible values: 0-21 (e.g. start with max. Volume 21) 
int gl_VOL_STEPS[]= {11,16,21};  // used for VOL_BTN only: user defined audio volume steps (X steps possible, 3 just as example),
                                 // 1st click: STEPS[0]. High values best for TTS, lower volumes for e.g. Radio streaming   


// === HARDWARE seetings ======= // SELECT (uncomment) 1 of the 3 pre-defined PCB (Printed Circuit Bord) or use own seetings ... 
                                 // also do NOT (!) forget to update 'lib_audio_recording.ino' (PSRAM, SDCARD, I2S Audio IN pins)

/* --- PCB: KALO device -------- // ESP32 WROOM (N4R0, no PSRAM) & SD card -or- ESP32-WROVER (N8R8, 4MB PSRAM limited, w/wo SD) 
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: [TECHIESMS]: w/wo Resistor + common GND, Analog)
#define pin_LED_RED     15       // R-G-B pins (Examples: KALO PCB 15-21-4,  KALO Proto 15-2-0, TECHIESMS 15-2-4) 
#define pin_LED_GREEN   21       // .. rule in code below: LOW switches led color ON 
#define pin_LED_BLUE    4  
#define pin_I2S_DOUT    25       // I2S_DOUT + I2S_LRC + I2S_BCLK are the I2S Audio OUT pins (e.g. for I2S Amplifier MAX98357) 
#define pin_I2S_LRC     26       // (Hint 1: I2S INPUT pins via Microphone INMP441: see header in 'lib_audio_recording.ino')
#define pin_I2S_BCLK    27       // (Hint 2: SD card pins NOT defined at all (always using VSPI Default pins 5,18,19,23) 
#define pin_RECORD_BTN  36       // RECORD tactile button on PCB (Printed Circuit Board), PULL-UP soldered
#define pin_TOUCH       13       // RECORD TOUCH control on TOP panel
#define NO_PIN          -1       // 'not connected' value for optional controls: RECORD_BTN | TOUCH | VOL_POTI | VOL_BTN 
#define pin_VOL_POTI    34       // Analogue wheel POTI as continuous audio volume control
#define pin_VOL_BTN     NO_PIN   // no button for audio volume needed, using continuous POTI for audio volume control */

// --- PCB: TECHIESMS device --- // ESP32 WROOM (N4R0, no PSRAM) & SD: https://techiesms.com/product/portable-ai-voice-assistant/
#define flg_LED_DIGITAL false    // (!) FALSE: [TECHIESMS]: no Resistor + common GND (Analog) (TRUE: Serial Resistor, common Vcc)
#define pin_LED_RED     15       // R-G-B pins (Examples: KALO PCB 15-21-4,  KALO Proto 15-2-0, TECHIESMS 15-2-4) 
#define pin_LED_GREEN   2        // .. rule in code below: LOW switches led color ON 
#define pin_LED_BLUE    4  
#define pin_I2S_DOUT    25       // I2S_DOUT + I2S_LRC + I2S_BCLK are the I2S Audio OUT pins (e.g. for I2S Amplifier MAX98357) 
#define pin_I2S_LRC     26       // (Hint 1: I2S INPUT pins via Microphone INMP441: see header in 'lib_audio_recording.ino')
#define pin_I2S_BCLK    27       // (Hint 2: SD card pins NOT defined at all (always using VSPI Default pins 5,18,19,23) 
#define pin_RECORD_BTN  36       // RECORD tactile button on PCB (Printed Circuit Board), PULL-UP soldered
#define pin_TOUCH       12       // # KALO mod # - additional RECORD TOUCH control on TOP panel (soldered to pin 12)
#define NO_PIN          -1       // 'not connected' value for optional controls: RECORD_BTN | TOUCH | VOL_POTI | VOL_BTN 
#define pin_VOL_POTI    NO_PIN   // no analogue wheel POTI available, using VOL_BTN for audio volume control  
#define pin_VOL_BTN     13       // # KALO mod # - using side button for audio volume (orig. REPEAT button no longer needed) 

/* --- PCB: ElatoAI ------------ // ESP32-S3 Dev (N16R8, 8MB PSRAM): https://github.com/akdeb/ElatoAI / https://www.elatoai.com/  
#define flg_LED_DIGITAL true     // TRUE: Serial Resistor, common Vcc (FALSE: [TECHIESMS]: w/wo Resistor + common GND, Analog)
#define pin_LED_RED     9        // R-G-B pins (Examples: KALO PCB 15-21-4,  KALO Proto 15-2-0, TECHIESMS 15-2-4)  
#define pin_LED_GREEN   8        // .. rule in code below: LOW switches led color ON 
#define pin_LED_BLUE    13  
#define pin_I2S_DOUT    7        // I2S_DOUT + I2S_LRC + I2S_BCLK are the I2S Audio OUT pins (e.g. for I2S Amplifier MAX98357) 
#define pin_I2S_LRC     5        // (Hint 1: I2S INPUT pins via Microphone INMP441: see header in 'lib_audio_recording.ino')
#define pin_I2S_BCLK    6        // (Hint 2: SD card pins NOT defined at all (always using VSPI Default pins 5,18,19,23)  
#define pin_TOUCH       3        // # KALO mod # - self soldered RECORD TOUCH (free: ESP32 12,14,32 / ESP32-S3: 3,12,14) 
#define NO_PIN          -1       // 'not connected' value for optional controls: RECORD_BTN | TOUCH | VOL_POTI | VOL_BTN 
#define pin_VOL_POTI    NO_PIN   // no analogue wheel POTI available, using VOL_BTN for audio volume control 
#define pin_VOL_BTN     2        // # KALO mod # - using original RECORD side button used as audio volume control 
#define pin_RECORD_BTN  NO_PIN   // # KALO mod # - no 2nd button available (original RECORD side btn on pin 2 used as VOL_BTN) */


// --- global Objects ----------

Audio audio_play;                // AUDIO.H object for I2S stream

uint32_t gl_TOUCH_RELEASED;      // idle value (untouched), read once on Init, used internally as reference in loop()


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

String Open_AI( String UserRequest, const char* LLM_API_key, bool flg_WebSearch, String City );  




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
  led_RGB(HIGH,HIGH,LOW); delay (330);  // ## RED
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

  // HELLO - Optional 1st: Say 'Hello' - Playing a optional WELCOME_FILE wav file on SD Card once (in reduced fixed volume)
  // this feature can be used on any ESP32 with or without PSRAM (SD.begin becomes true only if a SD Card & reader available)
  if (SD.begin())   // check if SD card reader exists: proceed further only if SD card reader installed
  {  if ( SD.exists( WELCOME_FILE ) )
     {  audio_play.setVolume(8);    // supported values: 0-21. We play file welcome file ALWAYS with reduced volume (e.g. 8)   
        audio_play.connecttoFS( SD, WELCOME_FILE );  
        // using 'isRunning()' trick to wait in setup() until PLAY is done (when done: proceed with next WELCOME_MSG below)
        while (audio_play.isRunning()) { audio_play.loop(); }
     }     
  } 

  // Next line looks strange: REPEATING output pinMode assigments ! / Reason: well known ESP32-S3 issue (S3 only):
  // .. certain ESP-S3 pins 'forget' OUTPUT assignments after I2S init, and frequently after launching 'SD.begin()'!
  pinMode(pin_LED_RED, OUTPUT);  pinMode(pin_LED_GREEN, OUTPUT);  pinMode(pin_LED_BLUE, OUTPUT);
  led_RGB( HIGH,HIGH,HIGH ); led_RGB( HIGH,LOW,HIGH );   // LED: init again, ## OFF -> back to ## GREEN

  // Initialize user defined Audio Volume (values 0-21)  // Init volume (user can change in loop() with VOL_POTI or VOL_BTN)
  audio_play.setVolume( gl_VOL_INIT );     
  
  // HELLO - Optional 2nd: Speak a WELCOME TEXT on Init (only if a favorite voice defined)
  if ( WELCOME_MSG != "" && FAVORITE_VOICE_TTS != "")
  {  TextToSpeech( OPENAI_KEY, "tts-1", WELCOME_MSG, "", FAVORITE_VOICE_TTS, "aac", "1" );
     // no waiting, direct jump into main loop (using 'audio_play.loop()', prelisten with user adjusted audio volume)  
  }
  
  // INIT done, starting user interaction  
  Serial.println( "\nWorkflow:\n> Hold or Touch button during recording voice .. OR enter request in Serial Monitor" );  
  Serial.println( "> [default request: chatting with Open AI 'gpt-4.1-nano' model, appending Chat history]" );  
  Serial.println( "> [key word GOOGLE: toggles to Open AI web search model]" );  
  Serial.println( "> [key word VOICE:  spoken user request triggers Open AI TTS voice instruction (character)]" );
  Serial.println( "> [key word RADIO | DAILY NEWS | TAGESSCHAU: starts audio url streaming]" );  
  Serial.println( "> [key command # (keyboard): printing complete CHAT history in Serial Monitor]\n" );  
  Serial.println( "==========================================================================================\n"); 

}



// ******************************************************************************************************************************

void loop() 
{
  String UserRequest;                   // user request, initialized new each loop pass 
  String LLM_Feedback;                  // Open AI response
  static String voice_instruct = "";    // NEW: used for 'voice character' (static keeps character for ongoing chat dialog)
  
  static String LLM_Feedback_before;    // static var to keep information from last request (as alternative to global var)
  static bool flg_UserStoppedAudio;     // both static vars are used for 'Press button' actions (stop Audio a/o repeat LLM)

  String   record_SDfile;               // 4 vars are used for receiving recording details               
  uint8_t* record_buffer;
  long     record_bytes;
  float    record_seconds;  
  
  
  // ------ Read USER INPUT via Serial Monitor (fill String UserRequest and ECHO after CR entered) ------------------------------
  // ESP32 as TEXT Chat device: this allows to use the Serial Monitor as an Open AI text chat device  
  // HINT: hidden feature (covered in lib_OpenAI_Chat.ino): Keyword '#' in Serial Monitor prints the history of complete dialog !
  
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


  // ------ Check status of RECORDING controls (supporting PUSH buttons and TOUCH buttons) --------------------------------------
   
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
     else  // ESP32-S3: idle values (UN-touched) ~ 22.000-28.000, TOUCHED: up to 35.000-120.000  // ESP32-S3: rising 20% or more
     {  flg_RECORD_TOUCH = (current_touch >  (uint32_t) (gl_TOUCH_RELEASED * 1.2)) ? true : false;          
     }
  }  else flg_RECORD_TOUCH = false;  // no touch button available -> never touched
    
  
  // ------ Read USER INPUT via Voice recording & launch Deepgram transcription -------------------------------------------------
  // ESP32 as VOICE chat device: Recording (as long presing or touching RECORD) & Transcription on Release (result 'UserRequest')
  // 3 different BTN actions:  PRESS & HOLD for recording || STOP (Interrupt) Open AI speaking || REPEAT last Open AI answer  

  if ( flg_RECORD_BTN == LOW || flg_RECORD_TOUCH )                // # Recording started, supporting btn and touch sensor
  {  delay(30);                                                   // unbouncing & suppressing finger button 'click' noise 
     if (audio_play.isRunning())                                  // Before we start any recording: always stop earlier Audio 
     {  audio_play.connecttohost("");                             // 'audio_play.stopSong()' won't work (collides with STT)
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
     audio_play.connecttohost( "https://liveradio.swr.de/sw282p3/swr3/play.mp3" ); 
     UserRequest = "";  // do NOT start LLM
  } 

  // 2. keyword 'DAILY NEWS' or German 'TAGESSCHAU' inside request-> Playing German TV News: Tagesschau24
  // Use case example (Recording request): "Please stream daily news for me!" -> Streaming launched
  
  if (cmd.indexOf("DAILY NEWS") >=0 || cmd.indexOf("TAGESSCHAU") >=0 ) 
  {  Serial.println( "< Streaming German Daily News TV: Tagesschau24 >" );   
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD       
     led_RGB(LOW,LOW,HIGH);                                       // LED: ## YELLOW indicating 'Stream' pending -> .. MAGENTA
     audio_play.connecttohost( "https://icecast.tagesschau.de/ndr/tagesschau24/live/mp3/128/stream.mp3"  ); 
     UserRequest = "";  // do NOT start LLM
  }

  // 3. keyword 'VOICE' (or German 'STIMME') anywhere in sentence -> Use (and remember) USER statement as 'Voice Instruction'
  // Background: Open TTS API supports 'voice instruction' (kind of 'voice system prompt) for customizing voice 'character'
  // We utilize this capability via keyword 'VOICE' -> sending request to LLM + Instruction (latest AUDIO.H + PSRAM needed!)
  // Use case example (Recording request): "Can you speak in silent voice please or whispering ?" -> Test it once ;)
  
  if (cmd == "VOICE" || cmd == "STIMME")                          // single word 'VOICE' only: Reset voice instruction prompt
  {  Serial.println( "< Voice instruction removed >" );   
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD      
     voice_instruct = ""; cmd = "";
     UserRequest = "";  // do NOT start LLM
  }
  if (cmd.indexOf("VOICE") >=0 || cmd.indexOf("STIMME") >=0 )     // request contains word 'VOICE' -> activate voice instruction
  {  Serial.println( "< Voice instruction stored > [" + UserRequest + "]");   
     led_RGB(LOW,HIGH,HIGH); delay(200);                          // LED: ## RED Flash (200ms) on detected KEYWORD  
     voice_instruct = UserRequest;
     // NOT erasing UserRequest -> starting LLM too
  }

  // 4. keyword 'GOOGLE' -> launching Open AI WEB SEARCH feature (handled in LMM call below)
  // 5. key COMMAND '#'  -> Serial Printing of complete CHAT history (built-in feature in Open_AI() call in lib._OpenAI_Chat.ino
  
   
  // ------ USER REQUEST found -> Call Open AI LLM ------------------------------------------------------------------------------
  // #NEW: Update: Utilizing new 'real-time' web search feature in case user request contains the keyword 'GOOGLE' 
  // [using same 'Open_AI(..)' function with additional parameter (Open_AI() function switches to dedicated LLM search model]
  // Recap: Open_AI() remembers complete prompts history (appending prompts) to support ongoing dialogs (web searches included;)
  
  if (UserRequest != "" ) 
  {     
    // # NEW #: launch Open AI WEB SEARCH feature if user request includes the keyword 'Google'
    // supporting User requests like 'Will it rain tomorrow in my region?, please ask Google'
        
    if ( UserRequest.indexOf("Google") >= 0 || UserRequest.indexOf("GOOGLE") >= 0 )                          
    {  
       led_RGB(LOW,HIGH,LOW);                                     // LED: ## MAGENTA indicating NEW Open AI WEB SEARCH Request
       Serial.print( "OpenAI SEARCH LLM> " );                     // function Open_AI(UserRequest) will Serial.print '...'
       
       // some workarounds are recommended to utilize the new Open AI Web Search for TTS (speaking the response).
       // Background: New Open AI Web Search models are not intended for TTS (much too detailed, including lists & links etc.)
       // and they are also 'less' prompt sensitive, means ignoring earlier instructions (e.g. 'shorten please!') quite often 
       // so we use 2 tricks: adding a 'default' instruction each time (forcing 'short answers') + removing remaining links
       // KEEP in mind: SEARCH models are slower (response delayed) than CHAT models, so we use on (GOOGLE) demand only !
       
       String Prompt_Enhanced = UserRequest + (String) WEB_SEARCH_ADDON;    // Trick 1: append user defined annex (see #define)
                                                                            // 'true' means: launch WEB SEARCH model
       // Action happens here! (WAITING until Open AI web search is done)        
       LLM_Feedback = Open_AI( Prompt_Enhanced, OPENAI_KEY, true, WEB_SEARCH_USER_CITY );   

       // even in case WEB_SEARCH_ADDON contains a instruction 'no links please!: there are still rare situation that 
       // links are included (eof search results). So we cut them manually  (prior sending to TTS / Audio speaking)
       
       int any_links = LLM_Feedback.indexOf( "([" );                    // Trick 2: searching for potential links at the end
       if ( any_links > 0 )                                             // (they typically start with '([..'  
       {  Serial.println( "\n>>> RAW: [" + LLM_Feedback + "]" );        // Serial Monitor: printing both, TTS: uses cutted only  
          LLM_Feedback = LLM_Feedback.substring(0, any_links) + "|";    // ('|' just as 'cut' indicator for Serial Monitor)
          Serial.print( ">>> CUTTED for TTS:" );    
       } 
    }
    
    else  // DEFAULT: LLM chat completion model (for human like conversations) 
    
    {  led_RGB(HIGH,HIGH,LOW);                                    // LED: ## BLUE indicating Open AI CHAT Request starting
       Serial.print( "OpenAI LLM> " );                            // function Open_AI(UserRequest) will Serial.print '...'
       
       // Action happens here! (WAITING until Open AI done)
       LLM_Feedback = Open_AI( UserRequest, OPENAI_KEY, false, "" );                
    }
          
    // final tasks (always):
    
    if (LLM_Feedback != "")                                       // in case we got any valid feedback ..  
    { led_RGB(LOW,LOW,LOW); delay(200);                           // -> LED: ## WHITE FLASH (200ms)    
      led_RGB(HIGH,HIGH,HIGH); delay(100);  
      
      Serial.println( " [" + LLM_Feedback + "]" );    
      LLM_Feedback_before = LLM_Feedback;                           
    }           
    else Serial.print("\n");   
  }


  // ------ Speak LLM answer with Open AI voices --------------------------------------------------------------------------------
  // Calling  TextToSpeech(), here: using OpenAI TTS services

  if (LLM_Feedback != "") 
  {  led_RGB(LOW,LOW,HIGH);                                       // LED: ## YELLOW indicating 'TTS' audio pending 

     // using fastest 'tts-1' model by default, gpt..tts models needed for voice instruction modified voices
     String tts_model = (voice_instruct != "") ? ("gpt-4o-mini-tts") : ("tts-1");

     TextToSpeech( OPENAI_KEY, tts_model, LLM_Feedback, voice_instruct, FAVORITE_VOICE_TTS, "aac", "1" ); 
  }

    
  // Adjusting Audio Volume (either via Analogue POTI or NEW: via toggle button) ------------------------------------------------
  
  if (pin_VOL_POTI != NO_PIN)     // --- KALO default: if available then using an POTI to adjust Audio Volume continuously 
  {  static long millis_before = millis();  
     static int  volume_before;
     // reading POTI, rarely only (e.g. 4 times/sec and only if diff >= 2) to avoid unnecessary audio flickering
     if (millis() > (millis_before + 250))  // each 250ms 
     { millis_before = millis(); 
       int volume = map( analogRead(pin_VOL_POTI), 0, 4095, 0, 21 );
       if ( abs(volume-volume_before) >= 2  )  // using 2 avoids flickering  
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



// ******************************************************************************************************************************
// Updating LED with RGB on/off values  

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



// ******************************************************************************************************************************
// audio_info(const char *info) is an inbuilt (optional) Callback function in AUDIO.H, printing details of played Audio events
// using this feature in DEBUG mode only

void audio_info(const char *info)
{  DebugPrintln( "AUDIO.H info: " + (String) info );   
}



// ******************************************************************************************************************************
// TextToSpeech() - Using OpenAI TTS services, launching function '.openai_speech()' from @Schreibfaul1's AUDIO.H library.
// Audio will be played via I2S Amplifier (e.g. MAX98357), pins see header of sketch 
// - IMPORTANT: Be aware of AUDIO.H dependency -> CHECK bottom line (UPDATE in case older AUDIO.H used) to avoid COMPILER ERRORS!

void TextToSpeech( String key, String model, String request, String voice_instruct, String voice, String format, String vspeed)
{   
   // OpenAI voices are multi-lingual :) .. 9 tts-1 voices (May 2025): alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer
   // Supported audio formats (response): aac | mp3 | wav (sample rate issue) | flac (PSRAM needed) 
   // Known AUDIO.H issue: Latency delay (~1 sec) before voice starts speaking 
   
   // - Link OpenAI TTS doc: https://platform.openai.com/docs/guides/text-to-speech/text-to-speech
   // - Link OpenAI TTS API reference: https://platform.openai.com/docs/api-reference/audio/createSpeech
   // - Link to test voices: https://platform.openai.com/playground/tts
   // - Link to more complex voice instruction prompts: https://www.openai.fm/

   // Params of TextToSpeech() / .openai_speech():
   // - model:          Available TTS models: tts-1 | tts-1-hd | gpt-4o-mini-tts (gpt models needed for 'voice instruct')
   // - request:        The text to generate audio for. The maximum length is 4096 characters.
   // - voice_instruct: Optional description of the voice sytle ("e.g. "you are whispering") / supported since AUDIO.H 3.1.0
   // - voice:          Voice name (multilingual), May 2025: alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer
   // - format:         supported audio formats: aac | mp3 | wav (sample rate issue) | flac (PSRAM needed) 
   // - vspeed:         The speed of the generated voice. Select a value from 0.25 to 4.0. DEFAULT: 1.0
      
   String Voice;
   String Voices[9] = { "alloy", "ash", "coral", "echo", "fable", "onyx", "nova", "sage", "shimmer" }; 
   
   if (FAVORITE_VOICE_TTS != "") { Voice = FAVORITE_VOICE_TTS; }    // Default: using user favorite voice  
   if (FAVORITE_VOICE_TTS == "") { Voice = Voices[random(9)];  }    // in case no preference: just test on by random
      
   Serial.print( "\nOpenAI SPEAKING, Voice = [" + Voice + "]" );      
   if ( voice_instruct != "") { Serial.print( "\nVoice INSTRUCTION = [" + voice_instruct + "]" ); }
   Serial.println();  

   /* in case you prefer free GOOGLE TTS (free of usage, but less 'human' and LIMITED in length: long sentences won't work !)
   // Google TTS are mono lingual only, language tag needed, e.g. en, en-US, en-IN, en-BG, en-AU, de-DE, th-TH etc.
   // more infos: https://cloud.google.com/text-to-speech/docs/voices */
   // audio_play.connecttospeech( request.c_str(), "en");    // <- uncomment this line for Google TTS via AUDIO.H 

   // DEFAULT: Using Open AI TTS, multi-lingual & human-sounding voices. CALL: '.openai_speech()' function in AUDIO.H library 
   // - IMPORTANT: The amount of .openai_speech() parameters changed in AUDIO.H library versions 
   // - Background: since AUDIO.H ver. '3.1.0u' a 4th parameter 'voice instruction' is added (usage e.g.: "you are whispering")
   // - choose the correct amout of parameter to avoid COMPILATION ERRORS (and set the wrong one into comment) ..
   //   ==> So CHECK your library & update this last line in code if needed !! ....
   
/* audio_play.openai_speech(key, model, request, voice_instruct, voice, format, vspeed);  // <- use this if version >= 3.1.0u */
   audio_play.openai_speech(key, "tts-1", request,               voice, format, vspeed);  // <- use this for e.g. 3.0.11g !
}

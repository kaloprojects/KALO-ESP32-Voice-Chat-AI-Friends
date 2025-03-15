
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                               KALO_ESP32_Voice_ChatGPT                                   ------------------
// ------------------      Example Code snippet for an ESP32 voice dialog device for Open AI (ChatGPT)         ------------------
// ------------------          [using KALO C++ libraries from github 'KALO-ESP32-Voice-Assistant']             ------------------
// ------------------                                                                                          ------------------
// ------------------                            Latest Update: March 14, 2025                                 ------------------
// ------------------           [NEW in this Update: TECHIESMS Voice Assistant PCB supported]                  ------------------
// ------------------             https://techiesms.com/product/portable-ai-voice-assistant                    ------------------
// ------------------                                                                                          ------------------
// ------------------                                    > Workflow <                                          ------------------
// ------------------             Entering User request via keyboard (Serial Monitor) OR Voice                 ------------------
// ------------------         Voice RECORDING (on holding BTN) with variable length [KALO I2S code]            ------------------
// ------------------                   STT SpeechToText [using Deepgram API service]                          ------------------
// ------------------                    Call OpenAI LLM, remembering dialog history                           ------------------
// ------------------                         TTS TextToSpeech with OpenAI voices                              ------------------
// ------------------             Short BTN Touch: STOP Audio or REPEAT last OpenAI response                   ------------------
// ------------------                                                                                          ------------------
// ------------------                        > Hardware Requirements / Circuit <                               ------------------
// ------------------                         ESP32 with connected Micro SD Card                               ------------------
// ------------------                    SD Card: using VSPI Default pins 5,18,19,23                           ------------------
// ------------------                   I2S Amplifier (e.g. MAX98357), pins see below                          ------------------
// ------------------------------------------------------------------------------------------------------------------------------


// *** HINTS: 
// 1. in case of an 'Sketch too Large' Compiler Warning/ERROR in Arduino IDE (ESP32 Dev Module):
//    -> select a larger 'Partition Scheme' via menu > tools: e.g. using 'No OTA (2MB APP / 2MB SPIFFS) ***
// 2. TECHIESMS pcb: In case PC does not detect ESP32 Serial Port, then install CH340 driver (missed on older Windows versions)
//    -> driver links here: https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all#windows-710


#define VERSION           "\n=== KALO ESP32 Voice ChatGPT (last update: March 14, 2025) =========================="   

#include <WiFi.h>         // only included here
#include <SD.h>           // also needed in other tabs (.ino) 

#include <Audio.h>        // needed for PLAYING Audio (via I2S Amplifier, e.g. MAX98357) with ..
                          // Audio.h library from Schreibfaul1: https://github.com/schreibfaul1/ESP32-audioI2S
                          // > ensure you have actual version (July 18, 2024 or newer needed for 8bit wav files!)
                          // > [reason: bug fix for 8bit https://github.com/schreibfaul1/ESP32-audioI2S/issues/786]                          


// --- PRIVATE credentials -----

const char* ssid =        "...";          // ### INSERT your wlan ssid 
const char* password =    "...";          // ### INSERT your password  
const char* OPENAI_KEY =  "...";          // ### INSERT your OpenAI key

 
// --- user preferences -------- 

String  favorite_VOICE =  "onyx";         // forcing voice (DemoVideo: "onyx"), keep EMPTY "" if you want to hear all by random
                                          // Multilingual Voices (Jan. 2025): alloy,ash,coral,echo,fable,onyx,nova,sage,shimmer

#define AUDIO_FILE        "/Audio.wav"    // mandatory, filename for the AUDIO recording

#define WELCOME_FILE      "/Welcome.wav"  // optionally, 'Hello' file on SD will be played once on start (e.g. a gong or voice)

#define WELCOME_MSG       "Hi my friend, welcome back. How can i help you ?"     // optionally on start (with favorite_VOICE)


// --- PIN assignments ---------

#define pin_RECORD_BTN    36    // primary RECORD button
#define pin_TOUCH         13    // alternative RECORD TOUCH btn 
#define pin_VOL_POTI      34    // Volume wheel poti

#define pin_LED_RED       15    // LED (LOW for Color ON)
#define pin_LED_GREEN     2     // R-G-B examples: TECHIESMS 15-2-4, KALO Prototyp 15-2-0, KALO LUCA 15-21-4 
#define pin_LED_BLUE      4

#define pin_I2S_DOUT      25    // 3 pins for I2S Audio Output
#define pin_I2S_LRC       26
#define pin_I2S_BCLK      27


// --- NEW: enhancements for TECHIESMS pcb circuits ---
// Supporting techiesms pcb: https://techiesms.com/product/portable-ai-voice-assistant/

#define TECHIESMS_PCB     true  // NEW: set to 'true' if techiesms pcb used (false on KALO default pcb with audio volume poti)
#define pin_TPCB_VOL_BTN  13    // used as 3 step VOL stepper 1>2>3 (TECHIESMS pcb only, #define TECHIESMS_PCB must be true) 
#define pin_TPCB_TOUCH    12    // used as alternative RECORD TOUCH btn (TECHIESMS pcb only, #define TECHIESMS_PCB must be true) 

int gl_TPCB_VolSteps[] =  { 16, 19, 21 };    // user can define multiple steps (3 steps is just an example), max value is 21
                                             // hint: TECHIESMS pcb sounds pretty silent, so we recommend high values only        
int gl_TPCB_VolDefault =  19;                // Default Audio volume (on Power On)


// --- global Objects ----------

Audio audio_play;

int gl_TOUCH_RELEASED;    // measured once in setup(), once is enough because ESP32 Touch stable untouched signal 
                          // typical values: untouched/empty ~50, when touched about 10-30
                          // Rule (by own tests): we trigger if current value is below 80% of gl_TOUCH_RELEASED


// declaration of functions in other modules (not mandatory but ensures compiler checks correctly)
// splitting Sketch into multiple tabs see e.g. here: https://www.youtube.com/watch?v=HtYlQXt14zU

bool    I2S_Record_Init(); 
bool    Record_Start( String filename ); 
bool    Record_Available( String filename, float* audiolength_sec ); 

String  SpeechToText_Deepgram( String filename );

String  Open_AI( String UserRequest, const char* LLM_API_key );  




// ******************************************************************************************************************************

void setup() 
{   
  // Initialize serial communication
  Serial.begin(115200); 
  Serial.setTimeout(100);    // 10 times faster reaction after CR entered (default is 1000ms)

  // Pin assignments:
  pinMode(pin_LED_RED, OUTPUT);  pinMode(pin_LED_GREEN, OUTPUT);  pinMode(pin_LED_BLUE, OUTPUT);
  pinMode(pin_RECORD_BTN, INPUT );  // use INPUT_PULLUP if no external Pull-Up connected ##
  pinMode(pin_TPCB_VOL_BTN, INPUT );  
    
  // on INIT: walk 1 sec thru 3 RGB colors (RED -> GREEN -> BLUE) 
  led_RGB(LOW,HIGH,HIGH); delay (330);  led_RGB(HIGH,LOW,HIGH); delay (330);  led_RGB(HIGH,HIGH,LOW); delay (330); 
   
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

  // Initialize SD card
  if (!SD.begin()) 
  { Serial.println("ERROR - SD Card initialization failed!"); 
    return; 
  }
  
  // initialize KALO I2S Recording Services (don't forget!)
  I2S_Record_Init();        
    
  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT);
  
  // Calibration: measure initial TOUCH value (later on: all below e.g. 80% of this 'un-touched' value will trigger TOUCH) 
  gl_TOUCH_RELEASED = (TECHIESMS_PCB) ? touchRead(pin_TPCB_TOUCH) : touchRead(pin_TOUCH);  // trick #17' ;) .. different pins
  
  // Optional 1st: Say 'Hello' - Playing a optional WELCOME_FILE wav file once
  if ( SD.exists( WELCOME_FILE ) )
  {  audio_play.setVolume(12);   // we play file welcome file with fixed reduced volume (e.g. 12 of 21)   
     audio_play.connecttoFS( SD, WELCOME_FILE );  
     
     // using 'isRunning()' trick to wait in setup() until PLAY is done (when done: proceed with next WELCOME_MSG below)
     while (audio_play.isRunning())
     { audio_play.loop();
     }     
  }

  // Optional 2nd: Speak a WELCOME TEXT on Init (only if a favorite voice defined)
  if ( WELCOME_MSG != "" && favorite_VOICE != "")
  {  audio_play.openai_speech(OPENAI_KEY, "tts-1", WELCOME_MSG, favorite_VOICE, "aac", "1");   
     // no waiting, means: now jump into main loop (using 'audio_play.loop()' in main loop())
  }
  
  // TECHIESMS pcb has no Volume POTI, so we start with default Audio Volume (using 2nd array value, typically mid range)
  if (TECHIESMS_PCB) { audio_play.setVolume(gl_TPCB_VolDefault); }
  
  // INIT done, starting user interaction
  Serial.println( "> Hold or Touch button during recording VOICE .. OR enter request in Serial Monitor" );  
  Serial.println( "  [command '#': chat history, keyword 'RADIO' or 'DAILY NEWS/Tagesschau': streaming]\n" );  
  
}



// ******************************************************************************************************************************

void loop() 
{
  String UserRequest;                     // user request, initialized new each loop pass 
  String LLM_Feedback;                    // Open AI response
  
  static String LLM_Feedback_before;      // static var to keep information from last request (as alternative to global var)
  static bool flg_UserStoppedAudio;       // both static vars are used for 'Press button' actions (stop Audio a/o repeat LLM)
  
  
  // ------ Read USER INPUT via Serial Monitor (fill String UserRequest and ECHO after CR entered) ------------------------------
  // ESP32 as TEXT Chat device: this allows to use the Serial Monitor as an Open AI text chat device  
  // (hidden feature, covered in lib_OpenAI_Chat.ino: enter '#' in Serial Monitor to list the history of complete chat dialog)
  
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


  // ------ Read USER INPUT via Voice recording & Deepgram transcription --------------------------------------------------------
  // ESP32 as VOICE chat device: Recording & Transcription (same result: filling String 'UserRequest' with spoken text)  
  // Pressing a button (or touching a pin) as long speaking. Code below supports buttons & touch pins (just for demo purposes) 
  // 3 different BTN actions:  PRESS & HOLD for recording || STOP (Interrupt) Open AI speaking || REPEAT last Open AI answer  

  int current_touch = (TECHIESMS_PCB) ? touchRead(pin_TPCB_TOUCH) : touchRead(pin_TOUCH);   // using correct pin (depending pcb)
  bool flg_touched  = (current_touch <= (int) (gl_TOUCH_RELEASED * 0.8)) ? true : false;     

  if (digitalRead(pin_RECORD_BTN) == LOW || flg_touched)          // # Recording started, supporting btn and touch sensor
  {  delay(30);                                                   // unbouncing & suppressing finger button 'click' noise 
 
     if (audio_play.isRunning())                                  // Before we start any recording: always stop earlier Audio 
     {  audio_play.connecttohost("");                             // 'audio_play.stopSong()' won't work (collides with STT)
        flg_UserStoppedAudio = true;
        Serial.println( "< STOP AUDIO >" );
     }   
     else flg_UserStoppedAudio = false;                           // trick allows to distinguish between STOP and REPEAT  
         
     // Now Starting Recording (no blocking, not waiting)
     Record_Start(AUDIO_FILE);                                    // that's the main task: Recording AUDIO (ongoing)  
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH && !flg_touched)        // # Recording not started yet OR stopped (on release button)
  {  
     float recorded_seconds; 
     if (Record_Available( AUDIO_FILE, &recorded_seconds ) )      // true ONCE when recording finalized (.wav file available)
     {  if (recorded_seconds > 0.4)                               // using short btn TOUCH (<0.4 secs) for other actions
        {  led_RGB(HIGH,LOW,LOW);                                 // LED [Update]: ## CYAN indicating Deepgram STT starting 
           Serial.print( "\nYou {STT}> " );                       // function SpeechToText_Deepgram will append '...'
                   
           UserRequest = SpeechToText_Deepgram( AUDIO_FILE );     // Action happens here! (WAITING until Deepgram done)
           
           if (UserRequest != "")                                 // Done!. In case we got a valid spoken transcription:   
           {  led_RGB(LOW,LOW,LOW); delay(200);                   // LED: ## WHITE FLASH (200ms)
              led_RGB(HIGH,HIGH,HIGH); delay(100);                    
           }    
           Serial.println( " [" + UserRequest + "]" );            // printing result in Serial Monitor always              
        }
        else                                                      // 2 additional Actions on short button PRESS (< 0.4 secs):
        { if (!flg_UserStoppedAudio)                              // - STOP AUDIO when playing (done above, if Btn == LOW)
          {  Serial.print( "< REPEAT TTS > " );                   // - REPEAT last LLM answer (if currently not playing)
             LLM_Feedback = LLM_Feedback_before;                  
          }
        }
     }      
  }  


  // ------ USER REQUEST found -> Check gimmicks first (Playing RADIO or TV NEWS ------------------------------------------------
  
  String cmd = UserRequest;
  cmd.toUpperCase();

  // Gimmick 1: Check for keyword 'RADIO' inside the user request -> Playing German RADIO Live Stream: SWR3
  if (cmd.indexOf("RADIO") >=0 )
  {  Serial.println( "< Streaming German RADIO: SWR3 >" );   
     audio_play.connecttohost( "https://liveradio.swr.de/sw282p3/swr3/play.mp3" ); 
     UserRequest = "";
  } 

  // Gimmik 2: Check for keyword 'DAILY NEWS' or German 'TAGESSCHAU' -> Playing German TV News: Tagesschau24
  if (cmd.indexOf("DAILY NEWS") >=0 || cmd.indexOf("TAGESSCHAU") >=0 ) 
  {  Serial.println( "< Streaming German Daily News TV: Tagesschau24 >" );   
     audio_play.connecttohost( "https://icecast.tagesschau.de/ndr/tagesschau24/live/mp3/128/stream.mp3"  ); 
     UserRequest = "";
  }

   
  // ------ USER REQUEST found -> Call Open AI LLM ------------------------------------------------------------------------------
  
  if (UserRequest != "" ) 
  { 
    led_RGB(HIGH,HIGH,LOW);                               // LED: ## BLUE indicating Open AI Request starting
    Serial.print( "OpenAI LLM> " );                       // function Open_AI(UserRequest) will append '...'
    
    LLM_Feedback = Open_AI( UserRequest, OPENAI_KEY );    // Action happens here! (WAITING until Open AI done) 
      
    if (LLM_Feedback != "")                               // in case we got a valid feedback -> LED: ## WHITE FLASH (200ms)  
    { led_RGB(LOW,LOW,LOW); delay(200);                
      led_RGB(HIGH,HIGH,HIGH); delay(100);  
      
      Serial.println( " [" + LLM_Feedback + "]" );    
      LLM_Feedback_before = LLM_Feedback;                           
    }           
    else Serial.print("\n");   
  }


  // ------ Speak LLM answer with Open AI voices (9 multi-lingual voices per random) --------------------------------------------
  // also printing in Serial Monitor
   
  if (LLM_Feedback != "") 
  {   
     led_RGB(LOW,LOW,HIGH);             // LED [Update]: ## YELLOW indicating 'TTS' launched (with Open AI voice)
     
     // [TextToSpeech OpenAI] - Repeat your sentence with 'human sounding' voices 
     // All voices are multi-lingual (!), available voices (Jan. 2025): alloy, ash, coral, echo, fable, onyx, nova, sage, shimmer
     // More info, testing voice samples here: https://platform.openai.com/docs/guides/text-to-speech/text-to-speech

     String Voices[9] = { "alloy", "ash", "coral", "echo", "fable", "onyx", "nova", "sage", "shimmer" }; 
     String Voice = Voices[random(9)];   // speak with different voices always (just for demo purposes)  
       
     if (favorite_VOICE != "") {Voice = favorite_VOICE;}      // or stay with user favorite voice in case defined in header 

     // Print in Serial Monitor
     Serial.println( "OpenAI Voice = [" + Voice + "]" );     
           
     // Action: Play TTS OpenAI via connected I2S Speaker:
     // (hint: supported formats for '.openai_speech()': aac | mp3 | wav (sample rate issue) | flac (PSRAM needed) )
     // known AUDIO.H issue: Latency delay (~1 sec) before voice starts speaking 
           
     audio_play.openai_speech(OPENAI_KEY, "tts-1", LLM_Feedback.c_str(), Voice, "aac", "1");    
  }


  // ----------------------------------------------------------------------------------------------------------------------------
  // re-adjust Audio Volume (either via Analogue POTI or NEW: via toggle button)
  
  if (!TECHIESMS_PCB)     // --- KALO default: using a Analogue POTI to adjust Audio Volume continuously
  {  static long millis_before = millis();  
     static int  volume_before;
     // reading POTI, only 5 times per second to avoid unnecessary calls or flickering)
     if (millis() > (millis_before + 200))  // each 200ms
     { millis_before = millis(); 
       int volume = map( analogRead(pin_VOL_POTI), 0, 4095, 0, 21 );
       if (volume != volume_before)
       {  volume_before = volume;
          audio_play.setVolume(volume);  // AUDIO: values from 0 to 21          
       }    
     } 
  }
    
  if (TECHIESMS_PCB)    // --- NEW: TECHISMS pcb does not offer a POTI, so we utilize free side button to toggle thru N values
  {  static bool flg_volume_updated = false; 
     static int volume_level = -1; 
     if (digitalRead(pin_TPCB_VOL_BTN) == LOW && !flg_volume_updated)          
     {  int steps = sizeof(gl_TPCB_VolSteps) / sizeof(gl_TPCB_VolSteps[0]);  // typically: 3 steps (any arrays supported)
        volume_level = ((volume_level+1) % steps);  // walking in circle (starting with 0): e.g. 0 -> 1 -> 2 -> 0 ..
        Serial.println( "New Audio Volume: [" + (String) volume_level + "] = " + (String) gl_TPCB_VolSteps[volume_level] );
        // visualize to user with ## WHITE FLASHES (with OFF between flashes)
        for (int i=0; i<=volume_level; i++)  
        {  led_RGB(LOW,LOW,LOW); delay(80); led_RGB(HIGH,HIGH,HIGH); delay(120);  // white flash 80ms, OFF 120ms                          
        }   
        flg_volume_updated = true;
        audio_play.setVolume( gl_TPCB_VolSteps[volume_level] );  
    }
    if (digitalRead(pin_TPCB_VOL_BTN) == HIGH && flg_volume_updated)         
    {  flg_volume_updated = false;    
    }
  }
  
  
  // ----------------------------------------------------------------------------------------------------------------------------
  // Play AUDIO (Schreibfaul1 loop for Play Audio (details here: https://github.com/schreibfaul1/ESP32-audioI2S))
  // and updating LED status
  
  audio_play.loop();  
  vTaskDelay(1); 

  // update LED status always (in addition to WHITE flashes + YELLOW on STT + BLUE on LLM + CYAN on TTS)
  if (digitalRead(pin_RECORD_BTN)==LOW || flg_touched) { led_RGB(LOW,HIGH,HIGH); }   // led RED as long the record ongoing
  else if (audio_play.isRunning())      { led_RGB(LOW,HIGH,LOW);  }   // led MAGENTA when Open AI voice is speaking 
  else                                  { led_RGB(HIGH,LOW,HIGH); }   // led GREEN (default) when ready for next user request
    
}




// ******************************************************************************************************************************

void led_RGB( bool red, bool green, bool blue ) 
{ // general usage: using LOW in code switches the LED color on, e.g. led_RGB(LOW,HIGH,HIGH) means RED on   
  // using static vars, reason: writing to real pin only if changed (increasing performance for frequently repeated calls)
  
  static bool red_before=HIGH, green_before=HIGH, blue_before=HIGH;  // memo: HIGH is same as true(1), LOW is false(0)

  if (!TECHIESMS_PCB)   // KALO default pcb, using complete Vcc (soldered serial resistor exist), common Anode 
  {  if (red   != red_before)   { digitalWrite(pin_LED_RED,red);     red_before=red;     }     
     if (green != green_before) { digitalWrite(pin_LED_GREEN,green); green_before=green; }      
     if (blue  != blue_before)  { digitalWrite(pin_LED_BLUE,blue);   blue_before=blue;   }      
  }   
  if (TECHIESMS_PCB)    // special case: soldered LED have common GND, missed serial resitors (so we must reduce voltage)
  {  // writing analog values 0 or max. ~40 (not 255!, means no digitalWwrite!) 
     if (red   != red_before)   { analogWrite(pin_LED_RED,  (red==LOW)?   40:0 );  red_before=red;     }      
     if (green != green_before) { analogWrite(pin_LED_GREEN,(green==LOW)? 40:0 );  green_before=green; }      
     if (blue  != blue_before)  { analogWrite(pin_LED_BLUE, (blue==LOW)?  40:0 );  blue_before=blue;   }   
  }   
}

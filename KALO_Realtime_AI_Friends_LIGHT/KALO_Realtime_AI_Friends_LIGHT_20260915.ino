
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                          *** KALO_Realtime_AI_Friends_LIGHT ***                          ------------------
// ------------------                                                                                          ------------------
// ------------------       ESP32 Open AI Live REALTIME conversation with Multiple AI characters (FRIENDS)     ------------------
// ------------------                  HW: ESP32/ESP32-S3 + I2S microphone + I2S Amplifier                     ------------------
// ------------------                             Latest Update: Sept. 15, 2026                                ------------------
// ------------------                                     Coded by KALO                                        ------------------
// ------------------                                                                                          ------------------
// ------------------          > LIGHT Limitations (compared to 'KALO_ESP32_Voice_Chat_AI_Friends') <          ------------------
// ------------------   - INPUT_BUTTON_MODE (recording by button) removed, starting directly in REALTIME       ------------------
// ------------------   - AUDIO.H library & features removed (no music, no url/radio streaming, no SD card)    ------------------
// ------------------   - web search feature optional: just add <lib_openai_groq_chat.ino> to AUTO activate    ------------------
// ------------------                                                                                          ------------------
// ------------------                                   > LIGHT Advantages <                                   ------------------
// ------------------   - Slim & compact code, easy workflow, versatile framework for further user add-ons     ------------------
// ------------------   - Easy to use (no btn needed), same emotional & fast REALTIME response (low latency)   ------------------
// ------------------   - Kept all AI features (call friends, multi-lingual, live transcript print, etc.)      ------------------
// ------------------   - HW requirements: supporting older/limited ESP32 (without S3, no PSRAM, no SD card)   ------------------
// ------------------   - Libs <lib_audio_recording.ino> & <lib_audio_transcription.inoi> no longer needed     ------------------
// ------------------   - Simple registration: Open AI key only (no Elevenlabs / no Deepgram / no Groq)        ------------------
// ------------------                                                                                          ------------------
// ------------------                          > Hardware PCB templates / circuits <                           ------------------
// ------------------               KALO AI Board PCB (DIY template), Source & Gerber files:                   ------------------
// ----------------    https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb   ----------------
// ------------------        other supported 'Ready to Go' devices (see KALO_ESP32_Voice_Chat_AI_Friends)      ------------------
// ------------------                     e.g. Elato AI, TechieSMS Assistant V1 & V3, etc.                     ------------------
// ------------------------------------------------------------------------------------------------------------------------------


// ## Hint: Configure your device to your AUDIO needs & environment, check #defines in <lib_openai_realtime.ino> header: ##
// - [REALTIME_OUTPUT_GAIN_INIT] & [REALTIME_OUTPUT_GAIN_MAX] define your favorite Audio OUT Response Volume 
// - [REALTIME_LOCAL_VAD_START_LEVEL] & [REALTIME_LOCAL_VAD_KEEP_LEVEL] adjust the Audio IN voice detection (environment noise)


#include <WiFi.h>                // AUDIO.H not needed (REALTIME only) -> no SD card or PSRAM needed, older ESP32 supported
#include <WebSocketsClient.h>
#include <WiFiClientSecure.h>

bool     DEBUG = false;          // <- User Preference on Power On, can also be toggled during runtime via command 'DEBUG ON|OFF'
#define  DebugPrint(x);          if(DEBUG){Serial.print(x);}
#define  DebugPrintln(x);        if(DEBUG){Serial.println(x);}


// === PRIVATE credentials =====

const char* ssid =               "...";         // ### INSERT your wlan ssid       [mandatory]
const char* password =           "...";         // ### INSERT your password        [mandatory]
const char* OPENAI_KEY =         "...";         // ### INSERT your OpenAI API KEY  [mandatory]

#define WEB_SEARCH_USER_CITY     "Bangkok"      // ### INSERT your user city, optimizes OpenAI 'websearch' [optional]
#define HELLO_TO_MY_FRIEND       "Hey there!"   // ### INSERT your inital 'Hello' to REALTIME (use your favorite language) 


// === HARDWARE settings ======= // more templates see: 'KALO_ESP32_Voice_Chat_AI_Friends.ino' (compatible syntax)

// --- PCB: TECHIESMS V1 ------- // ESP32 WROOM (N4R0, no PSRAM) & SD: https://techiesms.com/product/portable-ai-voice-assistant/
#define pin_I2S_DOUT    25       // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     26
#define pin_I2S_BCLK    27
#define I2S_WS          22       // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          35
#define I2S_SCK         33
#define I2S_LR          HIGH     // HIGH (L/R pin Vcc, RIGHT channel)
#define NO_PIN          -1       // [mandatory] ALIAS for 'not used' on optional control pins (3xLED, Buttons/Touch/Poti)
#define flg_LED_DIGITAL false    // [mandatory] false (!) -> [TECHIESMS]: no Serial Resistor + common GND (Analog Pin)
#define pin_LED_RED     15       // [optional] Use NO_PIN (if not available)
#define pin_LED_GREEN   2        // [optional] Use NO_PIN (if not available)
#define pin_LED_BLUE    4        // [optional] Use NO_PIN (if not available)
#define pin_RECORD_BTN  36       // [optional] earlier RECORD button -> used to STOP Realtime Audio
#define pin_TOUCH       12       // [optional] # KALO mod # - additional STOP TOUCH control on TOP panel (soldered to pin 12)
#define pin_VOL_POTI    NO_PIN   // [optional] no analogue wheel POTI available, using VOL_BTN for audio volume control
#define pin_VOL_BTN     13       // [optional] # KALO mod # - using side button for volume (orig. REPEAT btn no longer needed)

/* --- PCB: TECHIESMS V3 ------- // ESP32-S3 Dev (N16R8, 8MB PSRAM, no SD Card) .. same chip as in ElatoAI (below):
// Techiesms details link:       // https://techiesms.com/product/xiaozhi-ai-voice-assistant-esp32-s3-based-smart-assistant/
#define pin_I2S_DOUT    7        // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     16
#define pin_I2S_BCLK    15
#define I2S_WS          4        // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          6
#define I2S_SCK         5
#define I2S_LR          LOW      // LOW (L/R pin GND, default LEFT channel)
#define NO_PIN          -1       // ALIAS definition for 'not used', supported on all optional control pins
#define flg_LED_DIGITAL false    // [mandatory] false (!) -> [TECHIESMS]: no Serial Resistor + common GND (Analog Pin)
#define pin_LED_RED     8        // [optional] 3 x Status LED R-G-B pins (rule in code: LOW switches led color ON)
#define pin_LED_GREEN   19       // <- unfortunately connected to USB D-/D+ (pins 19/20) -> GREEN sometimes won't work on all pcb
#define pin_LED_BLUE    20       // fix: replace NoName LED with a reliable LED (common GND), e.g. Kingbright WP154A4SUREQBFZGC
#define pin_RECORD_BTN  14       // [optional] using REPEAT side button -> used to STOP Realtime Audio
#define pin_TOUCH       NO_PIN   // [optional] not available
#define pin_VOL_POTI    NO_PIN   // [optional] no wheel POTI available, using VOL_BTN for audio volume control 
#define pin_VOL_BTN     40       // [optional] Volume increase control [VOL- Button 39 currently not used]  */

/* --- PCB: KALO AI BOARD ------ // ## NEW - DFRobot FireBeetle2 ESP32-S3 WROOM-1(N16R8, 8MB PSRAM) - KALO AI Board PCB:
// PCB Layout (battery powered): // https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/hardware_pcb
// WIKI DFRobot Fire Beetle 2:   // https://wiki.dfrobot.com/SKU_DFR0975_FireBeetle_2_Board_ESP32_S3
#define pin_I2S_DOUT    4        // Mandatory: 3 x I2S Audio output pins (for I2S Amplifier MAX98357)
#define pin_I2S_LRC     5
#define pin_I2S_BCLK    6
#define I2S_WS          8        // Mandatory: 4 x I2S Audio input pins for microphone INMP441
#define I2S_SD          11
#define I2S_SCK         10
#define I2S_LR          LOW      // LOW (L/R pin GND, default LEFT channel)
#define NO_PIN          -1       // [mandatory] ALIAS for 'not used' on optional control pins (3xLED, Buttons/Touch/Poti)
#define flg_LED_DIGITAL true     // [mandatory] true [default] -> Serial Resistor, common Vcc (Digital Pin)
#define pin_LED_RED     0        // [optional] Use NO_PIN (if not available)
#define pin_LED_GREEN   9        // [optional] Use NO_PIN (if not available)
#define pin_LED_BLUE    18       // [optional] Use NO_PIN (if not available)
#define pin_RECORD_BTN  38       // [optional] earlier RECORD button -> used to STOP Realtime Audio
#define pin_TOUCH       7        // [optional] earlier RECORD Touch  -> used to STOP Realtime Audio
#define pin_VOL_POTI    3        // [optional] adjusting audio volume
#define pin_VOL_BTN     NO_PIN   // [optional] not needed (adjusting audio volume with continuous VOL_POTI) */


// --- Declarations & Vars -----

// WebSearchResult() declared as weak (option) function: auto enabled if <lib_openai_groq_chat.ino> exists
String __attribute__((weak)) WebSearchResult(String Request, const char* open_key, bool flgEmbedding, const char* groq_key);

// led_RGB() defined here: auto recogniton in other librarys via weak (called there if function defined here)
void   led_RGB( bool red, bool green, bool blue );

uint32_t gl_TOUCH_RELEASED;      // global idle value (untouched), read once on Init, used internally as reference



// ******************************************************************************************************************************

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(100);

  // Pin assignments
  if (pin_LED_RED    != NO_PIN)  { pinMode(pin_LED_RED,   OUTPUT); }
  if (pin_LED_GREEN  != NO_PIN)  { pinMode(pin_LED_GREEN, OUTPUT); }
  if (pin_LED_BLUE   != NO_PIN)  { pinMode(pin_LED_BLUE,  OUTPUT); }
  if (pin_VOL_BTN    != NO_PIN)  { pinMode(pin_VOL_BTN,   INPUT_PULLUP); }
  if (pin_RECORD_BTN != NO_PIN)  { pinMode(pin_RECORD_BTN,INPUT_PULLUP); }  // new usage -> STOP Realtime Audio
  led_RGB(LOW,LOW,LOW); led_RGB(HIGH,HIGH,HIGH);  // init [ON|OFF]

  // Init TOUCH idle value (untouched)
  gl_TOUCH_RELEASED = (pin_TOUCH != NO_PIN) ? (touchRead(pin_TOUCH)) : 0;

  // INIT microphone INMP441
  I2S_Recording_Init();

  // Connecting to WLAN
  led_RGB(LOW,LOW,HIGH);  // #YELLOW ...
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\n> Connecting WLAN " );
  while (WiFi.status() != WL_CONNECTED)
  { Serial.print(".");  delay(500);
  } Serial.println(". Done, device connected.");

  // Starting REALTIME
  RealtimeAPI_Start(OPENAI_KEY);
  RealtimeAPI_WaitUntilReady(10000) ;
  Serial.println( "< REALTIME READY >  " );

  // Wakeing up my Default friend
  led_RGB(HIGH,HIGH,LOW); // #BLUE = speaking
  RealtimeAPI_AddUserText( HELLO_TO_MY_FRIEND );
  RealtimeAPI_StartLLM_GetAudio( 45000, true);
  Serial.println();
  /* sketch waits here .. until Audio Response finished) */
}



// ******************************************************************************************************************************

void loop()
{
  String UserRequest = "";
  String RealtimeUserRequest = "";

  // ------ Read USER INPUT Option (1) - Open AI Realtime STT (non blocking Autorecording in background) - no BTN needed!
  RealtimeUserRequest = RealtimeAPI_Loop();   // recording chunks in loop, receiving user STT when sentence done
  if (RealtimeUserRequest != "")
  {  Serial.println( "\n[RT] You> [" + RealtimeUserRequest + "]" );
     UserRequest = RealtimeUserRequest;
  }

  // ------ Read USER INPUT: Option (2) - Enter request in Serial Monitor via KEYBOARD (usecase: ESP32 as AI TEXT Chat device)
  while (Serial.available() > 0)
  { UserRequest = Serial.readStringUntil('\n');
    UserRequest.replace("\r", "");  UserRequest.replace("\n", "");  UserRequest.trim();
    if (UserRequest != "")
    {  Serial.println( "\nYou> [" + UserRequest + "]" );
    }
  }

  // ------ Launching local Actions via KEYWORDS [optional]
  if ( Keyword_Commands( UserRequest ) )
  {  UserRequest = "";  // to skip any further LLM actions
  }

  // ------ SYNC User VOICE requests & KEYBOARD requests
  if (UserRequest != "" && RealtimeUserRequest == "")    { RealtimeAPI_AddUserText(UserRequest); }      // keyboard used
  if (UserRequest == "" && RealtimeUserRequest != "")    { RealtimeAPI_DeleteLastConversationItem(); }  // ignore local actions


  //  ------ OPTIONAL: Embed selfmade 'Actual Web Search' into REALTIME: ## add <lib_openai_groq_chat.ino> to AUTO activate ! ##
  //  Triggering whole workflow with user defined keywords 'inside' request (e.g. GOOGLE, INTERNET, WEATHER, {German} WETTER ..)
  if (UserRequest != "" && WebSearchResult)  // ! check if WebSearchResult() exists  <lib_openai_groq_chat.ino> or NULL pointer
  {   String cmd = UserRequest; cmd.toUpperCase(); cmd.replace(" ", "");
      if (cmd.indexOf("GOOGLE") >=0 || cmd.indexOf("INTERNET") >=0 || cmd.indexOf("WEATHER") >=0 || cmd.indexOf("WETTER") >=0 )
      {  String LLM_Feedback = WebSearchResult( UserRequest, OPENAI_KEY, true, "" );
         if (LLM_Feedback != "")
         {  Serial.print( "\n[SEARCH]");
            RealtimeAPI_AddUserText(LLM_Feedback);
         }
      }
  }

  // ------ Check FRIEND change (calling new friends alias names via Realtime STT or keyboard)
  if (UserRequest != "")
  {  if (RealtimeAPI_SwitchToCharacterByName(UserRequest))
     {  String new_friend_name = RealtimeAPI_CurrentCharacterName();
        Serial.println( "[RT] < REALTIME CHARACTER: Restarting session with [" + new_friend_name + "] >" );
        RealtimeAPI_RestartForCurrentCharacter(OPENAI_KEY, 10000);
        RealtimeAPI_AddUserText(UserRequest);  // Trick: repeat earlier User request for NEW friend
     }
  }

  // ------ REALTIME LLM & TTS Main Task -> Lauching Realtime LLM + AUDIO OUT (TTS) + LLM Text response
  if (UserRequest != "")
  {  String friend_name  = RealtimeAPI_CurrentCharacterName();
     String friend_voice = RealtimeAPI_CurrentCharacterVoice();
     Serial.print( "[RT] Livestream [" + friend_name + "|" + friend_voice + "] Live Stream> ");

     led_RGB(HIGH,HIGH,LOW); // #BLUE = speaking
     RealtimeAPI_StartLLM_GetAudio( 45000, true); /* true: Print LiveTranscript */  // LLM & Audio SPEAKING (blocking until done)
     Serial.println();

     String aiText = RealtimeAPI_GetAudioTranscription();                           // LLM Text Repeat [optional]
     aiText.replace(". ",". \n");  // CRLF for better readiability
     Serial.println( "[RT] LLM [" + friend_name + "]> [" + aiText + "]" );
     UserRequest = "";
  }

  // ------ finally update LED status (on each loop)
  {  if ( RealtimeAPI_isListening()) {led_RGB(LOW,HIGH,HIGH);}     // #RED:   automated user audio recording (VAD open)
     if (!RealtimeAPI_isListening()) {led_RGB(LOW,LOW, LOW );}     // #WHITE: default Ready & waiting in REALTIME
     /* RealtimeAPI_StartLLM_GetAudio (see code above)                #BLUE:  during AI response (see code above) */
  }
}

// end of LOOP() ****************************************************************************************************************



// Helper functions:

// ------------------------------------------------------------------------------------------------------------------------------
// Updating LED with RGB colors [optional]
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
// Keyword_Commands(String UserRequest): Parsing UserRequest and launching ANY user defined ACTIONS
// ------------------------------------------------------------------------------------------------------------------------------

bool Keyword_Commands( String UserRequest )
{
  String cmd = UserRequest;
  cmd.toUpperCase(); cmd.replace(".", "");

  // Examples:

  if (cmd == "RESET")                      { ESP.restart();  }
  if (cmd.indexOf("DEBUG ON")  >=0)        { Serial.println( "< DEBUG ON >");  DEBUG = true;  return (true); }
  if (cmd.indexOf("DEBUG OFF") >=0)        { Serial.println( "< DEBUG OFF >"); DEBUG = false; return (true); }
  if (cmd.indexOf("ROOM LIGHTS ON") >=0)   { Serial.println( "< ... LIGHT example ...>"); /*local action*/ return (true); }

  /* add any user favorite actions here ... */

  return (false);
}


// ------------------------------------------------------------------------------------------------------------------------------
// ----------------       KALO Library - SpeechToText (STT) transcription library (for pre-recorded audio)       ---------------- 
// ----------------                          via DEEPGRAM API - or - ELEVENLABS API                              ---------------- 
// ----------------                               Latest Update: June 28, 2025                                   ----------------
// ----------------                                                                                              ----------------   
// ----------------                                       Coded by KALO                                          ----------------
// ----------------                  Thanks to @Sandra from Deepgram Developer team (June 2024)                  ---------------- 
// ----------------                             and @palashis92 for ElevenLabs idea                              ----------------
// ----------------                                                                                              ---------------- 
// ----------------     Workflow: sending binary AUDIO data (wav file stored on SD Card or in RAM/PSRAM)         ----------------
// ----------------       via https POST message request, receiving transcription text as String                 ----------------   
// ----------------                                                                                              ----------------   
// ----------------                              [no Initialization needed]                                      ----------------
// ----------------                  [Prerequisites: DEEPGRAM or ELEVENLABS API KEY needed]                      ----------------
// ------------------------------------------------------------------------------------------------------------------------------


// = Some background info / my lessons learned during testing = 
//
// GENERAL comment to Speech-To-Text (STT) services (valid for Deepgram & Elevenlabs Functions):
// - STT services are AI based!, means 'whole sentences with context' have always a higher accuracy (correct word detection rate)
//   than short sentences without context, in particular if STT works MULTI lingual (automated language detection). Simple reason:
//   A short 'Hi' sentence or does not help the STT in recognizing user language. Tipp: use human sentences, not words only!.
// - Most STT allow switching off the MULTI lingual capabilities (using the 'language' parameter), keep in mind: this might
//   increase performance (and better accuracy), but it kills the capability to transcribe other language, also sentences
//   with mixed languages can’t be described correctly. I personally avoid 'language' parameter to keep enable full features
// - both functions below SpeechToText_ElevenLabs() & SpeechToText_Deepgram() have same parameter´
// - both STT need an personalized API key, registration needed, hint: FREE of-cost account are working well for daily usage
//
// DEEPGRAM vs. ELEVENLABS - major differences (my lessons learned):
// - ElevenLabs reacts significantly faster than Deepgram !, in particular on long sentences, that's why i use it most times.
//   Background: The latency for waiting server response are fast on both (typically  < 1 sec) .. the limiting factor seems
//   the time for sending all binary WAV data to server. Seems like the ESP32 <WiFiClientSecure.h> websockets handshake to 
//   ElevenLabs is 2-10 x faster than on Deepgram connections. Maybe a <WiFiClientSecure.h> issue (not an STT), i don't know.
// - MULTI lingual detection: Deepgram latest model (using Nova-3) has a well working word detection (much better than earlier
//   Nova-2 model), language detection working well. The Elevenlabs feels still better in word accuracy (detecting dialects and
//   slang sentences), also detecting much more languages, but sometimes detects a 'wrong' language which ends in total wrong 
//   transcription (my mitigation: using long sentences on Elevenlabs!). So both have Pro' and Cons.
// - MONO lingual mode (Deepgram): with parameter 'language' user can force STT to one dedicated language (to avoid any wrong
//   language detection). This allows best correct word detection (even on short words like 'Hi'), this might be important for 
//   automated IoT workflows (using short commands). The older (!) model Nova-2 has a strength here, because much more 
//   languages are supported than in latest Nova-3. That’s why the Deepgram function below toggles between 2 models. 
// - MONO lingual mode (ElevenLabs) fails often with parameter 'language' (at least with scribe v1 model), it does NOT force to  
//   one language, it triggers more a translation to 'user' language (something totally different). So be careful with 'language'
//   parameter in ElevenLabs (i don't use it, keeping the MULTI lingual features for about 100 languages feels better for me)
// - minor difference (might be of interest): ElevenLabs allows to keep websocket connection open (no auto-close)


// --- includes ----------------

/* #include <WiFiClientSecure.h>  // library needed, but already included in main.ino tab */
 
/* #include <SD.h>                // library needed, but already included in main.ino tab */


// --- defines & macros --------

/* needed, but already defined in main.ino tab:
#ifndef DEBUG                     // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG false             // <- define your preference here [true activates printing INFO details]  
#  define DebugPrint(x);          if(DEBUG){Serial.print(x);}   // do not touch
#  define DebugPrintln(x);        if(DEBUG){Serial.println(x);} // do not touch 
#endif */


// --- user preferences --------  

#define TIMEOUT_STT         8     // max. waiting time [sec] for STT Server transcription response (after wav sent), e.g. 8 sec. 
                                  // typical response latency is much faster. TIMEOUT is primary used if server connection lost
                                  


// ------------------------------------------------------------------------------------------------------------------------------
// SpeechToText_ElevenLabs( String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key )
// ------------------------------------------------------------------------------------------------------------------------------
// - Sending pre-recorded AUDIO WAV via https POST message to ElevenLabs SpeechToText (STT) server, receiving transcription text
// - Function supports 2 audio source locations: WAV file on SD Card (wav files) - OR - Binary data stored in RAM/PSRAM
// - current used model: "scribe_v1", server endpoint: "api.elevenlabs.io" 
//
// CALL:     Call function on demand at any time, no Initializing needed (function initializes/connects/closes websocket)
// Params:   - String audio_filename: Filename of audio file on SD Card | or empty "" if source is RAM/PSRAM buffer 
//           - uint8_t* PSRAM:        Pointer to PSRAM buffer (including wav header) | or NULL if audio source is file on SD Card 
//           - long PSRAM_length:     Amount of audio bytes in PSRAM | ignored if audio source is file on SD Card    
//           - String language:       ISO 639 codes. Recommendation: keep empty ! [default] to keep ElevenLabs powerfull 
//                                    MULTI-lingual capabilities enabled (predicting language automatically, ~ 100 languages !)
//           - const char* API_Key:   ELEVENLABS KEY, registration (free account) 
// RETURN:   String transcription (function is waiting until server result received or TIMEOUT passed)
//
// Examples: Serial.println( SpeechToText_ElevenLabs( "/Audio.wav", NULL, 0,  "", "my..KEY" ));  <- transcribe SD card file
//           String text = SpeechToText_ElevenLabs( "", PSRAM_buffer, bytes, "", "my..KEY" );    <- transcribe wav data buffer
//           etc ... 
//
// Links:    - API Reference (Create Transcript):       https://elevenlabs.io/docs/api-reference/speech-to-text/convert
//           - ElevenLabs Models (see Scribe v1):       https://elevenlabs.io/docs/models
//           - ElevenLabs Pricing (see Speech-To-Text): https://elevenlabs.io/de/pricing#pricing-table (e.g. register 0$ free)


String SpeechToText_ElevenLabs( String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key )
{ 
  static WiFiClientSecure client;     // websocket handle (no global var needed)
                                      // using static (allows to keep session open in case 'client.stop()' below removed)
  uint32_t t_start = millis(); 
  
  
  // ---------- Connect to ElevenLabs Server (only if needed, e.g. on INIT or after .close() and after lost connection)

  const char* STT_ENDPOINT = "api.elevenlabs.io";
  
  if ( !client.connected() )
  { DebugPrintln("> Initialize ELEVENLABS Server connection ... ");
    client.setInsecure();
    if (!client.connect(STT_ENDPOINT, 443)) 
    { Serial.println("\n* ERROR - WifiClientSecure connection to Server failed!");
      client.stop(); /* might not have any effect, similar with client.clear() */
      return ("");   // in rare cases: WiFiClientSecure freezed (with older libraries) 
    }
    DebugPrintln("Done. Connected to ELEVENLABS Server.");
  }
  uint32_t t_connected = millis();  

        
  // ---------- Check if AUDIO file exists, check file size (NEW: PSRAM supported)

  size_t audio_size;
  if ( PSRAM != NULL && PSRAM_length > 0)                
  {  audio_size = PSRAM_length; 
  }
  else
  {  File audioFile = SD.open( audio_filename );    
     audio_size = audioFile.size();
     audioFile.close();
     DebugPrintln( "> Audio File [" + audio_filename + "] found, size: " + (String) audio_size ); 
  }

  if (audio_size == 0)   // error handling: if nothing found at all .. then return empty string. DONE.
  {  DebugPrintln( "* ERROR - No AUDIO data for transcription found!");
     return ("");
  }

  
  // ---------- PAYLOAD - HEADER: Building multipart/form-data HEADER and sending via HTTPS request to ElevenLabs Server
  
  // API Reference (Create Transcript): https://elevenlabs.io/docs/api-reference/speech-to-text/convert
  // [model] (mandatory):            >> using model 'scribe_v1'
  // [tag_audio_events] (optional):  if true (default) adding audio events into transcript, e.g. (laughter), (footsteps)
  // [language_code] (optional):     ISO-639-1/ISO-639-3 language_code. Recommended to keep empty in Elevnlabs (see above)
  
  String payload_header;
  String payload_end;
  String boundary =  "---011000010111000001101001";
  String bond     =  "--" + boundary + "\r\n" + "Content-Disposition: form-data; ";  
   
  payload_header  =  bond + "name=\"model_id\"\r\n\r\n" + "scribe_v1\r\n";                             
  payload_header  += bond + "name=\"tag_audio_events\"\r\n\r\n" + "false\r\n";                         
  payload_header  += (language != "")? (bond + "name=\"language_code\"\r\n\r\n" + language + "\r\n") : ("");  
  payload_header  += bond + "name=\"file\"; filename=\"audio.wav\"\r\n" + "Content-Type: audio/wav\r\n\r\n"; 
  /* <= ... Hint: Binary audio data will be send here (between payload_header and payload_end) */
  payload_end     =  "\r\n--" + boundary + "--\r\n";
  
  /* DebugPrintln( "\n> PAYLOAD (multipart/form-data:\n" + payload_header + "... AUDIO DATA ..." + payload_end ); */
  
  size_t total_length = payload_header.length() + audio_size + payload_end.length();    // total PAYLOAD length
  
  // ---------- Sending  HTTPS POST request header ---
  client.println("POST /v1/speech-to-text HTTP/1.1");
  client.println("Host: " + (String) STT_ENDPOINT);
  client.println("xi-api-key: " + String (API_Key));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(total_length));
  client.println();
  
  // ---------- Sending PAYLOAD
  client.print(payload_header);


  // ---------- PAYLOAD - DATA: Sending binary AUDIO data (from SD Card or PSRAM) to ElevenLabs Server
  
  DebugPrintln("> POST Request to ELEVENLABS Server started, sending WAV data now ..." );
  
  // ---------- Option 1: Sending WAV from PSRAM buffer
  
  if (PSRAM != NULL && PSRAM_length > 0)
  {
     // observation: client.write( PSRAM, PSRAM_length ); .. unfortunaltly NOT working -> smaller CHUNKS needed !
     // Writing binary data above 16383 bytes in 1 block will be rejected (maybe a handshake WiFiClientSecure.h issue ?)
     // workaraound: sending WAV in chunks
     
     size_t blocksize   = 16383;     // found by try&error, 16383 (14 bit max value) seems a hard coded max. limit
     size_t chunk_start = 0;
     size_t result_bytes_sent;
  
     while ( chunk_start < PSRAM_length ) 
     { if ( (chunk_start + blocksize) > PSRAM_length )   
       {  blocksize = PSRAM_length - chunk_start;  // at eof buffer (rest)
       }
       result_bytes_sent = client.write( PSRAM + chunk_start, blocksize );  // sending in chunks
       DebugPrintln( "  Send Bytes: " + (String) blocksize + ", Confirmed: " + (String) result_bytes_sent );  
       chunk_start += blocksize;
     }
     DebugPrintln( "> All WAV data in PSRAM sent [" + (String) PSRAM_length + " bytes], waiting transcription" );    
  }
 
  // ---------- Option 2: Sending WAV from SD CARD (only if Option 1 not launched)
  
  if ( audio_filename != "" && !(PSRAM != NULL && PSRAM_length > 0) )
  {  
     // Reading the AUDIO wav file, sending in CHUNKS (closing file after done)
     // idea found here (WiFiClientSecure.h issue): https://www.esp32.com/viewtopic.php?t=4675
  
     File file = SD.open( audio_filename, FILE_READ );
     const size_t bufferSize = 1024;   // best values seem anywhere between 1024 and 2048; 
     uint8_t buffer[bufferSize];
     size_t bytesRead;
     while (file.available()) 
     { bytesRead = file.read(buffer, sizeof(buffer));
       if (bytesRead > 0) { client.write(buffer, bytesRead); }  // sending WAV AUDIO data       
     }
     file.close();
     DebugPrintln( "> SD Card file '" + (String) audio_filename + "' sent to STT server" );
     DebugPrintln( "> All bytes sent, waiting transcription");    
  }    
  
  // ---------- PAYLOAD - END:  Sending final bondery to complete multipart/form-data
  client.print( payload_end );  

  uint32_t t_wavbodysent = millis();  
  

  // ---------- Waiting (!) to ElevenLabs Server response (stop waiting latest after TIMEOUT_STT [secs])
 
  String response = "";   // waiting until available() true and all data completely received
  while ( response == "" && millis() < (t_wavbodysent + TIMEOUT_STT*1000) )   
  { while (client.available())                         
    { char c = client.read();
      response += String(c);      
    }
    // printing dots '.' e.g. each 250ms while waiting (always, also if DEBUG false)
    Serial.print(".");                   
    delay(250);                                 
  } 
  Serial.print(" ");  // final space at end (for better readability)
  
  if (millis() >= (t_wavbodysent + TIMEOUT_STT*1000))
  { Serial.print("\n*** TIMEOUT ERROR - forced TIMEOUT after " + (String) TIMEOUT_STT + " seconds");
    Serial.println(" (is your ElevenLabs API Key valid ?) ***\n");    
  } 
  uint32_t t_response = millis();  


  // ---------- closing connection to ElevenLabs 
  // ElevenLabs supports a 'nice' feature: websockets will NOT be closed automatically (at least for several minutes),
  // - would allow to keep connection OPEN for next requests. Result: increased performance, no connect needed (~0.5 sec faster)
  // - side effect: other WiFiClientSecure sockets might be less reliable (e.g. on Open AI TTS or Open AI LLM) without PSRAM
  // - Remove the client.stop() if performance is most important .. we keep it for best reliability (e.g. if no PSRAM available)
  // - Tip: can also be removed if just WAV Recording & STT transcrition needed (without Open AI LLM or TTS) -> fastest STT ! ;)
  
  client.stop();  
                     
    
  // ---------- Parsing json response, extracting transcription etc.
  
  /* Comment to return value 'detected languages' in older Nova-2 model:
  MULTI lingual (not used here): tag is labeled with 'detected_language' (instead 'languages')
  MONO  lingual mode: no language tag at all */
  
  int    response_len     = response.length();
  
  String transcription    = json_object( response, "\"text\":" );
  String detect_language  = json_object( response, "\"language_code\":" );    
  /* String wavduration   = json_object( response, "\"duration\":" ); */

  DebugPrintln( "\n---------------------------------------------------" );
  /* DebugPrintln( "-> Total Response: \n" + response + "\n");  // ## uncomment to see the complete server response ##   */
  DebugPrintln( "ELEVENLABS: [scribe_v1] - " + ((language == "")? ("MULTI lingual") : ("MONO lingual [" + language +"]")) )
  DebugPrintln( "-> Latency Server (Re)CONNECT [t_connected]:   " + (String) ((float)((t_connected-t_start))/1000) );   
  DebugPrintln( "-> Latency sending WAV file [t_wavbodysent]:   " + (String) ((float)((t_wavbodysent-t_connected))/1000) );   
  DebugPrintln( "-> Latency ELEVENLABS response [t_response]:   " + (String) ((float)((t_response-t_wavbodysent))/1000) );   
  DebugPrintln( "=> TOTAL Duration [sec]: ..................... " + (String) ((float)((t_response-t_start))/1000) ); 
  /*DebugPrintln( "=> Server detected audio length [sec]: " + wavduration ); // json object does not exist in ElevenLabs */
  DebugPrintln( "=> Server response length [bytes]: " + (String) response_len );
  DebugPrintln( "=> Detected language (optional): [" + detect_language + "]" );  
  DebugPrintln( "=> Transcription: [" + transcription + "]" );
  DebugPrintln( "---------------------------------------------------\n" );

  
  // ---------- return transcription String 
  return transcription;    
}



// ------------------------------------------------------------------------------------------------------------------------------
// SpeechToText_Deepgram( String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key )
// ------------------------------------------------------------------------------------------------------------------------------
// - Sending pre-recorded AUDIO WAV via https POST message to Deepgram SpeechToText (STT) server, receiving transcription text
// - Function supports 2 audio source locations: WAV file on SD Card (wav files) - OR - Binary data stored in RAM/PSRAM
// - current used model: "nova-3-general" (MULTI lingual) and "nova-2-general" (if language), server endpoint: "api.deepgram.com" 
//
// CALL:     Call function on demand at any time, no Initializing needed (function initializes/connects/closes websocket)
// Params:   - String audio_filename: Filename of audio file on SD Card | or empty "" if source is RAM/PSRAM buffer 
//           - uint8_t* PSRAM:        Pointer to PSRAM buffer (including wav header) | or NULL if audio source is file on SD Card 
//           - long PSRAM_length:     Amount of audio bytes in PSRAM | ignored if audio source is file on SD Card    
//           - String language:       Keep empty "" MULTI-lingual (Server detects language automatically) or define dedicated
//                                    MONO lingual language code. Function toggles from Nova-3 to Nova-2 model to support about
//                                    ~ 40 languages, Nova-2: https://developers.deepgram.com/docs/models-languages-overview
//                                    (Nova-3 model with -multi language is NOT used, because only few languages supported)
//           - const char* API_Key:   ELEVENLABS KEY, registration (free account) https://elevenlabs.io/pricing#pricing-table
// RETURN:   String transcription (function is waiting until server result received or TIMEOUT passed)
//
// Examples: Serial.println( SpeechToText_Deepgram( "/Audio.wav", NULL, 0,  "", "my..KEY" ));  <- transcribe SD card file
//           String text = SpeechToText_Deepgram( "", PSRAM_buffer, bytes, "", "my..KEY" );    <- transcribe wav data buffer
//           etc ... 
//
// Links:    - API Reference (STT pre-recoded Audio): https://developers.deepgram.com/reference/speech-to-text-api/listen
//           - Models & language overview: https://developers.deepgram.com/docs/models-languages-overview
//           - STT parameter / toggles:  https://developers.deepgram.com/docs/stt-streaming-feature-overview


String SpeechToText_Deepgram( String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key )
{ 
  static WiFiClientSecure client;     // websocket handle (no global var needed)
                                      // using static in case we ever decide again to remove 'client.stop()' at end
  uint32_t t_start = millis(); 

  
  // ---------- Connect to Deepgram Server (only if needed, e.g. on INIT or after .close() and after lost connection)

  const char* STT_ENDPOINT = "api.deepgram.com";
    
  if ( !client.connected() )
  { DebugPrintln("> Initialize DEEPGRAM Server connection ... ");
    client.setInsecure();
    if (!client.connect(STT_ENDPOINT, 443)) 
    { Serial.println("\n* ERROR - WifiClientSecure connection to Server failed!");
      client.stop(); /* might not have any effect, similar with client.clear() */
      return ("");   // in rare cases: WiFiClientSecure freezed (with older libraries) 
    }
    DebugPrintln("Done. Connected to DEEPGRAM Server.");
  }
  uint32_t t_connected = millis();  

        
  // ---------- Check if AUDIO file exists, check file size (NEW: PSRAM supported)

  size_t audio_size;
  if ( PSRAM != NULL && PSRAM_length > 0)                
  {  audio_size = PSRAM_length; 
  }
  else
  {  File audioFile = SD.open( audio_filename );    
     audio_size = audioFile.size();
     audioFile.close();
     DebugPrintln( "> Audio File [" + audio_filename + "] found, size: " + (String) audio_size ); 
  }

  if (audio_size == 0)   // error handling: if nothing found at all .. then return empty string. DONE.
  {  DebugPrintln( "* ERROR - No AUDIO data for transcription found!");
     return ("");
  }

  
  // ---------- Define Model & Language settings
  // Deepgram language settings are model depended, so we toggle model to combine best of both models:
  // Nova-3: MULTI lingual (auto-detecting user language), high performance, high word recognition rate
  // Nova-2: MONO lingual mode: e.g. "en", "en-US", "en-IN", "de" (supporting ~40 languages vs. 10 languages in Nova-3)
  
  String model_name, model_lang_param, model_lang_desc;
  
  if (language == "") // default: Deepgram understands multiple languages
  {  model_name       = "nova-3-general";    // latest and most powerful model (status May 2025) - MULTI lingual
     model_lang_param = "&language=multi";   // mandatory parameter ! (in Nova-2 using "&detect_language=true" instead) 
     model_lang_desc  = "MULTI lingual";     // Deepgram can understand & transcribe multiple (and mixed) languages     
  }
  else                // used in case user needs one dedicated (rare) language
  {  model_name       = "nova-2-general";                  // older model, MONO lingual mode, ++ supporting many languages 
     model_lang_param = "&language=" + language;           // MULTI lingual mode fail too often (using "&detect_language=true")
     model_lang_desc  = "MONO lingual [" + language +"]";  // > so we use MONO lingual only (if user defined 'his' language)         
  }
 
  
  // ---------- PAYLOAD - HEADER: Sending HTTPS request to Deepgram Server
  
  String optional_param;                           
  optional_param =  "?model=" + model_name;  // example: "?model=nova-3" or "?model=nova-2"
  optional_param += model_lang_param;        // example Nova3: "&language=multi", example Nova2: "&language=en-US"
  optional_param += "&smart_format=true";    // applies formatting (Punctuation, Paragraphs, upper/lower etc ..) 
                                             // info: https://developers.deepgram.com/docs/stt-streaming-feature-overview */
  optional_param += "&numerals=true";        // converts numbers to 123 format (Nova-2 always, Nova3-multi in 'en' only)
                                             // info: https://developers.deepgram.com/docs/smart-format
  /* optional_param += model_keywords;       // keywords (and 'keyterm'!) no longer supported in Nova-3 in -multi mode */
  
  client.println("POST /v1/listen" + optional_param + " HTTP/1.1"); 
  client.println("Host: " + (String) STT_ENDPOINT);
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String (API_Key));
  client.println("Content-Type: audio/wav");
  client.println("Content-Length: " + String(audio_size));        
  client.println();   // header complete, now sending binary body (wav bytes) .. 
  

  // ---------- PAYLOAD - DATA: Sending binary AUDIO data (from SD Card or PSRAM) to Deepgram Server
  
  DebugPrintln("> POST Request to DEEPGRAM Server started, sending WAV data now ..." );

  // ---------- Option 1: Sending WAV from PSRAM buffer
  
  if (PSRAM != NULL && PSRAM_length > 0)
  {
     // observation: client.write( PSRAM, PSRAM_length ); .. unfortunately NOT working -> smaller CHUNKS needed !
     // Writing binary data above 16383 bytes in 1 block will be rejected (maybe a handshake WiFiClientSecure.h issue ?)
     // workaraound: sending WAV in chunks    
  
     size_t blocksize =   16383;   // found by try&error, 16383 (14 bit max value) seems a hard coded max. limit
     size_t chunk_start = 0;
     size_t result_bytes_sent;
  
     while ( chunk_start < PSRAM_length ) 
     { if ( (chunk_start + blocksize) > PSRAM_length )   
       {  blocksize = PSRAM_length - chunk_start;  // at eof buffer (rest)
       }
       result_bytes_sent = client.write( PSRAM + chunk_start, blocksize );  // sending in chunks
       DebugPrintln( "  Send Bytes: " + (String) blocksize + ", Confirmed: " + (String) result_bytes_sent );  
       chunk_start += blocksize;
     }
     DebugPrintln( "> All WAV data in PSRAM sent [" + (String) PSRAM_length + " bytes], waiting transcription" );    
  }

  // ---------- Option 2: Sending WAV from SD CARD (only if Option 1 not launched)
  
  if ( audio_filename != "" && !(PSRAM != NULL && PSRAM_length > 0) )
  {  
     // Reading the AUDIO wav file, sending in CHUNKS (closing file after done)
     // idea found here (WiFiClientSecure.h issue): https://www.esp32.com/viewtopic.php?t=4675
  
     File file = SD.open( audio_filename, FILE_READ );
     const size_t bufferSize = 1024;   // best values seem anywhere between 1024 and 2048; 
     uint8_t buffer[bufferSize];
     size_t bytesRead;
     while (file.available()) 
     { bytesRead = file.read(buffer, sizeof(buffer));
       if (bytesRead > 0) { client.write(buffer, bytesRead); }  // sending WAV AUDIO data       
     }
     file.close();
     DebugPrintln( "> SD Card file '" + (String) audio_filename + "' sent to STT server" );
     DebugPrintln( "> All bytes sent, waiting transcription");    
  }  

  uint32_t t_wavbodysent = millis();  
  

  // ---------- Waiting (!) to Deepgram Server response (stop waiting latest after TIMEOUT_STT [secs])
 
  String response = "";   // waiting until available() true and all data completely received
  while ( response == "" && millis() < (t_wavbodysent + TIMEOUT_STT*1000) )   
  { while (client.available())                         
    { char c = client.read();
      response += String(c);      
    }
    // printing dots '.' e.g. each 250ms while waiting (always, also if DEBUG false)
    Serial.print(".");                   
    delay(250);                                 
  } 
  Serial.print(" ");  // final space at end (for better readability)
  
  if (millis() >= (t_wavbodysent + TIMEOUT_STT*1000))
  { Serial.print("\n*** TIMEOUT ERROR - forced TIMEOUT after " + (String) TIMEOUT_STT + " seconds");
    Serial.println(" (is your Deepgram API Key valid ?) ***\n");    
  } 
  uint32_t t_response = millis();  


  // ---------- closing connection to Deepgram 
  // Depgram does NOT support to keep connection open on PRE-RECORDED audio (Auto Close after ~ 10 secs).
  // So we close always for best reliability (avoiding potential side effects to other WiFiClientSecure sockets (e.g. OpenAI TTS)
  
  client.stop();     
                     
    
  // ---------- Parsing json response, extracting transcription etc.
  
  /* Comment to return value 'detected languages' in older Nova-2 model:
  MULTI lingual (not used here): tag is labeled with 'detected_language' (instead 'languages')
  MONO  lingual mode: no language tag at all */
  
  int    response_len     = response.length();
  String transcription    = json_object( response, "\"transcript\":" );
  String detect_language  = json_object( response, "\"languages\":" );    
  String wavduration      = json_object( response, "\"duration\":" );

  DebugPrintln( "\n---------------------------------------------------" );
  /* DebugPrintln( "-> Total Response: \n" + response + "\n");  // ## uncomment to see the complete server response ## */  
  DebugPrintln( "DEEPGRAM: [" + model_name + "] - " + model_lang_desc );  
  DebugPrintln( "-> Latency Server (Re)CONNECT [t_connected]:   " + (String) ((float)((t_connected-t_start))/1000) );   
  DebugPrintln( "-> Latency sending WAV file [t_wavbodysent]:   " + (String) ((float)((t_wavbodysent-t_connected))/1000) );  
  DebugPrintln( "-> Latency DEEPGRAM response [t_response]:     " + (String) ((float)((t_response-t_wavbodysent))/1000) );   
  DebugPrintln( "=> TOTAL Duration [sec]: ..................... " + (String) ((float)((t_response-t_start))/1000) ); 
  DebugPrintln( "=> Server detected audio length [sec]: " + wavduration );
  DebugPrintln( "=> Server response length [bytes]: " + (String) response_len );
  DebugPrintln( "=> Detected language (optional): " + detect_language );  
  DebugPrintln( "=> Transcription: [" + transcription + "]" );
  DebugPrintln( "---------------------------------------------------\n" );

  
  // ---------- return transcription String 
  return transcription;    
}



// ------------------------------------------------------------------------------------------------------------------------------
// JSON String Extract: Searching [element] in [input], example: element = '"text":' -> returns content 'How are you?'
// ------------------------------------------------------------------------------------------------------------------------------

String json_object( String input, String element )
{ String content = "";
  int pos_start = input.indexOf(element);      
  if (pos_start > 0)                                      // if element found:
  {  pos_start += element.length();                       // pos_start points now to begin of element content     
     int pos_end = input.indexOf( ",\"", pos_start);      // pos_end points to ," (start of next element)  
     if (pos_end > pos_start)                             // memo: "garden".substring(from3,to4) is 1 char "d" ..
     { content = input.substring(pos_start,pos_end);      // .. thats why we use for 'to' the pos_end (on ," ):
     } content.trim();                                    // remove optional spaces between the json objects
     if (content.startsWith("\""))                        // String objects typically start & end with quotation marks "    
     { content=content.substring(1,content.length()-1);   // remove both existing quotation marks (if exist)
     }     
  }  
  return (content);
}

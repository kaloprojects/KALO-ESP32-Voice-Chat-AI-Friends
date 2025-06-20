
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------            KALO Library - RECORDING audio (variable sample/bit rate, gain booster)           ----------------
// ----------------              and storing as .wav file (with header) on SD Card or [NEW] in PSRAM             ----------------   
// ----------------   [define preferred AUDIO RECORD location via #define RECORD_SPSRAM / RECORD_SDCARD below]   ----------------
// ----------------                               Latest Update: June 19, 2025                                   ----------------
// ----------------                                     Coded by KALO                                            ----------------
// ----------------                                                                                              ---------------- 
// ----------------                     Used pins SD Card: VSPI Default pins 5,18,19,23                          ----------------
// ----------------         Alternative to SD Card: ESP32 with PSRAM (e.g. ESP32 Wrover 2/4/8 MB PSRAM)          ----------------
// ----------------                 Used pins I2S Microphone INMP441: WS 22, SCK 33, SD 35                       ----------------
// ----------------                      Using latest ESP 3.x library (with I2S_std.h)                           ----------------
// ----------------                                                                                              ----------------
// ----------------             Functions: I2S_Recording_Init(), Recording_Loop(), Recording_Stop(..)            ----------------
// ------------------------------------------------------------------------------------------------------------------------------


// --- includes ----------------

#include "driver/i2s_std.h"       // important: older legacy #include <driver/i2s.h> no longer supported 
/* #include <SD.h>                // library needed, but already included in main.ino tab */


// --- defines & macros --------

/* needed, but already defined in main.ino tab:
#ifndef DEBUG                     // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG false             // <- define your preference here [true activates printing INFO details]  
#  define DebugPrint(x);          if(DEBUG){Serial.print(x);}   // do not touch
#  define DebugPrintln(x);        if(DEBUG){Serial.println(x);} // do not touch 
#endif */


// === user settings =========== 

#define RECORD_PSRAM      false   // true: store Audio Recording .wav in PSRAM (ESP32 with PSRAM needed, e.g. ESP32 Wrover)
#define RECORD_SDCARD     true    // true: store Audio Recording .wav on SD card (prerequisite: SD card reader, VSPI pins)
                                  // at least one of both has to be true (or both if needed)
                                  
                                  
// --- recording settings ------

#define AUDIO_FILE        "/Audio.wav"   // mandatory if RECORD_SDCARD is true: filename for the AUDIO recording                                  

#define SAMPLE_RATE       16000   // typical values: 8000 .. 44100, use e.g 8K (and 8 bit mono) for smallest .wav files  
                                  // hint: best quality with 16000 or 24000 (above 24000: random dropouts and distortions)
                                  // recommendation in case the STT service produces lot of wrong words: try 16000 

#define BITS_PER_SAMPLE   8       // 16 bit and 8bit supported (24 or 32 bits not supported)
                                  // hint: 8bit is less critical for STT services than a low 8kHz sample rate
                                  // for fastest STT: combine 8kHz and 8 bit. 

#define GAIN_BOOSTER_I2S  32      // original I2S streams is VERY silent, so we added an optional GAIN booster for INMP441
                                  // multiplier, values: 1-64 (32 seems best value for INMP441)
                                  // 64: high background noise but still working well for STT on quiet human conversations


// --- PINs I2S microphone -----

#define I2S_LR      HIGH          // [NEW]: use HIGH if L/R pin is on Vcc (RIGHT). HIGH is used e.g. in TECHIESMS and LUCA PCB,    
                                  // use [default] LOW if L/R pin of INMP441 is connected to GND (LEFT channel), e.g. ElatoAI                                                                       
#define I2S_WS      22            
#define I2S_SD      35          
#define I2S_SCK     33     

/* --- ElatoAI pin settings ----- // Circuits see here: https://github.com/akdeb/ElatoAI
#define I2S_LR      LOW           // LOW because L/R pin of INMP441 is connected to GND (LEFT channel) 
#define I2S_WS      4            
#define I2S_SD      14           
#define I2S_SCK     1   */
        

// Links of interest:
// SD Card Arduino library info: https://www.arduino.cc/reference/en/libraries/sd/
// SD Card ESP details: https://randomnerdtutorials.com/esp32-microsd-card-arduino/
// Dronebot I2S workshop: https://dronebotworkshop.com/esp32-i2s/ (using old I2S.h)
// Link WAV header: http://soundfile.sapp.org/doc/WaveFormat/
// I2S setting code snippets: https://github.com/espressif/esp-adf/issues/1047 
// Using tabs to organize code with the Arduino IDE: https://www.youtube.com/watch?v=HtYlQXt14zU 

// Code below is based on Espressif API I2S Reference Doc (Latest Master):
// link: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html#introduction


// --- global vars -------------

uint8_t* PSRAM_BUFFER;            // global array for RECORDED .wav (50% of PSRAM via ps_malloc() in I2S_Recording_Init()
                                  // (using 50% only to allow other functions using PSRAM too, e.g. AUDIO.H openai_speech() 
size_t PSRAM_BUFFER_max_usage;    // size of used buffer (50% of PSRAM)
size_t PSRAM_BUFFER_counter = 0;  // current pointer offset position to last recorded byte


// [std_cfg]: KALO I2S_std configuration for I2S Input device (Microphone INMP441), Details see esp32-core file 'i2s_std.h' 

i2s_std_config_t  std_cfg = 
{ .clk_cfg  =   // instead of macro 'I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),'
  { .sample_rate_hz = SAMPLE_RATE,
    .clk_src = I2S_CLK_SRC_DEFAULT,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  },
  // IMPORTANT (NEW): for .slot_cfg we use MACRO in 'i2s_std.h' (to support all ESP32 variants, ESP32-S3 supported too)
  // Datasheet INMP441: Microphone uses PHILIPS format (bc. Data signal has a 1-bit shift AND signal WS is NOT pulse lasting)
  
  .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG( I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO ), 
  
  /* // info: the MACRO above creates this structure on an ESP-32:
  .slot_cfg =   // hint: always using _16BIT because I2S uses 16 bit slots (even in case I2S_DATA_BIT_WIDTH_8BIT used !)
  { .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,  // not I2S_DATA_BIT_WIDTH_8BIT or (i2s_data_bit_width_t) BITS_PER_SAMPLE  
    .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, 
    .slot_mode = I2S_SLOT_MODE_MONO,             // do NOT use STEREO on INMP441 ! (would produce a wrong I2S_STD_SLOT_BOTH)
    .slot_mask = I2S_STD_SLOT_LEFT,              // <- this is WRONG in case RIGHT channel used (if L/R pin connected to Vcc)
                                                 // .. so we update in I2S_Recording_Init() bc. MACRO does not support this
    .ws_width =  I2S_DATA_BIT_WIDTH_16BIT,           
    .ws_pol = false, 
    .bit_shift = true,    // important ! (that's the reason why we need the PHILIPS macro, not the MSB macro) !
    .msb_right = false,   // because WIDTH_16BIT used
    // Update: ESP32-S3 (!) does NOT have a final '.msb_right' ... instead it has 3 other elements at eof structure: 
    // .left_align = true, .big_endian = false, .bit_order_lsb = false,        
  }, */
  
  .gpio_cfg =   
  { .mclk = I2S_GPIO_UNUSED,
    .bclk = (gpio_num_t) I2S_SCK,
    .ws   = (gpio_num_t) I2S_WS,
    .dout = I2S_GPIO_UNUSED,
    .din  = (gpio_num_t) I2S_SD,
    .invert_flags = 
    { .mclk_inv = false,
      .bclk_inv = false,
      .ws_inv = false,
    },
  },
};

// [re_handle]: global handle to the RX channel with channel configuration [std_cfg]
i2s_chan_handle_t  rx_handle;


// [myWAV_Header]: selfmade WAV Header:
struct WAV_HEADER 
{ char  riff[4] = {'R','I','F','F'};                        /* "RIFF"                                   */
  long  flength = 0;                                        /* file length in bytes - 8 [bug fix]       <= calc at end */ 
  char  wave[4] = {'W','A','V','E'};                        /* "WAVE"                                   */
  char  fmt[4]  = {'f','m','t',' '};                        /* "fmt "                                   */
  long  chunk_size = 16;                                    /* size of FMT chunk in bytes (usually 16)  */
  short format_tag = 1;                                     /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM  */
  short num_chans = 1;                                      /* 1=mono, 2=stereo                         */
  long  srate = SAMPLE_RATE;                                /* samples per second, e.g. 44100           */
  long  bytes_per_sec = SAMPLE_RATE * (BITS_PER_SAMPLE/8);  /* srate * bytes_per_samp, e.g. 88200       */ 
  short bytes_per_samp = (BITS_PER_SAMPLE/8);               /* 2=16-bit mono, 4=16-bit stereo (byte 34) */
  short bits_per_samp = BITS_PER_SAMPLE;                    /* Number of bits per sample, e.g. 16       */
  char  dat[4] = {'d','a','t','a'};                         /* "data"                                   */
  long  dlength = 0;                                        /* data length (filelength - 44) [bug fix]  <= calc at end */
} myWAV_Header;


bool flg_is_recording = false;         // only internally used

bool flg_I2S_initialized = false;      // to avoid any runtime errors in case user forgot to initialize
 


// ------------------------------------------------------------------------------------------------------------------------------
// I2S_Recording_Init()
// ------------------------------------------------------------------------------------------------------------------------------
// - Initializes upcoming I2S Recording via I2S microphone, setting I2S ports (via i2s_std.h) and pin assignments 
// - Configures I2S ports and global vars, checking PSRAM and SD card availability, printing HW details in DEBUG mode.
// - Function checks where the recorded AUDIO data will be stored (via global #defines RECORD_PSRAM + RECORD_SDCARD)
// - NEW: Function updates I2S structure 'std_cfg.slot_cfg.slot_mask': mandatory in case RIGHT channel used (I2S_LR on Vcc) 
// CALL:   Function has to be called once (e.g. in setup()) 
// Params: NONE
// RETURN: true: Initializing successful | false: (and ERROR in Serial Monitor) if failed

bool I2S_Recording_Init() 
{  
  // NEW: Updating I2S structure to the correct channel (LEFT and RIGHT supported)
  if (I2S_LR == HIGH) {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;}  // manually updated, not supported via MACRO (LUCA)
  if (I2S_LR == LOW)  {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; }  // I2S default in MONO (STEREO creates wrong 'BOTH')
  
  // Get the default channel configuration by helper macro (defined in 'i2s_common.h')
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  
  i2s_new_channel(&chan_cfg, NULL, &rx_handle);     // Allocate a new RX channel and get the handle of this channel
  i2s_channel_init_std_mode(rx_handle, &std_cfg);   // Initialize the channel
  i2s_channel_enable(rx_handle);                    // Before reading data, start the RX channel first

  // ------ Check hardware prerequisites (PSRAM & SD CARD Reader) & user preferences
  // Printing ESP32 details / links of interest: 
  // - How To Use PSRAM: https://thingpulse.com/esp32-how-to-use-psram/
  // - Using the PSRAM: https://www.upesy.com/blogs/tutorials/get-more-ram-on-esp32-with-psram
  // - PSRAM vs. SPIFFS/LittleFS: https://thingpulse.com/esp32-how-to-use-psram/
    
  DebugPrintln( "> I2S_Recording_Init - Initializing Recording Setup:"     );
  DebugPrintln( "- Sketch Size: " + (String) ESP.getSketchSize()           );
  DebugPrintln( "- Total Heap:  " + (String) ESP.getHeapSize()             );
  DebugPrintln( "- Free Heap:   " + (String) ESP.getFreeHeap()             );
  DebugPrintln( "- STACK Size:  " + (String) getArduinoLoopTaskStackSize() ); 
  DebugPrintln( "- Flash Size:  " + (String) ESP.getFlashChipSize()        );
  DebugPrintln( "- Total PSRAM: " + (String) ESP.getPsramSize()            );
  DebugPrintln( "- Free PSRAM:  " + (String) ESP.getFreePsram()            );     

  if (RECORD_PSRAM)
  { // check if PSRAM exists -> if yes .. ACTION: allocating 50% of available PSRAM for recording buffer
    if (ESP.getFreePsram() > 0 )
    {  PSRAM_BUFFER_max_usage = ESP.getFreePsram() / 2;  
       int max_seconds = (PSRAM_BUFFER_max_usage / (SAMPLE_RATE * BITS_PER_SAMPLE/8));        
       if ( max_seconds < 5 )  // all below ~5 seconds for AUDIO Recording makes less sense
       {  Serial.println("* ERROR - Not enough free PSRAM found!. Stopped."); 
          while(true);   // END (waiting forever)
       }    
       else // all fine, so we allocate PSRAM for recording  
       {  PSRAM_BUFFER = (uint8_t*) ps_malloc(PSRAM_BUFFER_max_usage);      
          DebugPrintln("> ps_malloc(): " + (String) PSRAM_BUFFER_max_usage + " (allocating 50% of free PSRAM for Audio)" );  
          DebugPrintln("> PSRAM maximum audio recording length [sec]: " + (String) max_seconds + "\n");           
       }     
    }
    else
    {  Serial.println("* ERROR - No PSRAM found!. Stopped."); 
       while(true);  // END (waiting forever) 
    }     
  }
  
  if (RECORD_SDCARD)
  { // check if SD card reader and SD card found 
    if (SD.begin()) 
    {  DebugPrintln("> SD CARD detected, used for AUDIO recording\n");       
    }
    else
    {  Serial.println("* ERROR - SD Card initialization failed!. Stopped."); 
       while(true);  // END (waiting forever) 
    }     
  }  

  /* Not used: 
  i2s_channel_disable(rx_handle);                   // Stopping the channel before deleting it 
  i2s_del_channel(rx_handle);                       // delete handle to release the channel resources */
  
  flg_I2S_initialized = true;                       // all is initialized, checked in procedure Recording_Loop()
 
  return flg_I2S_initialized;  
}



// ------------------------------------------------------------------------------------------------------------------------------
// Recording_Loop()
// ------------------------------------------------------------------------------------------------------------------------------
// - Function reads and cleans the I2S input stream continuously, appending received DATA to PSRAM buffer or file on SD Card
// - global #defines (#defines RECORD_PSRAM, RECORD_SDCARD) define where the I2S data will be stored (appended)
// - on 1st call: Audio WAV header will be generated and Recoding starting_PSRAM + RECORD_SDCARD)
// CALL:   - Call function 1st time to START new recording (typically if user is pressing a button)
//         - then call continuously in main loop() to ensure that I2S stream buffer won't overflow during recording
// Params: NONE
// RETURN: true: Recording started successfully | false: (and ERROR in Serial Monitor) if failed

bool Recording_Loop() 
{
  if (!flg_I2S_initialized)     // to avoid any runtime error in case user missed to initialize
  {  Serial.println( "* ERROR in Recording_Loop() - I2S not initialized, call 'I2S_Recording_Init()' missed" );    
     return false;
  }
  
  if (!flg_is_recording)  // entering 1st time -> remove old AUDIO file, create new file with WAV header
  { 
    flg_is_recording = true;
    
    if (RECORD_PSRAM)    
    {  PSRAM_BUFFER_counter = 44;   
       for (int i = 0; i < PSRAM_BUFFER_counter; i++)                 
       {   PSRAM_BUFFER[i] = ((uint8_t*)&myWAV_Header)[i];  // copying each byte of a struct var
       } 
       DebugPrintln("> WAV Header in PSRAM generated, Audio Recording started ... ");
    }
    if (RECORD_SDCARD)     
    {  if (SD.exists(AUDIO_FILE)) 
       {  SD.remove(AUDIO_FILE);  // because we start with a net new file
       }     
       File audio_file = SD.open(AUDIO_FILE, FILE_WRITE);
       audio_file.write((uint8_t *) &myWAV_Header, 44);
       audio_file.close();        
       DebugPrintln("> WAV Header stored on SD card, Audio Recording started ... ");
    }
    // now proceed below (flg_is_recording is true) ....
  }
  
  if (flg_is_recording)  // here we land when recording started already -> task: append record buffer to file
  { 
    // Array to store Original audio I2S input stream (reading in chunks, e.g. 1024 values) 
    int16_t audio_buffer[1024];                // max. 1024 values [2048 bytes] <- for the original I2S signed 16bit stream 
    uint8_t audio_buffer_8bit[1024];           // max. 1024 values [1024 bytes] <- self calculated if BITS_PER_SAMPLE == 8

    // now reading the I2S input stream (with NEW <I2S_std.h>)
    size_t bytes_read = 0;
    i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
    
    size_t values_recorded = bytes_read / 2;   // 1024 (also if 8bit, because I2S 'waste' 16 bit always, see below)
    
    // Optionally: Boostering the very low I2S Microphone INMP44 amplitude (multiplying values with factor GAIN_BOOSTER_I2S)  
    if ( GAIN_BOOSTER_I2S > 1 && GAIN_BOOSTER_I2S <= 64 );    // check your own best values, recommended range: 1-64
    for (int16_t i = 0; i < values_recorded; ++i)             // all 1024 values, 16bit (bytes_read/2) 
    {   audio_buffer[i] = audio_buffer[i] * GAIN_BOOSTER_I2S;  
    }

    // If 8bit requested: Calculate 8bit Mono files (self made because any I2S _8BIT settings in I2S would still waste 16bits)
    // use case: reduce resolution 16->8bit to archive smallest .wav size (e.g. for sending to SpeechToText services) 
    // details WAV 8bit: https://stackoverflow.com/questions/44415863/what-is-the-byte-format-of-an-8-bit-monaural-wav-file 
    // details Convert:  https://stackoverflow.com/questions/5717447/convert-16-bit-pcm-to-8-bit
    // 16-bit signed to 8-bit WAV conversion rule: FROM -32768...0(silence)...+32767 -> TO: 0...128(silence)...256 

    if (BITS_PER_SAMPLE == 8) // in case we store a 8bit WAV file we fill the 2nd array with converted values
    { for (int16_t i = 0; i < ( values_recorded ); ++i)        
      { audio_buffer_8bit[i] = (uint8_t) ((( audio_buffer[i] + 32768 ) >>8 ) & 0xFF); 
      }
    }
    
    /* // Optional (to visualize and validate I2S Microphone INMP44 stream): displaying first 16 samples of each chunk
    DebugPrint("> I2S Rx Samples [Original, 16bit signed]:    ");
    for (int i=0; i<16; i++) { DebugPrint( (String) (int) audio_buffer[i] + "\t"); } 
    DebugPrintln();
    if (BITS_PER_SAMPLE == 8)    
    {   DebugPrint("> I2S Rx Samples [Converted, 8bit unsigned]:  ");
        for (int i=0; i<16; i++) { DebugPrint( (String) (int) audio_buffer_8bit[i] + "\t"); } 
        DebugPrintln("\n");
    } */      

    if (RECORD_PSRAM)    
    {  // Append audio data to PSRAM 
       if (BITS_PER_SAMPLE == 16)  // for each value 2 bytes needed
       {  if (PSRAM_BUFFER_counter + values_recorded * 2 < PSRAM_BUFFER_max_usage)
          {  memcpy ( (PSRAM_BUFFER + PSRAM_BUFFER_counter), audio_buffer, values_recorded * 2 ); 
             PSRAM_BUFFER_counter += values_recorded * 2; 
          }  else { Serial.println("* WARNING - PSRAM full, Recording stopped."); }
       }        
       if (BITS_PER_SAMPLE == 8)  
       {  if (PSRAM_BUFFER_counter + values_recorded < PSRAM_BUFFER_max_usage)
          {  memcpy ( (PSRAM_BUFFER + PSRAM_BUFFER_counter), audio_buffer_8bit, values_recorded ); 
             PSRAM_BUFFER_counter += values_recorded;    
          }  else { Serial.println("* WARNING - PSRAM full, Recording stopped."); }
       }  
    }
    if (RECORD_SDCARD)
    {  // Save audio data to SD card (appending chunk array to file end)
       File audio_file = SD.open(AUDIO_FILE, FILE_APPEND);
       if (BITS_PER_SAMPLE == 16) // 16 bit default: appending original I2S chunks (e.g. 1014 values, 2048 bytes)
       {  audio_file.write((uint8_t*)audio_buffer, values_recorded * 2);   // for each value 2 bytes needed
       }        
       if (BITS_PER_SAMPLE == 8)  // 8bit mode: appending calculated 1014 values instead (1024 bytes, 2048/2) 
       {  audio_file.write((uint8_t*)audio_buffer_8bit, values_recorded);
       }  
       audio_file.close();                 
    }      
  }  
  return true;
}



// ------------------------------------------------------------------------------------------------------------------------------
// Recording_Stop( String* audio_filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec ) 
// ------------------------------------------------------------------------------------------------------------------------------
// - Function STOPs any ongoing recording, finalizing WAV header and returns recording DETAILS via pointer
// - Function has no action if no ongoing recording detected
// CALL:   Call function once on demand when ongoing recording should be stopped and wav stored (e.g. on REC button release)
// Params: Multiple values are returned via pointer (var declaration and instance have to be allocated in calling function!)
//         - String* audio_filename:  Filename of created file on SD Card,  only if RECORD_SDCARD true, "" on RECORD_PSRAM 
//         - uint8_t** buff_start:    Pointer to PSRAM start (ptr to ptr ;) only if RECORD_PSRAM true, NULL on RECORD_SDCARD 
//         - long* audiolength_bytes: Amount of recorded bytes in PSRAM,    only if RECORD_PSRAM true, 0 on RECORD_SDCARD  
//         - float* audiolength_sec:  Duration of recorded Audio in xx.xx seconds (RECORD_SDCARD and RECORD_PSRAM)
// RETURN: true: Recording successfully stopped, WAV header finalized and Audio DATA are ready for further processing 
//         (true ONCE only when done ! .. this allows to keep function without further actions in any loop)

bool Recording_Stop( String* audio_filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec ) 
{
  // Action: STOP recording and finalize recorded wav
  // Do nothing to in case no Record was started, recap: 'false' means: 'nothing is stopped' -> no action at all
  // important because typically 'Record_Stop()' is called always in main loop()  
  
  if (!flg_is_recording) 
  {   return false;   
  }
  
  if (!flg_I2S_initialized)   // to avoid runtime errors: do nothing in case user missed to initialize at all
  {  return false;
  }
  
  // here we land when Recording is active .. 
  // Tasks: 1. finalize WAV file header (final length tags),   2. fill return values (via &pointer), 
  //        3. stop recording (with flg_is_recording = false), 4. return true/done to main loop()
  
  if (flg_is_recording)       
  { 
    flg_is_recording = false;  // important: this is done only here (means after wav finalized we are done)
    
    // init default values 
    *audio_filename = "";
    *buff_start = NULL;      
    *audiolength_bytes = 0;
    *audiolength_sec = 0;
    
    if (RECORD_PSRAM)   
    {  myWAV_Header.flength = (long) PSRAM_BUFFER_counter -  8;  
       myWAV_Header.dlength = (long) PSRAM_BUFFER_counter - 44;  
       // copy each byte of a struct var again:
       for (int i = 0; i < 44; i++)   // same as on init:  
       {   PSRAM_BUFFER[i] = ((uint8_t*)&myWAV_Header)[i];  
       } 

       // return updated values via REFERENCE (pointer):
       *buff_start        = PSRAM_BUFFER;           // comment: buff_start is a pointer TO the pointer of PSRAM ;)
       *audiolength_bytes = PSRAM_BUFFER_counter;
       *audiolength_sec   = (float) (PSRAM_BUFFER_counter-44) / (SAMPLE_RATE * BITS_PER_SAMPLE/8);   

       DebugPrintln("> ... Done. Audio Recording into PSRAM finished.");
       DebugPrintln("> Bytes recorded: " + (String) *audiolength_bytes + ", audio length [sec]: " + (String) *audiolength_sec );
        
       /* // Optional for debugging: Writing the PSRAM content to a 2nd file "AudioPSRAM.wav", printing first chunks
       File control_file = SD.open("/AudioPSRAM.wav", FILE_WRITE);
       control_file.write( PSRAM_BUFFER, PSRAM_BUFFER_counter );
       control_file.close(); 
       Serial.println( "\n# DEBUG: PSRAM content mirrored on SD card [AudioPSRAM.wav]" );
       Serial.println(   "# DEBUG: PSRAM extract [220 bytes, 44 byte wav header in first 2 rows]:\n ");
       for (int i=0; i<220; i++) 
       { Serial.print( PSRAM_BUFFER[i], HEX); Serial.print( "\t"); 
         if ( (i+1)%22 == 0) {Serial.println();}
       } Serial.println(); */
    }
    
    if (RECORD_SDCARD)
    {  
       File audio_file = SD.open(AUDIO_FILE, "r+");   // Do NOT use 'FILE_WRITE' we need a 'r+' !
       /* Blog info: https://github.com/espressif/arduino-esp32/issues/4028, 
       // Reference: https://cplusplus.com/reference/cstdio/fopen/ */
       
       long filesize = audio_file.size();
       /* bug fix: earlier version was wrrong: .flength = filesize; .dlength = (filesize-8) */
       audio_file.seek(0); myWAV_Header.flength = (filesize-8);  myWAV_Header.dlength = (filesize-44); 
       audio_file.write((uint8_t *) &myWAV_Header, 44);
       audio_file.close(); 

       // return updated values via REFERENCE (pointer):
       *audio_filename    = AUDIO_FILE;
       *audiolength_bytes = filesize;
       *audiolength_sec   = (float) (filesize-44) / (SAMPLE_RATE * BITS_PER_SAMPLE/8);   

       DebugPrintln("> ... Done. Audio Recording finished, stored as '" + (String) AUDIO_FILE + "' on SD Card.");
       DebugPrintln("> Bytes recorded: " + (String) *audiolength_bytes + ", audio length [sec]: " + (String) *audiolength_sec ); 
    }
    
    // Record is done (stored either in PSRAM or on SD card)
    flg_is_recording = false;  // important: this is done only here (any next Recording_Stop() calls have no action)
    return true;               // means: telling the main loop that new record is available now 
  }    
}

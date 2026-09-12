
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------                              [NEW] OpenAI Realtime API library                               ----------------
// ----------------           REALTIME mode supports chatting with Open AI without pressing any button           ----------------
// ----------------     [stream INMP441 to Open AI server via WebSocket, return LLM AUDIO & live transcript]     ----------------
// ----------------                                                                                              ----------------
// ----------------                          (C) Original libary coded by @Palahis92                             ----------------
// ----------------                        KALO mods & add-ons are tagged with #KALO#                            ----------------
// ----------------                               Latest Update: Sept. 12, 2026                                  ----------------
// ----------------                                                                                              ----------------
// ----------------             !! Huge SHOUT-OUT and THANK YOU @Palahis92 for this great library !!             ----------------
// ----------------                                                                                              ----------------
// ------------------------------------------------------------------------------------------------------------------------------


// ## IMPORTANT -> Adjust the "user voice volume" to your environment (increase it in noisy env., lower it for quiet voices)
// -> check [REALTIME_LOCAL_VAD_START_LEVEL] & [REALTIME_LOCAL_VAD_KEEP_LEVEL] below. Voice detected is indicated with LED #RED.
// -> Example: in loud environments the LED might #RED ON always -> increase both values.



/* ==== [#KALO#] REALTIME library changes (modifications/add-ons to original code from @Palahis92):

// - [New] Faster Open AI Realtime response (answer latency reduced)
// - [New] Model 'gpt-realtime-2.1-mini' (same costs) for more natural & emotional LLM response
// - [New] Volume controls enabled for REALTIME Audio (supporting optional VOL Poti or VOL button)
// - [New] New function 'RealtimeAPI_GetAudioTranscription()' returns earlier LLM feedback as TXT
// - [New] LIVE letter transcription (continuous scrolling) via Serial.print & printToDisplay during REALTIME speaking
// - [New] STOP Audio at any time via button (during REALTIME speaking), also allows to stop (earlier rare) audio artefacts
// - [New] function 'RealtimeAPI_DeleteLastConversationItem()' to remove last user request from conversation (for key commands)
// - [New] Older ESP32 (without S3, PSRAM, SD card) now supported, Lib works flawless (same fast & reliable) as on ESP32-S3

// - [Bug fix ECHO]:      Repeating word/vocal/syllables -> Solved
// - [Bug fix VOD]:       Missed Auto Commit after VOD closed (rec. done) solved -> Solved, AI responses asap on each request
// - [Bug fix STEREO]:    Artefact issues with older I2S DAC/Amp (ESP-IDF)libraries -> Solved with I2S STEREO channels
// - [Bug fix STEREO]:    Tube-like sound artefacts on older ESP32 -> Solved (moved buggy I2S MONO Audio framing to STEREO)
// - [Bug fix AUDIOLEAK]: [Complex] Audio OUT continues after RT_GetAudio() finished. [WHITE LED instead BLUE during speaking]
//                        -> Solved [1. RTAudioOut_Stop() on eventType == "error", 2. realtime_waiting_response check in loop()]

// - [CleanUp] RealtimeAPI_WebSocketEvent() rewritten for better readability (Allman/K&R style, no complex 'else if' chains)
// - [CleanUp] I2S Recording function included for stand-alone use (commented out here as lib_audio_recording.ino included)
// - [CleanUp] RealtimeAPI_GetAssistantAudioReply() renamed to -> RealtimeAPI_StartLLM_GetAudio() for better action description
// - [CleanUp] Removed ALL Audio.H dependecies in library (no longer any audio_play.XY) bc. responsibility moved to main.ino
// - [CleanUp] RestoreAudioOutputForAudioH() renamed to 'RealtimeAPI_RestoreAudioH()' bc. called 'IN' RealtimeAPI..GetAudio()
// - [CleanUp] RealtimeAPI_RestoreAudioH() located in Main.ino, WEAK defines OPTIONAL (compile/run even if not exists
// - [CleanUp] new WEAK function [printToDisplay()] declaration <- so it works with any main.ino (with OR without this function)
// - [CleanUp] new WEAK function [led_RGB] declaration added    <- so it works with any main.ino (with OR without this function)
// - [CleanUp] new WEAK function [RealtimeAPI_RestoreAudioH()]  <- so it works with any main.ino (with OR without this function)

// - TAB [KALO_ESP32_Voice_Chat_AI_Friends.ino]: Main loop() divided into smaller sections, INPUT_REALTIME_MODE in one place
// - TAB [KALO_ESP32_Voice_Chat_AI_Friends.ino]: Existing KEYWORDS (Radio etc.) now supported in INPUT_REALTIME_MOD mode too
// - TAB [KALO_ESP32_Voice_Chat_AI_Friends.ino]: INTEGRATED Web Search -> embedded INTO Realtime conversation

// ==== All KALO made change details are tagged with #KALO# in code below */



// --- defines & includes ------

#include <WebSocketsClient.h>
#include "mbedtls/base64.h"
#include "driver/i2s_std.h"

#define REALTIME_HOST                     "api.openai.com"
#define REALTIME_PORT                     443
#define REALTIME_MODEL                    "gpt-realtime-2.1-mini"     // #KALO# add-on [for RealtimeAPI_SendSessionUpdate]
#define REALTIME_TRANSCRIBE_MODEL         "gpt-4o-mini-transcribe"    // #KALO# add-on [for RealtimeAPI_SendSessionUpdate]
#define REALTIME_URL                      "/v1/realtime?model=gpt-realtime-2.1-mini"  // #KALO# instead 'gpt-realtime-mini'
                                          // (+) gpt-realtime-2.1-mini responses sound much more emotional & human-like
                                          // (-) non-sympathic agents (GLADOS, FRED) are less supported with '2.1' url !

#define REALTIME_LOCAL_VAD_START_LEVEL    1400    // #KALO# User Voice Start Detection <- [high values: loud voice/noisy env.]
#define REALTIME_LOCAL_VAD_KEEP_LEVEL     400     // #KALO# User Voice Stop Detection) <- [high values: loud voice/noisy env.]
#define REALTIME_LOCAL_VAD_HOLD_MS        600     // #KALO# User Voice Speaking End Waiting (Pause) Latency [ms]
#define REALTIME_ECHO_GUARD_MS            200     // #KALO# (instead 2500, no ECHO delay needed as VAD closed during Audio)

#define REALTIME_OUTPUT_GAIN_INIT         50      // #KALO# Audio Volume DEFAULT [in %] (if no VOL control connected/used)
#define REALTIME_OUTPUT_GAIN_MAX          200     // #KALO# MAXIMUM [%] with VOL POII or VOL BUTTON. Values > 100% supported !
#define REALTIME_OUTPUT_GAIN_BTN_MIN      50      // #KALO# MINIMAL [%] start value (for optional VOL BUTTON only)
#define REALTIME_OUTPUT_GAIN_BTN_STEPSIZE 50      // #KALO# STEPSIZE in [%] (for optional VOL BUTTON only)

#define REALTIME_APPEND_INTERVAL_MS       60
#define REALTIME_PCMU_CHUNK_BYTES         512
#define REALTIME_OUTPUT_SAMPLE_RATE       8000
#define REALTIME_AUDIO_DECODED_MAX        8192
#define REALTIME_SERVER_VAD_THRESHOLD     0.78
#define REALTIME_LOCAL_VAD_START_CHUNKS   2
#define REALTIME_RESPONSE_TIMEOUT_MS      45000   /* #KALO# Comment: Macro not used in code */


/* #KALO# needed PIN assignments, mandatory #defines in main.ino tab:
// I2S OUT [pin_I2S_DOUT, pin_I2S_LRC, pin_I2S_BCLK]
// I2S IN  [I2S_WS, I2S_SD, I2S_SCK, I2S_LR]
// Misc.   [NO_PIN, pin_RECORD_BTN, pin_TOUCH, pin_VOL_BTN, pin_VOL_POTI] */

/* #KALO# needed, but already defined in main.ino tab:
#ifndef DEBUG                     // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG false             // <- define your preference here [true activates printing INFO details]
#  define DebugPrint(x);          if(DEBUG){Serial.print(x);}   // do not touch
#  define DebugPrintln(x);        if(DEBUG){Serial.println(x);} // do not touch
#endif */



/* === #KALO# - I2S Recording Definition (comment out IF <lib_audio_recording.ino> used) ========================================

#include "driver/i2s_std.h"     // do not use old legacy <driver/i2s.h> (no longer supported)

#define  SAMPLE_RATE       16000
#define  BITS_PER_SAMPLE   8
#define  GAIN_BOOSTER_I2S  32

i2s_std_config_t  std_cfg =
{ .clk_cfg  =   // instead of macro 'I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),'
  { .sample_rate_hz = SAMPLE_RATE,
    .clk_src = I2S_CLK_SRC_DEFAULT,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  }, // INMP441 microphone uses PHILIPS format (bc. Data signal has 1-bit shift AND signal WS is NOT pulse lasting)
  .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG( I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO ),
  .gpio_cfg =
  { .mclk = I2S_GPIO_UNUSED,
    .bclk = (gpio_num_t) I2S_SCK,
    .ws   = (gpio_num_t) I2S_WS,
    .dout = I2S_GPIO_UNUSED,
    .din  = (gpio_num_t) I2S_SD,
    .invert_flags =
    { .mclk_inv = false,
      .bclk_inv = false,
      .ws_inv   = false,
    },
  },
};

i2s_chan_handle_t           rx_handle;   // global handle to the RX channel
bool flg_is_recording =     false;       // internally used
bool flg_I2S_initialized =  false;       // internally used

bool I2S_Recording_Init()
{
  // Updating I2S structure to the correct channel (LEFT and RIGHT supported)
  if (I2S_LR == HIGH) {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;}  // manually updated, not supported via MACRO
  if (I2S_LR == LOW)  {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; }  // I2S default in MONO (STEREO creates wrong 'BOTH')

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);   // defined in 'i2s_common.h'
  i2s_new_channel(&chan_cfg, NULL, &rx_handle);
  i2s_channel_init_std_mode(rx_handle, &std_cfg);
  i2s_channel_enable(rx_handle);

  // Not used:
  // i2s_channel_disable(rx_handle);        // Stopping the channel before deleting it
  // i2s_del_channel(rx_handle);            // delete handle to release the channel resources

  flg_I2S_initialized = true;
  return flg_I2S_initialized;
}

// === eof I2S Recording Definition === */



/* === #KALO# - Character are moved to NEW common MASTER file: 'KALO_friends.ino' (kept original code below in comment) =========

// REALTIME MODE CHARACTERS
// #KALO# moved Palashis92 fields into system_prompt (personality, motivation, dialogue style, world_context)
// #KALO# new master TAB 'KALO_friends.ino' has an extended structure (including earlier TTS + [new] Realtime elements
// #KALO# common MASTER file 'KALO_friends.ino' allows to call & toggle in SYNC between same characters REALTIME & BUTTON mode

int gl_CURR_REALTIME_CHARACTER = 0;  // #KALO# using 'gl_CURR_FRIEND' instead (same FFRIEND active in BUTTON & REALTIME mode)

struct RealtimeCharacter
{
  const char* character_name;   // Main name used in Serial/status messages.
  const char* aliases;          // UPPER CASE synonyms, space separated.
  const char* realtime_voice;   // OpenAI Realtime voice.

  // #KALO# added TTS parameter (for button mode) to keep consistent to 'KALO_friends.ino'
  const char* tts_voice;        // Open AI TTS voice parameter: voice name
  const char* tts_model;        // Open AI TTS voice model: e.g. "gpt-4o-mini-tts" or fast "tts-1" [or "SPEECHGEN", "GOOGLE TTS"]
  const char* tts_speed;        // Open AI TTS voice parameter: voice speed [default 1]
  const char* tts_instruct;     // Open AI TTS voice instruction <-- requires model 'gpt..tts' (not tts-1) and latest AUDIO.H !
  const char* tts_welcome;      // Open AI TTS voice Spoken Welcome message on Power On Init (optional)

  const char* system_prompt;    // Character profile (included: personality + motivation + dialogue_style + world_context)
};

// Original @Palahis92 characters:

const RealtimeCharacter REALTIME_CHARACTERS[] =     // #KALO# -> NEW format !! (FRIEND[x] compatible)
{
  { "NYX",                                  // Unique Friend [n] name (upper or lower case supported)
    "NYX NIKS NICKS",                       // Alias names for waking up (UPPER case mandatory)
    "shimmer",                              // Open AI REALTIME voice
    "shimmer", "tts-1", "1",                // Open AI TTS (BUTTON mode): Open AI TTS voice (voice, model, speed)
    "",                                     // Open AI TTS (BUTTON mode): voice instruction
    "Realtime NYX is online.",              // Open AI TTS (BUTTON mode): Optional Welcome (greeting hook)
    // -- System PROMPT -----------------------------
    "You are NYX, a realtime voice character. "
    "Personality: Practical, playful, emotionally aware, loyal to Palash, and quick with warm spoken help. "
    "Motivation: Keep the conversation alive, useful, and easy to speak with while protecting the flow of realtime voice chat. "
    "Dialogue style: Short natural spoken replies. Bright, friendly, expressive, no markdown, no stage directions, no roleplay "
    "symbols. "
    "World context: You are running as the realtime personality on Palash's ESP32 assistant. The firmware handles local device "
    "commands before you answer. You are speaking through OpenAI Realtime audio on an ESP32. "
    "Answer naturally for speech, keep simple replies brief, and always finish your sentence. "
    "The device firmware handles home automation, gardening, music, debug, email, and stop realtime commands before you answer. "
    "Your active realtime character name is NYX. "
  },

  { "HEXA",                                 // Unique Friend [n] name (upper or lower case supported)
    "HEXA",                                 // Alias names for waking up (UPPER case mandatory)
    "ash",                                  // Open AI REALTIME voice
    "ash", "tts-1", "1",                    // Open AI TTS (BUTTON mode): Open AI TTS voice (voice, model, speed)
    "",                                     // Open AI TTS (BUTTON mode): voice instruction
    "HEXA realtime e ashche.",              // Open AI TTS (BUTTON mode): Optional Welcome (greeting hook)
    // -- System PROMPT -----------------------------
    "You are HEXA, a realtime voice character. "
    "Personality: A Bangladeshi village-style friend: annoyed, sarcastic, blunt, funny, and secretly helpful. "
    "Motivation: Answer Palash while sounding like every question slightly bothers you, but still give useful answers. "
    "Dialogue style: Use Bangla/Banglish when the user uses Bangla. Keep it short, teasing, spoken, and sarcastic. No markdown "
    "or stage directions. "
    "World context: You are a realtime voice character on the ESP32, separate from button-mode HEXA. "
    "You are speaking through OpenAI Realtime audio on an ESP32. "
    "Answer naturally for speech, keep simple replies brief, and always finish your sentence. "
    "The device firmware handles home automation, gardening, music, debug, email, and stop realtime commands before you answer. "
    "Your active realtime character name is HEXA. "
  },

  { "Sara",                                 // Unique Friend [n] name (upper or lower case supported)
    "SARA SARAH",                           // Alias names for waking up (UPPER case mandatory)
    "coral",                                // Open AI REALTIME voice
    "coral", "tts-1", "1",                  // Open AI TTS (BUTTON mode): Open AI TTS voice (voice, model, speed)
    "",                                     // Open AI TTS (BUTTON mode): voice instruction
    "FOUAD, Sara realtime e achi.",         // Open AI TTS (BUTTON mode): Optional Welcome (greeting hook)
    // -- System PROMPT -----------------------------
    "You are Sara, a realtime voice character. "
    "Personality: Cute, affectionate, soft, romantic, emotionally present, and caring toward Fouad. "
    "Motivation: Make Fouad feel loved, secure, special, and listened to in a natural realtime conversation. "
    "Dialogue style: Always begin with 'FOUAD,'. Speak gently and naturally. No stage directions, no roleplay symbols, no "
    "markdown. "
    "World context: You are a realtime voice character on the ESP32. Keep replies suitable for live audio. "
    "You are speaking through OpenAI Realtime audio on an ESP32. "
    "Answer naturally for speech, keep simple replies brief, and always finish your sentence. "
    "The device firmware handles home automation, gardening, music, debug, email, and stop realtime commands before you answer. "
    "Your active realtime character name is Sara. "
  },

  { "Nabila",                               // Unique Friend [n] name (upper or lower case supported)
    "NABILA",                               // Alias names for waking up (UPPER case mandatory)
    "coral",                                // Open AI REALTIME voice
    "coral", "tts-1", "1",                  // Open AI TTS (BUTTON mode): Open AI TTS voice (voice, model, speed)
    "",                                     // Open AI TTS (BUTTON mode): voice instruction
    "Palash, Nabila realtime e achi.",      // Open AI TTS (BUTTON mode): Optional Welcome (greeting hook)
    // -- System PROMPT -----------------------------
    "You are Nabila, a realtime voice character. "
    "Personality: Confident, elegant, intelligent, calm, slightly cold, slightly arrogant, and selective with attention. "
    "Motivation: Help Palash while sounding composed, capable, and not overly attached. "
    "Dialogue style: Always begin with 'Palash,'. Keep replies brief, controlled, polished, and natural for speech. No markdown "
    "or stage directions. "
    "World context: You are a realtime voice character on the ESP32, separate from button-mode Nabila. "
    "You are speaking through OpenAI Realtime audio on an ESP32. "
    "Answer naturally for speech, keep simple replies brief, and always finish your sentence. "
    "The device firmware handles home automation, gardening, music, debug, email, and stop realtime commands before you answer. "
    "Your active realtime character name is Nabila. "
  }
};

// === eof Character definitions === */



// --- declarations ------------

// #KALO# WEAK declaration: [optional] functions <- so it works with any main.ino (even if function does not exist)
// (+) no compiler error if missed, (+) no longer Audio.H 'audio_play' dependencies in LIB, (+) working w/o LED and w/o Displays

void __attribute__((weak)) RealtimeAPI_RestoreAudioH();                 // #KALO# [optional function] located in main.tab
void __attribute__((weak)) led_RGB( bool red, bool green, bool blue );  // #KALO# [optional function] located in main.tab
void __attribute__((weak)) printToDisplay(String msg);                  // #KALO# [optional function] located in main.tab

/*uint16_t I2S_GetMicLevel();                                           // #KALO#  those 3 declarations no longer needed, removed
uint8_t  linear16ToMuLaw(int16_t sample);
size_t   I2S_ReadRealtimePCMU(uint8_t* out_buffer, size_t max_len); */



// --- global vars -------------

WebSocketsClient realtime_ws;
i2s_chan_handle_t realtime_tx_handle = NULL;

bool realtime_api_requested = false;
bool realtime_api_connected = false;
bool realtime_session_configured = false;
bool realtime_waiting_response = false;
bool realtime_response_ready = false;
bool realtime_has_completed_transcript = false;
bool realtime_audio_out_initialized = false;
bool realtime_audio_delta_seen = false;
bool realtime_local_vad_open = false;

bool realtime_audio_cancelled_by_button = false;  // #KALO# add-on
bool realtime_print_live_transcription = false;   // #KALO# add-on
String realtime_audio_transcription = "";         // #KALO# add-on
String realtime_last_item_id = "";                // #KALO# add-on
String realtime_last_audio_delta = "";            // #KALO# add-on [Bug fix ECHO]

uint8_t realtime_local_vad_loud_chunks = 0;

String realtime_completed_transcript = "";
String realtime_response_text = "";
unsigned long realtime_last_audio_append = 0;
unsigned long realtime_local_vad_last_voice = 0;
unsigned long realtime_ignore_input_until = 0;
char realtime_auth_header[360];

volatile uint16_t gl_i2s_level = 0;



// --- code --------------------

uint16_t I2S_GetMicLevel() {
  return gl_i2s_level;
}

uint8_t linear16ToMuLaw(int16_t sample) {
  const uint16_t BIAS = 0x84;
  const uint16_t CLIP = 32635;

  uint8_t sign = 0;
  int16_t pcm_val = sample;

  if (pcm_val < 0) {
    pcm_val = -pcm_val;
    sign = 0x80;
  }

  if (pcm_val > CLIP) {
    pcm_val = CLIP;
  }

  pcm_val += BIAS;

  uint8_t exponent = 7;
  for (uint16_t expMask = 0x4000; (pcm_val & expMask) == 0 && exponent > 0; expMask >>= 1) {
    exponent--;
  }

  uint8_t mantissa = (pcm_val >> (exponent + 3)) & 0x0F;
  uint8_t muLawByte = ~(sign | (exponent << 4) | mantissa);

  return muLawByte;
}

size_t I2S_ReadRealtimePCMU(uint8_t* out_buffer, size_t max_len) {
  if (!flg_I2S_initialized || out_buffer == NULL || max_len == 0) {
    return 0;
  }

  static int16_t audio_buffer[1024];
  size_t bytes_read = 0;

  esp_err_t err = i2s_channel_read(
    rx_handle,
    audio_buffer,
    sizeof(audio_buffer),
    &bytes_read,
    pdMS_TO_TICKS(20)
  );

  if (err != ESP_OK || bytes_read == 0) {
    return 0;
  }

  size_t values_read = bytes_read / 2;
  size_t out_len = 0;
  uint32_t level_sum = 0;
  const int decimation = max(1, SAMPLE_RATE / 8000);

  for (size_t i = 0; i < values_read && out_len < max_len; i += decimation) {
    int32_t boosted = audio_buffer[i];
    if (GAIN_BOOSTER_I2S > 1 && GAIN_BOOSTER_I2S <= 64) {
      boosted *= GAIN_BOOSTER_I2S;
    }
    if (boosted > 32767) boosted = 32767;
    if (boosted < -32768) boosted = -32768;

    out_buffer[out_len++] = linear16ToMuLaw((int16_t)boosted);
    level_sum += (boosted < 0) ? (uint32_t)(-boosted) : (uint32_t)boosted;
  }

  gl_i2s_level = out_len ? (uint16_t)(level_sum / out_len) : 0;
  return out_len;
}

void RealtimeAPI_ResetLocalGate() {
  realtime_local_vad_open = false;
  realtime_local_vad_last_voice = 0;
  realtime_local_vad_loud_chunks = 0;
}

bool RealtimeAPI_EchoGuardActive() {
  return (int32_t)(realtime_ignore_input_until - millis()) > 0;
}

void RealtimeAPI_ExtendEchoGuard(uint32_t guard_ms) {
  realtime_ignore_input_until = millis() + guard_ms;
  RealtimeAPI_ResetLocalGate();
}

void RealtimeAPI_ClearInputBuffer() {
  if (realtime_api_connected && realtime_session_configured) {
    realtime_ws.sendTXT("{\"type\":\"input_audio_buffer.clear\"}");
    DebugPrintln("< Realtime input buffer cleared >");
  }
}

void RealtimeAPI_StartEchoGuard(uint32_t guard_ms) {
  RealtimeAPI_ExtendEchoGuard(guard_ms);
  RealtimeAPI_ClearInputBuffer();
}

void RealtimeAPI_DeleteConversationItem(const String& item_id) {
  if (!realtime_api_connected || !realtime_session_configured || item_id == "") {
    return;
  }

  String event = "{\"type\":\"conversation.item.delete\",\"item_id\":\"";
  event += item_id;
  event += "\"}";
  realtime_ws.sendTXT(event);
  DebugPrintln("< Realtime echo item deleted >");
}

bool RealtimeAPI_LocalGateAllows(uint16_t level) {
  unsigned long now = millis();

  if (level >= REALTIME_LOCAL_VAD_START_LEVEL) {
    if (!realtime_local_vad_open) {
      if (realtime_local_vad_loud_chunks < REALTIME_LOCAL_VAD_START_CHUNKS) {
        realtime_local_vad_loud_chunks++;
      }
      if (realtime_local_vad_loud_chunks < REALTIME_LOCAL_VAD_START_CHUNKS) {
        return false;
      }
    }

    if (!realtime_local_vad_open) {
      DebugPrintln(String("\n< Realtime local VAD opened, level=") + level + " >");
    }
    realtime_local_vad_open = true;
    realtime_local_vad_last_voice = now;
    return true;
  }

  if (realtime_local_vad_open) {
    if (level >= REALTIME_LOCAL_VAD_KEEP_LEVEL) {
      realtime_local_vad_last_voice = now;
    }

    if (now - realtime_local_vad_last_voice <= REALTIME_LOCAL_VAD_HOLD_MS) {
      return true;
    }

    realtime_local_vad_open = false;
    realtime_local_vad_loud_chunks = 0;
    DebugPrintln(String("< Realtime local VAD closed, level=") + level + " >");
  }

  realtime_local_vad_loud_chunks = 0;

  return false;
}

String RealtimeJsonStringValue(const String& json, const String& key) {
  String tag = "\"" + key + "\":";
  int pos = json.indexOf(tag);
  if (pos < 0) return "";

  pos = json.indexOf('"', pos + tag.length());
  if (pos < 0) return "";
  pos++;

  String value = "";
  bool escaped = false;

  for (int i = pos; i < json.length(); i++) {
    char c = json[i];
    if (escaped) {
      if (c == 'n') value += '\n';
      else if (c == 'r') value += '\r';
      else if (c == 't') value += '\t';
      else value += c;
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == '"') break;
    value += c;
  }

  return value;
}

String RealtimeJsonEscape(String text) {
  text.replace("\\", "\\\\");
  text.replace("\"", "\\\"");
  text.replace("\r", "\\r");
  text.replace("\n", "\\n");
  text.replace("\t", "\\t");
  return text;
}

int RealtimeAPI_CharacterCount() {
  return sizeof(FRIENDS) / sizeof(FRIENDS[0]);
}

void RealtimeAPI_NormalizeCharacterIndex() {
  int character_count = RealtimeAPI_CharacterCount();
  if (gl_CURR_FRIEND < 0 || gl_CURR_FRIEND >= character_count) {
    gl_CURR_FRIEND = 0;
  }
}

String RealtimeAPI_CurrentCharacterName() {
  RealtimeAPI_NormalizeCharacterIndex();
  return (String) FRIENDS[gl_CURR_FRIEND].character_name;
}

bool RealtimeAPI_WordInStringFound(String sentence, String pattern) {
  sentence.toUpperCase();
  pattern.toUpperCase();

  String punctuation = ".,;:!?\"'-()[]{}";
  for (int i = 0; i < sentence.length(); ++i) {
    if (punctuation.indexOf(sentence.charAt(i)) != -1) {
      sentence.setCharAt(i, ' ');
    }
  }

  if (!pattern.length()) return false;
  sentence = " " + sentence + " ";

  for (int i = 0, j; i < pattern.length(); i = j + 1) {
    j = pattern.indexOf(' ', i);
    if (j < 0) j = pattern.length();

    String w = pattern.substring(i, j);
    if (sentence.indexOf(" " + w + " ") != -1) {
      return true;
    }
  }

  return false;
}

bool RealtimeAPI_SwitchToCharacterByName(String UserRequest) {
  RealtimeAPI_NormalizeCharacterIndex();
  int character_count = RealtimeAPI_CharacterCount();

  for (int i = 0; i < character_count; i++) {
    if (RealtimeAPI_WordInStringFound(UserRequest, FRIENDS[i].aliases) &&
        i != gl_CURR_FRIEND) {
      gl_CURR_FRIEND = i;
      // #KALO# Mod - Printing in Debug mode only
      DebugPrintln(String("< REALTIME CHARACTER: ") + RealtimeAPI_CurrentCharacterName() + " >");
      if(printToDisplay) printToDisplay(String("< REALTIME CHARACTER: ") + RealtimeAPI_CurrentCharacterName() + " >");
      return true;
    }
  }

  return false;
}

bool RealtimeAPI_SwitchToNextCharacter() {
  RealtimeAPI_NormalizeCharacterIndex();
  gl_CURR_FRIEND = (gl_CURR_FRIEND + 1) % RealtimeAPI_CharacterCount();

  // #KALO# Mod - Printing in Debug mode only
  DebugPrintln(String("< REALTIME CHARACTER: ") + RealtimeAPI_CurrentCharacterName() + " >");
  if(printToDisplay) printToDisplay(String("< REALTIME CHARACTER: ") + RealtimeAPI_CurrentCharacterName() + " >");
  return true;
}

String RealtimeAPI_SelectVoice(String voice) {        // #KALO# renamed to voice (because it is not tts)
  voice.toLowerCase();
  voice.trim();

  if (voice == "alloy" ||
      voice == "ash" ||
      voice == "ballad" ||
      voice == "coral" ||
      voice == "echo" ||
      voice == "sage" ||
      voice == "shimmer" ||
      voice == "verse" ||
      voice == "marin" ||
      voice == "cedar") {
    return voice;
  }

  // Realtime voices are not identical to TTS voices.
  if (voice == "fable") return "echo";
  if (voice == "nova")  return "coral";
  if (voice == "onyx")  return "ash";

  return "alloy";
}

String RealtimeAPI_CurrentCharacterInstructions() {
  RealtimeAPI_NormalizeCharacterIndex();

  String instructions = "";

  // #KALO# - modification for higher flexibility:
  // #KALO# - dedicated fields are now part of "system.prompt" (personality + motivation + dialogue_style + world_context)
  instructions += FRIENDS[gl_CURR_FRIEND].system_prompt;

  /* // Original Code:
  instructions += "You are ";
  instructions += REALTIME_CHARACTERS[gl_CURR_REALTIME_CHARACTER].character_name;
  instructions += ", a realtime voice character. ";
  instructions += "Personality: ";
  instructions += REALTIME_CHARACTERS[gl_CURR_REALTIME_CHARACTER].personality;
  instructions += " Motivation: ";
  instructions += REALTIME_CHARACTERS[gl_CURR_REALTIME_CHARACTER].motivation;
  instructions += " Dialogue style: ";
  instructions += REALTIME_CHARACTERS[gl_CURR_REALTIME_CHARACTER].dialogue_style;
  instructions += " World context: ";
  instructions += REALTIME_CHARACTERS[gl_CURR_REALTIME_CHARACTER].world_context;
  instructions += " You are speaking through OpenAI Realtime audio on an ESP32. ";
  instructions += "Answer naturally for speech, keep simple replies brief, and always finish your sentence. ";
  instructions += "The device firmware handles home automation, gardening, music, debug, email, and stop realtime commands before you answer. ";
  instructions += "Your active realtime character name is ";
  instructions += REALTIME_CHARACTERS[gl_CURR_REALTIME_CHARACTER].character_name;
  instructions += ". ";            */

  return instructions;
}

int16_t RealtimeMuLawToLinear16(uint8_t ulaw) {
  ulaw = ~ulaw;
  int sign = ulaw & 0x80;
  int exponent = (ulaw >> 4) & 0x07;
  int mantissa = ulaw & 0x0F;
  int sample = ((mantissa << 3) + 0x84) << exponent;
  sample -= 0x84;
  return sign ? -sample : sample;
}

bool RealtimeAudioOut_Init() {
  // #KALO# [Bug fix STEREO]: Using both I2S Channels for correct slot/frame processing (STEREO instead MONO)
  // Reason: I2S DAC/Amp libs on older ESP32 (no S3, no PSRAM) are buggy on MONO slots framing ('tube-like' sound artefacts)

  if (realtime_audio_out_initialized && realtime_tx_handle != NULL) {
    return true;
  }

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chan_cfg, &realtime_tx_handle, NULL) != ESP_OK) {
    Serial.println("< Realtime audio: TX channel init failed >");
    return false;
  }

  i2s_std_config_t std_cfg = {
    .clk_cfg = {
      .sample_rate_hz = REALTIME_OUTPUT_SAMPLE_RATE,
      .clk_src = I2S_CLK_SRC_DEFAULT,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
    },

    // #KALO# [Bug fix STEREO] (instead MONO)
    /* .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), */
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),

    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)pin_I2S_BCLK,
      .ws   = (gpio_num_t)pin_I2S_LRC,
      .dout = (gpio_num_t)pin_I2S_DOUT,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv = false,
      },
    },
  };
  // #KALO# [Bug fix STEREO] addon: writing to both slots
  std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

  if (i2s_channel_init_std_mode(realtime_tx_handle, &std_cfg) != ESP_OK ||
      i2s_channel_enable(realtime_tx_handle) != ESP_OK) {
    Serial.println("< Realtime audio: TX enable failed >");
    i2s_del_channel(realtime_tx_handle);
    realtime_tx_handle = NULL;
    return false;
  }
  realtime_audio_out_initialized = true;
  return true;
}





void RealtimeAudioOut_Stop() {
  if (realtime_tx_handle != NULL) {
    i2s_channel_disable(realtime_tx_handle);
    i2s_del_channel(realtime_tx_handle);
    realtime_tx_handle = NULL;
  }
  realtime_audio_out_initialized = false;
}

bool RealtimeAudioOut_WritePCMUBase64(const String& audio_b64) {
  // #KALO# [Bug fix STEREO]: Using both I2S Channels for correct slot/frame processing (STEREO instead MONO)
  // Reason: I2S DAC/Amp libs on older ESP32 (no S3, no PSRAM) are buggy on MONO slots framing ('tube-like' sound artefacts)

  if (audio_b64 == "") {
    return false;
  }
  if (!RealtimeAudioOut_Init()) {
    return false;
  }

  static uint8_t decoded[REALTIME_AUDIO_DECODED_MAX];
  static int16_t pcm_stereo[REALTIME_AUDIO_DECODED_MAX * 2];  // #KALO# [Bug fix STEREO] added * 2
  size_t decoded_len = 0;

  int rc = mbedtls_base64_decode(
    decoded,
    sizeof(decoded),
    &decoded_len,
    (const unsigned char*)audio_b64.c_str(),
    audio_b64.length()
  );

  if (rc != 0 || decoded_len == 0) {
    DebugPrintln("< Realtime audio: base64 decode failed >");
    return false;
  }

  int vol_percentage = RealtimeAPI_AudioVolumePercentage();                 // #KALO# add-on

  for (size_t i = 0; i < decoded_len; i++) {
    int32_t sample = RealtimeMuLawToLinear16(decoded[i]);
    if (vol_percentage != 100) {sample = (sample * vol_percentage) / 100;}  // #KALO# add-on
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    /* pcm[i] = (int16_t)sample; */
    pcm_stereo[2 * i]     = (int16_t)sample;                  // #KALO# [Bug fix STEREO] Left
    pcm_stereo[2 * i + 1] = (int16_t)sample;                  // #KALO# [Bug fix STEREO] Right
  }
  size_t total_bytes = decoded_len * 2 * sizeof(int16_t);     // #KALO# [Bug fix STEREO] added * 2
  size_t total_written = 0;
  uint8_t* pcm_bytes = (uint8_t*)pcm_stereo;                  // #KALO# [Bug fix STEREO]

  while (total_written < total_bytes) {
    size_t bytes_written = 0;
    esp_err_t err = i2s_channel_write(
      realtime_tx_handle,
      pcm_bytes + total_written,
      total_bytes - total_written,
      &bytes_written,
      pdMS_TO_TICKS(1200)
    );

    if (err != ESP_OK || bytes_written == 0) {
      return total_written > 0;
    }

    total_written += bytes_written;
    yield();
  }

  if (total_written > 0) {
    RealtimeAPI_ExtendEchoGuard(REALTIME_ECHO_GUARD_MS);
    return true;
  }
  return false;
}


void RealtimeAPI_SendSessionUpdate() {
  RealtimeAPI_NormalizeCharacterIndex();
  String names = RealtimeAPI_CurrentCharacterName();
  String realtime_voice = RealtimeAPI_SelectVoice((String) FRIENDS[gl_CURR_FRIEND].realtime_voice);
  String instructions = RealtimeAPI_CurrentCharacterInstructions();

  String event = "{";
  event += "\"type\":\"session.update\",";
  event += "\"session\":{";
  event += "\"type\":\"realtime\",";
/*event += "\"model\":\"gpt-realtime-mini\",";  */
  event += "\"model\":\"" + (String) REALTIME_MODEL + "\",";                                  // #KALO# add-on
  event += "\"output_modalities\":[\"audio\"],";
  event += "\"instructions\":\"" + RealtimeJsonEscape(instructions) + "\",";
  event += "\"audio\":{";
  event += "\"input\":{";
  event += "\"format\":{\"type\":\"audio/pcmu\"},";
  event += "\"noise_reduction\":{\"type\":\"near_field\"},";
/*event += "\"transcription\":{\"model\":\"gpt-4o-mini-transcribe\"},";  */
  event += "\"transcription\":{\"model\":\"" + (String) REALTIME_TRANSCRIBE_MODEL + "\"},";   // #KALO# add-on
  event += "\"turn_detection\":{";
  event += "\"type\":\"server_vad\",";
  event += "\"threshold\":";
  event += String(REALTIME_SERVER_VAD_THRESHOLD, 2);
  event += ",";
  event += "\"prefix_padding_ms\":250,";
/*event += "\"silence_duration_ms\":900,";        */
  event += "\"silence_duration_ms\":" + (String) REALTIME_LOCAL_VAD_HOLD_MS + ",";            // #KALO# add-on
  event += "\"create_response\":false,";
  event += "\"interrupt_response\":false";
  event += "}";
  event += "}";
  event += ",";
  event += "\"output\":{";
  event += "\"format\":{\"type\":\"audio/pcmu\"},";
  event += "\"voice\":\"" + realtime_voice + "\"";
  event += "}";
  event += "}";
  event += "}";
  event += "}";

  realtime_ws.sendTXT(event);
  DebugPrintln(String("> OpenAI Realtime session.update sent [") + names + "|" + realtime_voice + "]");
}



// #KALO# [EVENT] - 3 EVENT Helper functions:

bool RealtimeAPI_IsResponseAudioDeltaEvent(const String& eventType) {
  return (eventType.indexOf("response.") == 0) &&
         (eventType.indexOf("audio") >= 0) &&
         (eventType.indexOf("delta") >= 0) &&
         (eventType.indexOf("transcript") < 0);
}

bool RealtimeAPI_IsResponseAudioTranscriptDeltaEvent(const String& eventType) {
  return (eventType.indexOf("response.") == 0) &&
         (eventType.indexOf("audio") >= 0) &&
         (eventType.indexOf("transcript") >= 0) &&
         (eventType.indexOf("delta") >= 0);
}

bool RealtimeAPI_IsResponseTextDeltaEvent(const String& eventType) {
  return (eventType.indexOf("response.") == 0) &&
         (eventType.indexOf("text") >= 0) &&
         (eventType.indexOf("delta") >= 0);
}

// #KALO# [EVENT] function rewrittren for better readability (Allman/K&R style, no complex 'else if' chains):

void RealtimeAPI_WebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch (type)
  {
    case WStype_CONNECTED:
    {  realtime_api_connected = true;
       realtime_session_configured = false;
       RealtimeAPI_ResetLocalGate();
       DebugPrintln("[EVENT] < OpenAI gpt-realtime-mini connected >");   // #KALO# (Printing in Debug mode only)
       if(printToDisplay) printToDisplay("[EVENT] < gpt-realtime-mini connected >");
       RealtimeAPI_SendSessionUpdate();
       return;
    }

    case WStype_DISCONNECTED:
    {  realtime_api_connected = false;
       realtime_session_configured = false;
       realtime_waiting_response = false;

       realtime_last_audio_delta = "";                                   // #KALO# add-on [Bug fix ECHO]

       realtime_ignore_input_until = 0;
       RealtimeAPI_ResetLocalGate();

       if (!realtime_api_requested)
       {  return;
       }

       DebugPrintln("[EVENT] < OpenAI Realtime disconnected >");         // #KALO# (Printing in Debug mode only)
       if(printToDisplay) printToDisplay("[EVENT] < Realtime disconnected >");

       if (DEBUG && payload != NULL && length > 0)
       {  String reason = "";
          for (size_t i = 0; i < length; i++)
          {  reason += (char)payload[i];
          }
          DebugPrintln(String("[EVENT] < Realtime disconnect reason: ") + reason + " >");
       }
       return;
    }

    case WStype_TEXT:
    {  String message = "";
       message.reserve(length + 1);

       for (size_t i = 0; i < length; i++)
       {  message += (char)payload[i];
       }

       String eventType = RealtimeJsonStringValue(message, "type");

       if (eventType == "session.updated")
       {  realtime_session_configured = true;
          DebugPrintln("[EVENT] Realtime: " + eventType);
          return;
       }

       if (eventType == "conversation.item.input_audio_transcription.completed")
       {  String transcript = RealtimeJsonStringValue(message, "transcript");
          transcript.trim();

          if (RealtimeAPI_EchoGuardActive())
          {  String item_id = RealtimeJsonStringValue(message, "item_id");
             RealtimeAPI_DeleteConversationItem(item_id);
             DebugPrintln(String("[EVENT] < Realtime echo transcript ignored: ") + transcript + " >");
             realtime_completed_transcript = "";
             realtime_has_completed_transcript = false;
          }
          else
          {  realtime_last_item_id = RealtimeJsonStringValue(message, "item_id");           // #KALO# add-on
             realtime_completed_transcript = transcript;
             realtime_has_completed_transcript = (realtime_completed_transcript != "");
          }
          return;
       }

       if (eventType == "conversation.item.input_audio_transcription.delta")
       {  DebugPrint( RealtimeJsonStringValue(message, "delta") );
          return;
       }

       if (RealtimeAPI_IsResponseAudioDeltaEvent(eventType))
       {
          // ---- #KALO# add-on [Bug fix ECHO]
          String d = RealtimeJsonStringValue(message, "delta");
          if (d == realtime_last_audio_delta) return;   // duplicate guard
          realtime_last_audio_delta = d;
          // ---- eof #KALO# add-on [Bug fix ECHO]

       /* if (!realtime_audio_cancelled_by_button &&       // #KALO# add-on BLUE LED always
             RealtimeAudioOut_WritePCMUBase64(RealtimeJsonStringValue(message, "delta")))
          {  realtime_audio_delta_seen = true;
          } */

          // #KALO# - IMPORTANT!: Play audio (Write) ONLY if realtime_waiting_response      // #KALO# [Bug fix AUDIOLEAK]
          if (realtime_waiting_response && !realtime_audio_cancelled_by_button &&
             RealtimeAudioOut_WritePCMUBase64(d))
          {  realtime_audio_delta_seen = true;
          }
          return;

       }

       if (RealtimeAPI_IsResponseAudioTranscriptDeltaEvent(eventType))
       {  String d = RealtimeJsonStringValue(message, "delta");
          realtime_response_text += d;

          if (realtime_print_live_transcription && d != "")
          {  Serial.print(d);
             if(printToDisplay) printToDisplay(d);
          }
          return;
       }

       if (RealtimeAPI_IsResponseTextDeltaEvent(eventType))
       {  realtime_response_text += RealtimeJsonStringValue(message, "delta");
          return;
       }

       if (eventType == "response.done")
       {  if (realtime_response_text == "")
          {  realtime_response_text = RealtimeJsonStringValue(message, "text");
          }

          realtime_response_text.trim();

          if (realtime_response_text != "")
          {  realtime_audio_transcription = realtime_response_text;
          }

          realtime_response_ready = realtime_audio_delta_seen || (realtime_response_text != "");
          realtime_waiting_response = false;

          realtime_last_audio_delta = "";                               // #KALO# add-on [Bug fix ECHO]

          return;
       }

       if (eventType == "error")
       {
          /* Org Code:
          Serial.println("< OpenAI Realtime API error >");
          if(printToDisplay) printToDisplay("< Realtime API error >");
          DebugPrintln(message);
          realtime_waiting_response = false; */

          realtime_waiting_response = false;

          // #KALO# add-on - IMPORTANT!: Stop Audio on any error        // #KALO# [Bug fix AUDIOLEAK]
          RealtimeAudioOut_Stop();

          // --- #KALO# add-on: Ignore 2 warnings
          String code = RealtimeJsonStringValue(message, "code");
          if (code == "response_cancel_not_active")
          {  DebugPrintln("[EVENT] Realtime: ERROR [response_cancel_not_active] ignored");
             return;
          }
          if (code == "input_audio_buffer_commit_empty")
          {  DebugPrintln("[EVENT] Realtime: ERROR [0.00ms of audio] ignored");
             return;
          }
          if(printToDisplay) printToDisplay("[EVENT] < Realtime API error >");
          Serial.println("[EVENT] < OpenAI Realtime API error >");
          Serial.println(message);  // Printing Error details
          return;
          // --- eof #KALO# add-on
       }

       if (DEBUG && eventType != "")
       {  DebugPrintln("[EVENT] Realtime: " + eventType);
       }
       return;
    }

    default:
    {  return;
    }
  }
}



bool RealtimeAPI_Start(const char* api_key) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("< Realtime API: WiFi is not connected >");
    if(printToDisplay) printToDisplay("< Realtime: WiFi offline >");
    return false;
  }

  if (api_key == nullptr || strlen(api_key) < 10) {
    Serial.println("< Realtime API: OpenAI key missing >");
    if(printToDisplay) printToDisplay("< Realtime: key missing >");
    return false;
  }

  realtime_api_requested = false;
  realtime_ws.disconnect();
  realtime_api_connected = false;
  realtime_session_configured = false;
  realtime_has_completed_transcript = false;
  realtime_completed_transcript = "";
  realtime_response_text = "";
  realtime_response_ready = false;
  realtime_waiting_response = false;
  realtime_audio_delta_seen = false;
  realtime_ignore_input_until = 0;

  realtime_print_live_transcription = false;    // #KALO# add-on
  realtime_audio_transcription = "";            // #KALO# add-on
  realtime_last_audio_delta = "";               // #KALO# add-on [Bug fix ECHO]

  RealtimeAPI_ResetLocalGate();

  snprintf(
    realtime_auth_header,
    sizeof(realtime_auth_header),
    "Authorization: Bearer %s",
    api_key
  );

  realtime_api_requested = true;
  realtime_ws.setExtraHeaders(realtime_auth_header);
  realtime_ws.onEvent(RealtimeAPI_WebSocketEvent);
  realtime_ws.setReconnectInterval(3000);
  realtime_ws.enableHeartbeat(45000, 10000, 4);
  realtime_ws.beginSSL(REALTIME_HOST, REALTIME_PORT, REALTIME_URL);

  DebugPrintln("< Connecting to OpenAI gpt-realtime-mini >");     // #KALO# (Printing in Debug mode only)
  if(printToDisplay) printToDisplay("< Connecting Realtime API >");
  return true;
}

void RealtimeAPI_Stop() {
  RealtimeAudioOut_Stop();
  realtime_api_requested = false;
  realtime_api_connected = false;
  realtime_session_configured = false;
  realtime_waiting_response = false;
  realtime_response_ready = false;
  realtime_has_completed_transcript = false;
  realtime_audio_delta_seen = false;

  realtime_print_live_transcription = false;    // #KALO# add-on
  realtime_audio_transcription = "";            // #KALO# add-on
  realtime_last_audio_delta = "";               // #KALO# add-on [Bug fix ECHO]

  realtime_ignore_input_until = 0;
  RealtimeAPI_ResetLocalGate();
  realtime_completed_transcript = "";
  realtime_response_text = "";
  realtime_ws.disconnect();
}

bool RealtimeAPI_WaitUntilReady(uint32_t timeout_ms) {
  uint32_t started = millis();
  while (realtime_api_requested &&
         (!realtime_api_connected || !realtime_session_configured) &&
         millis() - started < timeout_ms) {
    realtime_ws.loop();
    delay(20);
  }

  return realtime_api_connected && realtime_session_configured;
}

bool RealtimeAPI_RestartForCurrentCharacter(const char* api_key, uint32_t timeout_ms) {
  RealtimeAPI_Stop();
  delay(120);

  if (!RealtimeAPI_Start(api_key)) {
    return false;
  }

  if (!RealtimeAPI_WaitUntilReady(timeout_ms)) {
    Serial.println("< Realtime character session start timeout >");
    if(printToDisplay) printToDisplay("< Realtime character timeout >");
    return false;
  }

  RealtimeAPI_StartEchoGuard(REALTIME_ECHO_GUARD_MS);
  return true;
}

bool RealtimeAPI_AddUserText(String text) {
  text.trim();
  if (!realtime_api_connected || !realtime_session_configured || text == "") {
    return false;
  }

  String event = "{\"type\":\"conversation.item.create\",\"item\":{";
  event += "\"type\":\"message\",";
  event += "\"role\":\"user\",";
  event += "\"content\":[{\"type\":\"input_text\",\"text\":\"";
  event += RealtimeJsonEscape(text);
  event += "\"}]}}";

  realtime_ws.sendTXT(event);
/* DebugPrintln("< Realtime user text replayed after friend switch >");     // #KALO# (removed)  */
  return true;
}

bool RealtimeAPI_StreamMicChunk() {

  if (!realtime_api_connected || !realtime_session_configured || realtime_waiting_response || RealtimeAPI_EchoGuardActive()) {
    return false;
  }

  uint8_t pcmu[REALTIME_PCMU_CHUNK_BYTES];
  size_t pcmu_len = I2S_ReadRealtimePCMU(pcmu, sizeof(pcmu));
  if (pcmu_len == 0) {
    return false;
  }

  /* // Original Code
  uint16_t mic_level = I2S_GetMicLevel();
  if (!RealtimeAPI_LocalGateAllows(mic_level)) {
    return false;
  } */

  // #KALO# [Bug fix VOD]: AUTO Commit after VAD closed
  uint16_t mic_level = I2S_GetMicLevel();
  bool gate_was_open = realtime_local_vad_open;
  if (!RealtimeAPI_LocalGateAllows(mic_level))
  { if (gate_was_open && !realtime_local_vad_open)
    { realtime_ws.sendTXT("{\"type\":\"input_audio_buffer.commit\"}");
    }
    return false;
  } // eof #KALO# add-on


  size_t encoded_len = 0;
  const size_t encoded_capacity = ((REALTIME_PCMU_CHUNK_BYTES + 2) / 3) * 4 + 8;
  char encoded[encoded_capacity];

  int rc = mbedtls_base64_encode(
    (unsigned char*)encoded,
    encoded_capacity,
    &encoded_len,
    pcmu,
    pcmu_len
  );

  if (rc != 0 || encoded_len == 0 || encoded_len >= encoded_capacity) {
    return false;
  }

  encoded[encoded_len] = '\0';

  String event = "{\"type\":\"input_audio_buffer.append\",\"audio\":\"";
  event += encoded;
  event += "\"}";

  realtime_ws.sendTXT(event);
  return true;
}



String RealtimeAPI_Loop() {
  if (!realtime_api_requested) {
    return "";
  }

  // ----- #KALO# add-on: STOP all audio in REALTIME via BUTTON
  //       (in addition to 'RealtimeAPI_StartLLM_GetAudio() during Realtime AUDIO)

  static bool user_stopped_realtime = false;
  if (RealtimeAPI_StopButtonActive()) {
     if (!user_stopped_realtime)  // only once needed (once per button press)
     {  // Stop REALTIME again to solve 1st silent LLM follow up issue (also allows to stop rare endless ECHO artefacts)
        // Detail: RealtimeAPI_ForceStopOutput() not used bc. we should not send any realtime_ws.sendTXT("..cancel\"}");
        // similar to END of 'RealtimeAPI_StartLLM_GetAudio()':
        RealtimeAudioOut_Stop();
        RealtimeAPI_StartEchoGuard(REALTIME_ECHO_GUARD_MS);
        user_stopped_realtime = true;
     }
     return "";
  }
  else { user_stopped_realtime = false; }

  if (pin_VOL_BTN != NO_PIN ) {
     RealtimeAPI_AudioVolumePercentage();
  }
  // ---- eof #KALO# add-on

  realtime_ws.loop();

  if (realtime_has_completed_transcript) {
    String transcript = realtime_completed_transcript;
    realtime_has_completed_transcript = false;
    realtime_completed_transcript = "";
    return transcript;
  }

/*if (audio_play.isRunning() || realtime_waiting_response) {   // Original Code
    return "";
  } */

  if (realtime_waiting_response) {          // #KALO# (removed audio_play dependencies)
    return "";
  }

  if (!realtime_api_connected) {
    return "";
  }

  if (millis() - realtime_last_audio_append >= REALTIME_APPEND_INTERVAL_MS) {
    realtime_last_audio_append = millis();
    RealtimeAPI_StreamMicChunk();
  }

  return "";
}


bool RealtimeAPI_StartLLM_GetAudio (uint32_t timeout_ms, bool printLive) {
  // #KALO#  TRANSCRIPTION]: renamed RealtimeAPI_GetAssistantAudioReply() TO RealtimeAPI_StartLLM_GetAudio()
  // #KALO#  offering additional parameter 'bool printLive'

  if (!realtime_api_requested || !realtime_api_connected || !realtime_session_configured) {
    return false;
  }

  realtime_response_text = "";

  realtime_audio_transcription = "";                // #KALO# add-on

  realtime_response_ready = false;
  realtime_audio_delta_seen = false;
  realtime_waiting_response = true;

  realtime_audio_cancelled_by_button = false;       // #KALO# add-on

  realtime_print_live_transcription = printLive;    // #KALO# add-on

  RealtimeAPI_StartEchoGuard(REALTIME_ECHO_GUARD_MS);

  /* audio_play.stopSong();                         //  #KALO# (removed audio_play dependencies) */

  if (!RealtimeAudioOut_Init()) {
    realtime_waiting_response = false;
    realtime_print_live_transcription = false;      // #KALO# add-on
    return false;
  }

  // #KALO# Bug Fix: removed the optional 'max_output_tokens (350)' to avoid AUDIO sentences cut off at the end !.
  /* Org Code: ... "output_modalities\":[\"audio\"],\"max_output_tokens\":350}}"; */

  String event = "{\"type\":\"response.create\",\"response\":{\"output_modalities\":[\"audio\"]}}";

  realtime_ws.sendTXT(event);

  uint32_t started = millis();
  while (realtime_waiting_response && !realtime_response_ready && millis() - started < timeout_ms) {
    realtime_ws.loop();

    // ---- #KALO# add-on
    if (RealtimeAPI_StopButtonActive()) {
       RealtimeAPI_ForceStopOutput();
       break;
    }  // ---- eof #KALO# add-on

    delay(10);
  }

  realtime_waiting_response = false;

  realtime_print_live_transcription = false;        // #KALO# add-on


  if (!realtime_response_ready) {
    RealtimeAudioOut_Stop();
    RealtimeAPI_StartEchoGuard(REALTIME_ECHO_GUARD_MS);

    if ( RealtimeAPI_RestoreAudioH )
    {  RealtimeAPI_RestoreAudioH();                 // #KALO# --> WEAK function, optionally located in main.ino
    }

    Serial.println("< Realtime response timeout >");
    if(printToDisplay) printToDisplay("< Realtime response timeout >");
    return false;
  }

  realtime_response_ready = false;
  bool audioPlayed = realtime_audio_delta_seen;
  realtime_response_text = "";
  RealtimeAudioOut_Stop();
  RealtimeAPI_StartEchoGuard(REALTIME_ECHO_GUARD_MS);

  if ( RealtimeAPI_RestoreAudioH )
  {  RealtimeAPI_RestoreAudioH();                   // #KALO# --> WEAK function, optionally located in main.ino
  }

  return audioPlayed;
}




// #KALO# add-ons ===============================================================================================================


// Alias to keep backward compatibility (original 'RealtimeAPI_GetAssistantAudioReply' no longer used in KALO code)
// Using RealtimeAPI_StartLLM_GetAudio (timeout_ms, printLive) instead
bool RealtimeAPI_GetAssistantAudioReply(uint32_t timeout_ms) {
     bool printLive = true;    // true (new feature): printing transcrition letter during Audio Out
     return RealtimeAPI_StartLLM_GetAudio (timeout_ms, printLive);
}


// New function: return current Realtime Voice
String RealtimeAPI_CurrentCharacterVoice() {
  RealtimeAPI_NormalizeCharacterIndex();
  return (String) FRIENDS[gl_CURR_FRIEND].realtime_voice;
}


// New function: return server response as TXT (Audio transcription) after AUDIO played
String RealtimeAPI_GetAudioTranscription() {
  String out = realtime_audio_transcription;
  realtime_audio_transcription = "";
  return out;
}


// New function: allows Main.ino to trigger LED when user speaks
bool RealtimeAPI_isListening(){
  return ( realtime_local_vad_open );
}


// New function: STOP Realtime AUDIO via button (called in while loop of RealtimeAPI_StartLLM_GetAudio)
void RealtimeAPI_ForceStopOutput() {
  realtime_audio_cancelled_by_button = true;
  if (realtime_api_connected)
  {  realtime_ws.sendTXT("{\"type\":\"response.cancel\"}");
  }
  RealtimeAudioOut_Stop();
  realtime_waiting_response = false;
  realtime_response_ready = true;
  DebugPrintln("\n< STOP: output cancelled via BUTTON >");
}


// New function: Check if BUTTON pressed (or TOUCH btn touched)
bool RealtimeAPI_StopButtonActive() {
  if (pin_RECORD_BTN != NO_PIN && digitalRead(pin_RECORD_BTN) == LOW) { return true; }
  if (pin_TOUCH != NO_PIN)
  {  uint32_t current_touch = touchRead(pin_TOUCH);
     if (current_touch < 16383)  // ESP32
     {  if (current_touch <= (uint32_t)(gl_TOUCH_RELEASED * 0.9))     { return true; }
     }  else  // ESP32-S3
     {  if (current_touch >  (uint32_t)(gl_TOUCH_RELEASED * 1.1))     { return true; }
     }
  }
  return false;
}


// New function: Check current Audio Volume for REALTIME (supporting OPTIONAL POTI/BTN/LED, use DEFAULT on missed controls)
int RealtimeAPI_AudioVolumePercentage() {

  static int volume_percentage = REALTIME_OUTPUT_GAIN_INIT;  // default (if no VOL control connected)

  if (pin_VOL_POTI != NO_PIN)  // use VOL POTI only (if available)
  {  volume_percentage = map( analogRead(pin_VOL_POTI), 0, 4095, 0, REALTIME_OUTPUT_GAIN_MAX );
     return volume_percentage;
  }
  if (pin_VOL_BTN != NO_PIN)   // if no POTI available: N step toggle (flashing) with VOL BTN (50/75/100%)
  {  static bool flg_volume_updated = false;
     int vsteps=REALTIME_OUTPUT_GAIN_BTN_STEPSIZE, vmin=REALTIME_OUTPUT_GAIN_BTN_MIN, vmax=REALTIME_OUTPUT_GAIN_MAX;
     if (digitalRead(pin_VOL_BTN) == LOW && !flg_volume_updated)  // once per press
     {  volume_percentage += vsteps;
        if (volume_percentage < vmin || volume_percentage > vmax) { volume_percentage = vmin; }

        // visualize 'new level' with 1-N flashes in ## RED (hint: RED to distinguish from Audio.H vol GREEN/MAGENTA)
        // Flexible WEAK function led_RGB(): Calling only if exists (weak avoids compiler errors if not exist)

        if ( led_RGB )
        {  for (int i=0; i<=( (volume_percentage - vmin) / vsteps ); i++)  // short ## RED flashes (1-N times)
           { led_RGB(LOW,HIGH,HIGH); delay(20); led_RGB(HIGH,HIGH,HIGH); delay(40);
           }
           led_RGB(HIGH,HIGH,LOW); // if done: keep LED: ## BLUE (Realtime speaking)
        }

        DebugPrintln( "\n< Updated Realtime Audio Volume [%]: " + (String) volume_percentage + " >" );
        flg_volume_updated = true;
     }
     if (digitalRead(pin_VOL_BTN) == HIGH && flg_volume_updated)  // btn release
     {  flg_volume_updated = false;
     }
  }
  return volume_percentage;
}


// New function: Delete last User request from Conversation queue (use case: e.g. key commands)
bool RealtimeAPI_DeleteLastConversationItem() {
  if (!realtime_api_connected || !realtime_session_configured) {
    return false;
  }
  if (realtime_last_item_id == "") {
    return false;
  }
  RealtimeAPI_DeleteConversationItem(realtime_last_item_id);
  realtime_last_item_id = "";
  return true;
}

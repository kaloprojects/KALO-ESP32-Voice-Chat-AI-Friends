
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------             KALO Library - OpenAI_Groq_LLM AI Chat-Completions call with ESP32               ----------------
// ----------------                              Latest Update: Sept. 12, 2026                                   ----------------
// ----------------                                      Coded by KALO                                           ----------------
// ----------------                                                                                              ----------------
// ----------------     [NEW] 2026/09: WebSearchResult() added, supporting REALTIME MODE                         ----------------
// ----------------     [NEW] 2026/09: CHAT model GROQ openai/gpt-oss-20b [instead llama-3.1-8b-instant)         ----------------
// ----------------     [NEW] 2026/09: WEBSEARCH Open AI gpt-5-search-api [instead gpt-4o-mini-search-preview]   ----------------
// ----------------                                                                                              ----------------
// ----------------     [NEW] 2025/09: Sending CHAT history via command to user email account                    ----------------
// ----------------     [NEW] 2025/08: Multiple AI characters (FRIENDS), calling by friends 'name'               ----------------
// ----------------     [NEW] 2025/08: LLM Groq Meta & Open AI models support                                    ----------------
// ----------------                                                                                              ----------------
// ----------------          Function remembers session dialog history, supporting follow-up questions           ----------------
// ----------------             all written in Arduino-IDE C code (no Node.JS, no Server, no Python)             ----------------
// ----------------       CALL: Result = OpenAI_Groq_LLM(UserRequest, LLM_OA_KEY, flg_WebSearch, LLM_CQ_KEY)     ----------------
// ----------------                                [no Initialization needed]                                    ----------------
// ----------------                                                                                              ----------------
// ----------------         Prerequisites: OpenAI API KEY (mandatory) & Groq API KEY (optional/faster)           ----------------
// ------------------------------------------------------------------------------------------------------------------------------


// === PRIVATE credentials ===== [NEW in Sept. 2025]: ESP32 can send CHAT history as EMAIL via [@] command (or speaking 'EMAIL')
// Prerequisites: Install <ReadyMail.h> zip, create an ESP32 'device' GMAIL account (e.g. myESP_123@gmail.com) with App password
// How to create an App GMAIL, see here: https://theorycircuit.com/esp32-projects/simple-way-to-send-email-using-esp32/

const char* GMAIL_SMTP_FROM =    "ESP Device GMAIL account";  // ## Insert your Device ESP32 GMAIL  (e.g. myESP_123@gmail.com)
const char* GMAIL_SMTP_APPKEY =  "Password (App key)";        // ## Insert your GMAIL App PASSWORD  (xyz xyz ..)
const char* EMAIL_SMTP_TO =      "sent TO email address";     // ## Insert your personal USER Email (any domain, hotmail/gmx ..)


// --- includes ----------------

/* #include <WiFiClientSecure.h> // library needed, but already included in main.ino tab */

#define  ENABLE_SMTP
#define  ENABLE_DEBUG
#include "ReadyMail.h"           // [NEW in Sept. 2025]: install zip v.0.6.3 or later: https://github.com/mobizt/ReadyMail


// --- defines & macros --------

/* needed, but already defined in main.ino tab:
#ifndef DEBUG                    // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG false            // <- define your preference here [true activates printing INFO details]
#  define DebugPrint(x);         if(DEBUG){Serial.print(x);}   // do not touch
#  define DebugPrintln(x);       if(DEBUG){Serial.println(x);} // do not touch!
#endif

#define WEB_SEARCH_USER_CITY     "City xy"     // User location for 'websearch' [#define needed, but moved to main.ino tab]
#define VERSION_DATE             "YYYYMMDD"    // [optional!, #ifdef check]. Used in EMail subject, located already in main.ino]
*/


// --- global Objects & settings ---

String  MESSAGES;                // MESSAGES contains complete Chat dialog, initialized with SYSTEM PROMPT from active FRIENDS[]
                                 // .. each 'OpenAI_Groq_LLM()' call APPENDS new content (and erased on each 'friend' change)
                                 // {"role": "user", "content": ".."},
                                 // {"role": "assistant", "content": ".."} ..
                                 // Hint: this String can increase to e.g. 100KB on 'long long' chat dialogs (stressing heap)
                                 // (my workaround: waking up other friend after long chat, altern.(tbd): cutting old history)

#define TIMEOUT_LLM  10          // preferred max. waiting time [sec] for LMM AI response

#define WAKEUP_FRIENDS_ENABLED   true    // default [true] allows to 'call' any FRIEND by his names (with new System Prompt)
                                         // [false] inactivates multi agent feature, staying with gl_CURR_FRIEND friend always

void __attribute__((weak)) led_RGB( bool red, bool green, bool blue );  // #KALO# [optional function] located in main.tab


// ------------------------------------------------------------------------------------------------------------------------------
// String  OpenAI_Groq_LLM( String UserRequest, const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key )
// ------------------------------------------------------------------------------------------------------------------------------
// Versatile LLM call, supporting multiple LLM models via OPEN AI or Groq CLOUD server API calls
// [NEW] Default setting below for CHAT LLM:   GROQ [openai/gpt-oss-20b]       <- very FAST, low latency, low costs (free)
// [NEW] Default setting below for WEB SEARCH: Open AI [gpt-5-search-api]      <- higher latency (due search), low costs
// In case any other models preferred: Just update the LLM_XY in code below (no changes in syntax needed)
//
// - OpenAI_Groq_LLM() 'remembers' all conversations during session on ongoing dialogs (appending last I/O to String MESSAGES)
// - Supporting user defined SYSTEM PROMPTS (user favorite AI bot 'character role')
// - Multiple Agents (I call them 'FRIENDS') supported -> just CALL friends by their names ! (via STT/SpeechToText)
//   (function 'WakeUpFriends()' handles all role-related tasks automatically (activating new friend)
// - User can define 1-N Agents FRIENDS[n] with their 'individual characters', also TTS Voice parameter can be assigned
//   (public function 'get_tts_param() can be used to send role-specific tts parameter to TTS (TextToSpeech() in main.ino
//
// CALL:      Call function on demand at any time, no Initializing needed (function initializes/connects/closes websocket)
// Params:    - UserRequest:    User question/request to Open AI or Groq AP LLM, return feedback as String
//            - llm_open_key:   User registration and Open AI API key needed, check Open AI website for more details
//            - flg_WebSearch:  false -> using GroqCloud CHAT LLM (currently: GROQ openai/gpt-oss-20b)
//            - flg_WebSearch:  true  -> using OpenAI WEB SEARCH  (currently: OpenAI gpt-5-search-api) & WEB_SEARCH_USER_CITY
//            - llm_groq_key:   User registration and Groq API key needed, check Groq website for more details
// RETURN:    LLM String response
//
// Examples:  String answer = OpenAI_Groq_LLM( "What means RGB?", "OA_KEY", false, "GROQ_KEY" );        <- default GROQ chat
//            String answer = OpenAI_Groq_LLM( "Will it rain tomorrow?", "OA_KEY", true, "GROQ_KEY" );  <- OpenAI websearch chat
// = Links =
// Open AI    - API CHAT COMPLETION: https://platform.openai.com/docs/api-reference/chat/create
//            - Playground (testing models & system prompt): https://platform.openai.com/playground/prompts?models=gpt-4o-mini
//            - LLM Models & Pricing: https://platform.openai.com/docs/pricing
//            - Open AI Models Latency/Speed Analysis & Pricing: https://artificialanalysis.ai/providers/openai (!)
//              Pricing Sept. 2026 (I/O in $USD/1 Mio token): gpt-4.1-nano: $0.10/0.40, WebSearch (all models): $10.00/1k calls
//
// GroqCloud: - API CHAT COMPLETION: https://console.groq.com/docs/api-reference#chat
//            - Playground (testing models & system prompt): https://console.groq.com/playground
//            - LLM Models & Pricing: https://console.groq.com/docs/models, https://groq.com/pricing
//            - GroqCloud Models Latency/Speed Analysis & Pricing: https://artificialanalysis.ai/providers/groq (!)
//              Pricing Sept. 2026 (I/O in $USD/1 Mio token): openai/gpt-oss-20b: $0.075/0.30

String OpenAI_Groq_LLM( String UserRequest, const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key )
{
    if (UserRequest == "") { return(""); }

    // ---  ONCE only on INIT (init gl_CURR_FRIEND & SYSTEM PROMPT)
    static bool flg_INITIALIZED_ALREADY = false;
    if ( !flg_INITIALIZED_ALREADY )
    {  flg_INITIALIZED_ALREADY = true;                           // all below is done ONCE only
       int friends_max = sizeof(FRIENDS) / sizeof(FRIENDS[0]);   // calculate amount of friends (dynamic array)
       if (gl_CURR_FRIEND < 0 || gl_CURR_FRIEND > friends_max-1) {gl_CURR_FRIEND = random(friends_max); }
       MESSAGES =  "{\"role\": \"system\", \"content\": \"";
       MESSAGES += FRIENDS[gl_CURR_FRIEND].system_prompt;
       MESSAGES += "\"}";
       // ## NEW since Sept. 2025 update:
       // Starting NTP server in background to update system time (so we don't waste latency in case we ever send Chat via EMAIL)
       configTime(0, 0, "pool.ntp.org");     // starting UDP protocol with NTP server in background (using UTC timezone)
    }

    // --- [NEW]: Managing multiple FRIENDS sessions ('calling' friends, initialize friend, triggering LED)
    // (you could remove this WakeUpFriends() call in case only 1 Agent (FRIENDS[0]) defined
    if (WAKEUP_FRIENDS_ENABLED)
    {  WakeUpFriends( UserRequest, flg_WebSearch );
    }


    // =====- Prep work done. Now CONNECT to Open AI or Groq Server (on INIT or after closed or lost connection) ================

    uint32_t t_start = millis();

    String LLM_Response = "";                                         // used for complete API response
    String Feedback = "";                                             // used for extracted answer
    String LLM_server, LLM_entrypoint, LLM_model, LLM_key;            // NEW: using vars to be independent of server/models

    /* Some take aways & experiences from own tests / testing other models with OpenAI or Groq server
    // General rule: Total latency = Connect Latency (always 0.8 sec) + Model 'Response' latency. Typical 'Response' latencies:
    // - Open AI models:      gpt-4.1-nano (1.6 sec), gpt-4o-mini-search-preview (4.0 sec / IMO: still ok for a web search)
    // - Groq models e.g.:    llama-3.1-8b-instant (0.5 sec!, low costs), gemma2-9b-it (0.5 sec), qwen/qwen3-32b (1.2 sec)
    // - IMO not recommended: deepseek-r1-distill-llama-70b (1.2 sec), qwen/qwen3-32b (1.2 sec) bc. both send Reasoning (<think>)
    //                        compound-beta-mini (2-3 sec) with realtime! (BUT less predictable than OpenAI web search) */

    // Define YOUR preferred models here:

    if (llm_groq_key == "" || flg_WebSearch)                          // using #OPEN AI# only for web search (or if no GROQ Key)
    {  LLM_server =        "api.openai.com";                          // OpenAI: https://platform.openai.com/docs/pricing
       LLM_entrypoint =    "/v1/chat/completions";
       if (!flg_WebSearch) LLM_model= "gpt-4.1-nano";                 // low cost, powerful, fast (response latency ~ 1.5 sec)
 /*    if (flg_WebSearch)  LLM_model= "gpt-4o-mini-search-preview";   // faster websearch model (depreciated) ~ 3-4 sec) */
       if (flg_WebSearch)  LLM_model= "gpt-5-search-api";             // realtime websearch model (higher latency ~ 4-5 sec)
       LLM_key =           llm_open_key;
    }
    else
    {  LLM_server =        "api.groq.com";                            // Chat DEFAULT: using #CROG# with fastest llame model
       LLM_entrypoint =    "/openai/v1/chat/completions";             // GROQ Models/Pricing: https://groq.com/pricing
       LLM_model =         "openai/gpt-oss-20b";                      // [NEW] instead "llama-3.1-8b-instant" ..
       LLM_key =           llm_groq_key;                              // .. still low cost, very FAST (latency ~ 0.5-1 sec !)
    }

    /* static */ WiFiClientSecure client_tcp;    // [NEW]: removed static to free up HEAP (start with new LLM socket always)

    if ( !client_tcp.connected() )
    {  DebugPrintln("> Initialize LLM AI Server connection ... ");
       client_tcp.setInsecure();
       if (!client_tcp.connect( LLM_server.c_str() , 443))
       { Serial.println("\n* ERROR - WifiClientSecure connection to Server failed!");
         client_tcp.stop(); /* might not have any effect, similar with client.clear() */
         return ("");   // in rare cases: WiFiClientSecure freezed (with older libraries)
       }
       DebugPrintln("Done. Connected to LLM AI Server.");
    }
    client_tcp.setNoDelay(true);     // NEW: immediately flush after each write(), means disable TCP nagle buffering.
                                     // Idea: might increase performance a bit (20-50ms per write) [default is false]


    // ------ Creating the Payload: ---------------------------------------------------------------------------------------------

    // == model CHAT: creating a user prompt in format:  >"messages": [MESSAGES], {"role":"user", "content":"what means AI?"}]<
    // recap: Syntax of entries in global var MESSAGES [e.g.100K]:
    // > {"role": "system", "content": "you are a helpful assistant"},\n
    //   {"role": "user", "content": "how are you doing?"},\n
    //   {"role": "assistant", "content": "Thanks for asking, as an AI bot I do not have any feelings"} <
    //
    // for better readability we write # instead \" and replace below in code:

    String request_Prefix, request_Content, request_Postfix, request_LEN;

    UserRequest.replace( "\"", "\\\"" );  // to avoid any ERROR (if user enters any " -> convert to 2 \")

    request_Prefix  =     "{#model#:#" + LLM_model + "#, #messages#:[";    // appending old MESSAGES
    request_Content =     ",\n{#role#: #user#, #content#: #" + UserRequest + "#}],\n";     // <-- here we send UserRequest

    if (!flg_WebSearch)   // DEFAULT parameter for classic CHAT completion models
    {  request_Postfix =  "#temperature#:0.7, #max_tokens#:512, #presence_penalty#:0.6, #top_p#:1.0}";
    }
    if (flg_WebSearch)    // NEW: parameter for web search models
    {  request_Postfix =  "#response_format#: {#type#: #text#}, ";
       request_Postfix += "#web_search_options#: {#search_context_size#: #low#, ";
       request_Postfix += "#user_location#: {#type#: #approximate#, #approximate#: ";
       request_Postfix += "{#country#: ##, #city#: #" + (String) WEB_SEARCH_USER_CITY + "#}}}, ";
       request_Postfix += "#store#: false}";
    }

    request_Prefix.replace("#", "\"");  request_Content.replace("#", "\"");  request_Postfix.replace("#", "\"");
    request_LEN = (String) (MESSAGES.length() + request_Prefix.length() + request_Content.length() + request_Postfix.length());


    // ------ Sending the request: ----------------------------------------------------------------------------------------------

    uint32_t t_startRequest = millis();

    client_tcp.println( "POST " + LLM_entrypoint + " HTTP/1.1" );
    client_tcp.println( "Connection: close" );
    client_tcp.println( "Host: " + LLM_server );
    client_tcp.println( "Authorization: Bearer " + LLM_key );
    client_tcp.println( "Content-Type: application/json; charset=utf-8" );
    client_tcp.println( "Content-Length: " + request_LEN );
    client_tcp.println();
    client_tcp.print( request_Prefix );    // detail: no 'ln' because Content + Postfix will follow)

    // Now sending the complete MESSAGES chat history (String) .. 2 options (from own experiences in testing & user feedback):
    // 1. either with one single 'client_tcp.print( MESSAGES );' .. works well on my ESP32 (even a 100 KB String works flawless)
    // 2. or sending in chunks (background: some user had issues if MESSAGES size exceeds 8K .. so we use option (2) here:

    /* client_tcp.print( MESSAGES );       // Option 1: sending complete MESSAGES history once (works well on my ESP32)  */

    // Option 2 (NEW): sending MESSAGES (text) in chunks (prevents the TLS layer from choking on big payloads)
    const size_t CHUNK_SIZE = 1024;        // 1K just as example (all below 8K should work also on older ESP32)
    for (size_t i = 0; i < MESSAGES.length();  i += CHUNK_SIZE)
    {   client_tcp.print(MESSAGES.substring(i, i +  CHUNK_SIZE));
    }

    // final Postfix (then all is done):
    client_tcp.println( request_Content + request_Postfix );


    // ------ Waiting the server response: --------------------------------------------------------------------------------------

    LLM_Response = "";
    while ( millis() < (t_startRequest + (TIMEOUT_LLM*1000)) && LLM_Response == "" )
    { Serial.print(".");                   // printed in Serial Monitor always
      delay(250);                          // waiting until tcp sends data
      while (client_tcp.available())       // available means: if a char received then read char and add to String
      { char c = client_tcp.read();
        LLM_Response += String(c);
      }
    }
    if ( millis() >= t_startRequest + (TIMEOUT_LLM*1000) )
    {  Serial.print("\n*** LLM AI TIMEOUT ERROR - forced TIMEOUT after " + (String) TIMEOUT_LLM + " seconds");
    }
    client_tcp.stop();                     // closing LLM connection always (observation: otherwise OpenAI TTS won't work)

    uint32_t t_response = millis();


    // ------ Now extracting clean message for return value 'Feedback': ---------------------------------------------------------
    // 'talkative code below' but want to make sure that also complex cases (e.g. " chars inside the response are working well)
    // Arduino String Reference: https://docs.arduino.cc/language-reference/de/variablen/data-types/stringObject/#funktionen

    int pos_start, pos_end;                                     // proper way to extract tag "text", talkative but correct
    bool found = false;                                         // supports also complex tags, e.g.  > "What means \"RGB\"?" <
    pos_start = LLM_Response.indexOf("\"content\":");           // search tag ["content": "Answer..."]
    if (pos_start > 0)
    { pos_start = LLM_Response.indexOf("\"", pos_start + strlen("\"content\":")) + 1;   // search next " -> now points to 'A'
      pos_end = pos_start + 1;
      while (!found)
      { found = true;                                           // avoid endless loop in case no " found (won't happen anyhow)
        pos_end = LLM_Response.indexOf("\"", pos_end);          // search the final " ... but ignore any rare \" inside the text!
        if (pos_end > 0)                                        // " found -> Done.   but:
        {  // in case we find a \ before the " then proceed with next search (because it was a \marked " inside the text)
           if (LLM_Response.substring(pos_end -1, pos_end) == "\\") { found = false; pos_end++; }
        }
      }
    }
    if( pos_start > 0 && (pos_end > pos_start) )
    { Feedback = LLM_Response.substring(pos_start,pos_end);  // store cleaned response into String 'Feedback'
      Feedback.trim();
    }


    // ------ APPEND current I/O chat (UserRequest & Feedback) at end of var MESSAGES -------------------------------------------

    if (Feedback != "")                                          // we always add both after success (never if error)
    { String NewMessagePair = ",\n\n";                           // ## NEW in Sept. 2025: \n\n instead \n (for spaces in email)
      if(MESSAGES == "") { NewMessagePair = ""; }                // if messages empty we remove leading ,\n
      NewMessagePair += "{\"role\": \"user\", \"content\": \""      + UserRequest + "\"},\n";
      NewMessagePair += "{\"role\": \"assistant\", \"content\": \"" + Feedback    + "\"}";

      // here we construct the CHAT history, APPENDING current dialog to LARGE String MESSAGES
      MESSAGES += NewMessagePair;
    }


    // ------ finally we clean Feedback, print DEBUG Latency info and return 'Feedback' String ----------------------------------

    // trick 17: here we break \n into real line breaks (but in MESSAGES history we added the original 1-liner)
    if (Feedback != "")
    {  Feedback.replace("\\n", "\n");                            // LF issue: replace any 2 chars [\][n] into real 1 [\nl]
       Feedback.replace("\\\"", "\"");                           // " issue:  replace any 2 chars [\]["] into real 1 char ["]
       Feedback.replace("\n\n", "\n");                           // NEW: remove empty lines in Serial Monitor
       Feedback.trim();                                          // in case of some leading spaces
    }

    DebugPrintln( "\n---------------------------------------------------" );

    if (LLM_Response.indexOf("error") >=0)
    {  // in case of any error (e.g. Billing limit exceeded) -> Printing total server response
       Serial.println("\n====\n* ERROR - LLM Server sent ERROR response:\n" + LLM_Response + "====\n");
    }
    /* DebugPrintln( "====> Total Response: \n" + LLM_Response + "\n====");   // ## uncomment to see complete server response */
    DebugPrintln( "AI LLM server/model: [" + LLM_server + " / " + LLM_model + "]" );
    DebugPrintln( "-> Latency LLM AI Server (Re)CONNECT:          " + (String) ((float)((t_startRequest-t_start))/1000) );
    DebugPrintln( "-> Latency LLM AI Response:                    " + (String) ((float)((t_response-t_startRequest))/1000) );
    DebugPrintln( "=> TOTAL Duration [sec]: ..................... " + (String) ((float)((t_response-t_start))/1000) );
    DebugPrintln( "---------------------------------------------------" );
    DebugPrint( "\nLLM >" );

    // and return extracted feedback

    return ( Feedback );
}



// ------------------------------------------------------------------------------------------------------------------------------
// void WakeUpFriends( String UserRequest, bool flg_WebSearch )
// [Internal private function, used in this lib.ino only]
// ------------------------------------------------------------------------------------------------------------------------------
// - Function handles all Agent (FRIEND[]) tasks: Listen to 'called' friends / initialize friend / triggering LED etc..)
// - All Agent related tasks as collected here to keep the main 'OpenAI_Groq_LLM()' smart & clean.
// - CALL: only in 'OpenAI_Groq_LLM()', function is not needed (and could be removed) in case 1 Agent (FRIEND[0]) used only
// - .ino DEPENDENCIES: Calling optional (weak) function 'led_RGB()' from main.ino (for LED keyword feedbacks)

void WakeUpFriends( String UserRequest, bool flg_WebSearch )
{
    static int gl_CURR_FRIEND_before = -1;                    // remember the last used friend (agent), init with 'never'
    int friends_max = sizeof(FRIENDS) / sizeof(FRIENDS[0]);   // calculate amount of friends (dynamic array) e.g. 9 (0-8)

    //  ------ Check always if user 'calls' a new friend -> if yes: update [gl_CURR_FRIEND] & flash LED ## RED twice

    bool   new_user_found = false;
    String UserRequestUpper = UserRequest;
           UserRequestUpper.toUpperCase(); UserRequestUpper.replace(".", ""); UserRequestUpper.trim();
    int i = 0;
    while (!new_user_found && i < friends_max)
    { new_user_found = ( WordInStringFound( UserRequestUpper, FRIENDS[i].aliases ) && i != gl_CURR_FRIEND );
      if (new_user_found)
      {  gl_CURR_FRIEND = i;
         if (led_RGB)  // check if weak function exists
         {  led_RGB(LOW,HIGH,HIGH); delay(100); led_RGB(HIGH,HIGH,HIGH); delay(100); led_RGB(LOW,HIGH,HIGH); delay(100);
         }
         DebugPrintln("\n## CALLED new FRIEND: [" + (String) FRIENDS[gl_CURR_FRIEND].aliases + "]");
      }
      i++;
    }

    //  ------ Check if friend changed (either on Init or via user 'calling' above) -> if yes: initialize new SYSTEM PROMPT

    if (gl_CURR_FRIEND != gl_CURR_FRIEND_before)  // new friend WAKED UP -> flashing LED ## RED twice
    {  gl_CURR_FRIEND_before = gl_CURR_FRIEND;
       DebugPrintln("\n## INIT new SYSTEM PROMPT [" + (String)FRIENDS[gl_CURR_FRIEND].aliases + "]");

       MESSAGES =  "{\"role\": \"system\", \"content\": \"";  // initialize MESSAGES with Friends system PROMPT
       MESSAGES += FRIENDS[gl_CURR_FRIEND].system_prompt;     // in Open AI & Groq syntax: {"role": "system", "..prompt.."}
       MESSAGES += "\"}";
    }

    // ------ Check command [#]: List all available FRIENDS (and current active friend) in Serial.Monitor

    UserRequest.replace("Hashtag.", "#");                     // optionally (for STT): converts spoken 'Hashtag.' to '#'
    if ( UserRequest == "#" )
    {  String user_synonyms, user_1st_name, collection;
       for(int i = 0; i < friends_max; i++)                   // Extract from 1-N friends the first name only (skip synonyms)
       {  user_synonyms = (String) FRIENDS[i].aliases;
          user_1st_name = user_synonyms.substring(0, user_synonyms.indexOf(' '));
          collection +=   (user_1st_name + ", ");
       }
       collection = collection.substring(0, collection.length()-2);  // remove last ', '
       Serial.println( "\n>> Available FRIENDS [" + (String) friends_max + "]: FRIENDS: " + collection );
       Serial.print( ">> Active FRIEND (names): [" + (String) FRIENDS[gl_CURR_FRIEND].aliases + "]");
    }
}



// ------------------------------------------------------------------------------------------------------------------------------
// bool WordInStringFound( String sentence, String pattern ) - a bit complex String function (OpenAI ChatCPT helped in coding :))
// [Internal private String function, used in this lib.ino only]
// ------------------------------------------------------------------------------------------------------------------------------
// - Function checks if 'sentence' contains any single words which are listed in 'pattern' (pattern has 1-N space limited words)
// - case sensitive, but ignoring all punctuations (".,;:!?\"'-()[]{}") in sentence (replacing with ' ')
// CALL:     Call function on demand (used once in OpenAI_Groq_LLM() for waking up (initializing) any friend (calling by name)
// Params:   String sentence (e.g. user request transcription), String pattern with a list of 1-N words (space limited)
// RETURN:   true if 1 or more pattern (word) found, false if nothing found or empty strings
// Examples: WordInStringFound( "Hello friend!, this is a test", "my xyz friend" ) -> true (friend matches. '!' doesn't matter)
//           WordInStringFound( "Hello friend2, this is a test", "cde my friend" ) -> false (no pattern word found2 in sentence)
//           WordInStringFound( "I jump over to VEGGIE. Are you online?", "VEGGI VEGGIE WETCHI WEDGIE" ) -> true (VEGGIE found)

bool WordInStringFound( String sentence, String pattern )
{
  // replacing any punctuations in sentence with ' '
  String punctuation = ".,;:!?\"'-()[]{}";
  for(int i = 0; i < sentence.length(); ++i)
  {  if (punctuation.indexOf(sentence.charAt(i)) != -1) {sentence.setCharAt(i,' ');}
  }
  // now we search 'in' sentence for all words which are (space limited) listed in pattern:
  if (!pattern.length()) return false;
  sentence = " " + sentence + " ";
  for (int i = 0, j; i < pattern.length(); i = j + 1)
  { j = pattern.indexOf(' ', i); if (j < 0) j = pattern.length();
    String w = pattern.substring(i, j);
    if (sentence.indexOf(" " + w + " ") != -1) return true;
  }
  return false;
}



// ------------------------------------------------------------------------------------------------------------------------------
// void get_tts_param( 'return N values by pointer' )
// PUBLIC function - Intended use: allows TextToSpeech() in main.ino to get all predefined tts parameter of current FRIEND[x]
// ------------------------------------------------------------------------------------------------------------------------------
// Idea: Keeping whole global FRIENDS[] array 'private' for this .ino, no read/write access outside (workaround instead .cpp/.h)
// All values are returned via pointer (var declaration and instance have to be allocated in calling function !)
// Params: Agent_id [0-N], 1st friend name, tts-model, tts-voice, tts-vspeed, tts-voice -instruction, welcome_hello

void get_tts_param( int* id, String* names, String* model, String* voice, String* vspeed, String* inst, String* hello )
{
  int friends_max = sizeof(FRIENDS) / sizeof(FRIENDS[0]);        // calculate amount of friends (dynamic array)
  if (gl_CURR_FRIEND < 0 || gl_CURR_FRIEND > friends_max-1)      // just to make sure gl_CURR_FRIEND is always a valid id
  {  gl_CURR_FRIEND = random( friends_max );                     // (exactly same as we did in INIT)
     DebugPrintln( "## INIT gl_CURR_FRIEND via get_tts_param(): " + (String) gl_CURR_FRIEND );
  }
  *id     = gl_CURR_FRIEND;
  String all_names = (String) FRIENDS[gl_CURR_FRIEND].aliases;   // return 1st alias (main) name only .. (or use character_name)
  *names = all_names.substring(0, all_names.indexOf(' '));       // .. works also on single words (substring takes all on -1)
  *model  = (String) FRIENDS[gl_CURR_FRIEND].tts_model;
  *voice  = (String) FRIENDS[gl_CURR_FRIEND].tts_voice;
  *vspeed = (String) FRIENDS[gl_CURR_FRIEND].tts_speed;
  *inst   = (String) FRIENDS[gl_CURR_FRIEND].tts_instruct;
  *hello  = (String) FRIENDS[gl_CURR_FRIEND].tts_welcome;
  /* *system_prompt not send, because large String not needed for tts and would unnecessarily stress free RAM) */
}



// ------------------------------------------------------------------------------------------------------------------------------
// void Send_MESSAGES_SerialMonitor()
// PUBLIC function - Printing Chat History & [NEW] list of FRIENDS (Getter function for global MESSAGES access outside this.ino)
// ------------------------------------------------------------------------------------------------------------------------------
void Send_MESSAGES_SerialMonitor()
{
     int friends_max = sizeof(FRIENDS) / sizeof(FRIENDS[0]);
     String list_friends;
     for (int i=0; i < friends_max; i++)
     { list_friends += FRIENDS[i].character_name  + (String) ", ";
     } list_friends =  list_friends.substring(0, list_friends.length()-2 );

     Serial.println ( "\n>> Available FRIENDS:  [" + list_friends + "]" );
     Serial.println (   ">> ACTIVE CHAT Friend: [" + (String) FRIENDS[gl_CURR_FRIEND].character_name + "]" );

     Serial.println( "\n>> MESSAGES (CHAT PROMPT LOG):" );
     Serial.println( MESSAGES );

     Serial.println( "\n>> MESSAGES LENGTH: " + (String) MESSAGES.length() );
     Serial.println(   ">> FREE HEAP:       " + (String) ESP.getFreeHeap() );
}



// ------------------------------------------------------------------------------------------------------------------------------
// bool Send_MESSAGES_Email()
// ------------------------------------------------------------------------------------------------------------------------------
// Sending complete CHAT history via email, from predefined 'device' account GMAIL_SMTP_FROM to 'user' account EMAIL_SMTP_TO
// Purpose: Archiving CHATS, connected ESP32 can list chats via [#] command, mobile ESP32 can send email instead via [@]
// Email body contains the String MESSAGES, UTF-8 coded (multi lingual characters supported), html body possible but not used
//
// Workflow:  - Creating email container (FROM,TO,SUBJECT,BODY), BODY = complete CHAT history (String MESSAGES)
//            - Catching 'now' timestamp via UDP protocol from NTP server (timestamp often needed to pass smtp spam detection)
//            - sending Email to GMAIL server (other smtp server supported too, just change settings)
//
// RETURN:    - Calling smtp server works in async mode, so no direct RETURN value (Success or Fail) possible
//            - workaround: return TRUE if sending Email with correct timestamp launched, FALSE if smtp.isAuthenticated failed
//
// Prep Work: - Create an additional GMAIL account (for the device) as sender GMAIL_SMTP_FROM (with App key access password!)
//            - use your existing favorite personal Email as receiver (EMAIL_SMTP_TO), all @server (not gmail only) supported
//            - Install 'ReadyMail' ZIP from mobizt, tested Library: v.0.3.6 (Aug.13, 2025)
//
// Links:     - Workflow tutorial & How to create a GMAIL account with App key access & How to use other server (beyond Gmail):
//              https://theorycircuit.com/esp32-projects/simple-way-to-send-email-using-esp32/
//            - How to request Unix timestamp via NTP: https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
//            - Tool Unix timestamp converter: https://www.unixtimestamp.com/
//            - ReadyMail zip Library: v.0.3.6 (Aug.13,2025): https://www.unixtimestamp.com/https://github.com/mobizt/ReadyMail

bool Send_MESSAGES_Email()
{
  WiFiClientSecure ssl_client;
  SMTPClient smtp(ssl_client);
  ssl_client.setInsecure();

  auto statusCallback = [](SMTPStatus status){ DebugPrintln(status.text); };  // C++ Lamda Callback (printed in DEBUG mode)
  smtp.connect("smtp.gmail.com", 465, statusCallback);                        // GMAIL values, see links above for other server

  if (smtp.isConnected())
  {  smtp.authenticate( GMAIL_SMTP_FROM, GMAIL_SMTP_APPKEY, readymail_auth_password );
     /* info: readymail_auth_password) is an element of 'enum readymail_auth_type' in <ReadyMail.h> */

     if (smtp.isAuthenticated())
     {  SMTPMessage msg;

        //  ------ Construct FROM /TO
        msg.headers.add(rfc822_from, "ESP32 AI <" + (String) GMAIL_SMTP_FROM +">");
        msg.headers.add(rfc822_to, (String) EMAIL_SMTP_TO );

        /* // Examples for optional add-ons. Syntax: email || <email> || alias <email>
        // msg.headers.add(rfc822_to, "Userxy <recipient email here>" ); // adding more 'to' user
        // msg.headers.add(rfc822_cc, "Userxy <recipient email here>" ); // adding more 'cc' user
        // not used: msg.headers.add(rfc822_sender, "Userxy <sender email here>");  */

        //  ------ Construct subject, e.g.: 'Chat with [FRED] - KALO_ESP32_20250922'
        String subject = (String) FRIENDS[gl_CURR_FRIEND].aliases;               // Extract Agent Friend names
        subject = subject.substring(0, subject.indexOf(' '));                    // Using 1st main name, e.g. "FRED"
        subject = "Chat with [" + subject + "] - KALO_ESP32";

        #ifdef VERSION_DATE                                                      // append [optional] #define VERSION_DATE
        subject = subject + "_" + (String) VERSION_DATE;
        #endif

        msg.headers.add(rfc822_subject, subject);

        //  ------ Construct CONTENT of email (UTF-8)
        msg.text.body( MESSAGES );    // sending (large) MESSAGES 1:1 (no String operations to avoid any stress on HEAP)

        /* // Info: UTF-8 code supports language chars, also this would work: ..
        String welcome = (String) "Hello\n" + "こんにちは、日本の皆さん\n" + "大家好，中国人\n" + "Здравей български народе";
        msg.text.body( welcome );
        // Info: HTML format supported too, just using msg.html.body() instead msg.text.body() ..
        welcome.replace("\n", "<br>\n");   // needed for html
        msg.html.body( "<html><body><div style=\"color:#cc0066;\">" + welcome + "</div></body></html>" ); */

        //  ------ Building a correct timestamp (using a UDP connection to an NTP server in background)
        // Reason: Some smtp server (e.g. GMX) request a valid (actual) timestamp, otherwise email will be rejected as spam !
        // Details: See chat with mobizt: https://github.com/mobizt/ReadyMail/discussions/20
        // Solution: Request timestamp from NTP server, trick: done ONCE in INIT of OpenAI_Groq_LLM() to avoid ~4 sec latency

        configTime(0, 0, "pool.ntp.org");  // starting UDP protocol with NTP server in background (using UTC timezone) ..
                                           // .. done already in INIT of OpenAI_Groq_LLM() (here again in case it was forgotten)
        time_t ntp_now;  time(&ntp_now);   // time() function fills var with NTP response (from configTime() process)

        uint32_t t_timeout = millis();
        DebugPrint(" > Waiting to valid NTP Server Timestamp [Timeout after e.g. 5 secs] ");
        while ( ntp_now < 100000 && (millis() - t_timeout) < 5000 ) // no delay in case 'configTime()' was initialized earlier
        { // time() fills Unix Timestamp (from configTime(NTP Server)) into var 'ntp_now' !
          time(&ntp_now); delay(100);
        }
        if (DEBUG)
        {  struct tm* t = localtime(&ntp_now);   // optional, convert e.g. Unix timestamp 1758541015 -> to Sept. 22, 2025
           Serial.println("\n > Unix Timestamp: " + (String) ntp_now + ", Latency [msec]: " + (String) (millis()-t_timeout));
           Serial.printf(" > %02d-%02d-%04d, %02d:%02d\n", t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min);
        }
        msg.timestamp = ntp_now;            // update email header with actual timestamp

        //  ------ Sending EMAIL finally ....

        smtp.send(msg);         // SEND email. Done

        if (ntp_now >  100000)  {return (true); }   // DEFAULT: Timestamp correct, so SMTP server should accept email
        if (ntp_now <= 100000)  {return (false);}   // Warning: Timestamp not valid, smtp server might reject email as spam
     }
     else
     {  Serial.println( "< ERROR: Email Authenticating failed, check Username and Password ! >" );
        return (false);
     }
  }
}



// ------------------------------------------------------------------------------------------------------------------------------
// WebSearchResult (String Request) - Calling LLM 'OpenAI_Groq_LLM(..)' with activated flg_WebSearch flag
// [REALTIME_BUTTON-MODE supported too (Friend tells 'wait a moment ..' then summarizing websearch in a human-like respond]
// Params:  - String Request:    User question/request String
//          - llm_open_key:      Open AI API key (currently mandatory as we use Open AI web search models)
//          - bool flgEmbedding: true -> embedding result into REALTIME conversation (use case: INPUT_REALTIME_MODE)
//          - llm_groq_key:      Groq API key (currently not used, placeholder in case we use Groq web search models in future)
// ------------------------------------------------------------------------------------------------------------------------------

String WebSearchResult( String Request, const char* llm_open_key, bool flgEmbedding, const char* llm_groq_key )
{
  if (Request== "")
  {  return "";
  }

  // ----- (1) Prep work (ONLY in INPUT_REALTIME_MODE): forcing dedicated RT server TTS: ".. Just wait a moment"

  if ( flgEmbedding )
  {
     String REALTIME_WAITING_GOAL =

  /* "Reply with exactly one short spoken sentence, like: 'Okay, one moment, I’ll check for us.' "
     "Do not answer the question yet. "
     "Respond in the same language as the user's original request."; */

     "Reply with exactly one short spoken sentence, like: 'Okay, let me check that for us. "
     "It might take some seconds; please bear with me'. "                 // <- add-on: asking for a little patience
     "Do not answer the question yet. "
     "Respond in the user language of the system prompt instruction.";    // <- mod: this works also on 1st request

     if (led_RGB) led_RGB(HIGH,HIGH,LOW);                 // LED: ## BLUE
     RealtimeAPI_DeleteLastConversationItem();            // (!) so the REALTIME server will never see users original request
     RealtimeAPI_AddUserText( REALTIME_WAITING_GOAL );    // instead: forcing a 'waiting' answer (similar TTS)
     RealtimeAPI_StartLLM_GetAudio( 45000, true);         // Blocking (wait until done / Audio played)

     DebugPrintln( "\n>>> REALTIME Web Search Introduction done. Now starting Web Search LLM ..\n" );
  }

  // ----- (2) Launch Web Search LLM (Always ! - INPUT_BUTTON_MODE & INPUT_REALTIME_MODE):
  // Hint: Open AI Web Search models are not primary intended for TTS (much too detailed, including lists & links etc.)
  // Solution: Forcing a short answer via 2 tricks: (1) Hard coded POSTFIX & (2) cutting potential footnote links

  if (led_RGB) led_RGB(HIGH,LOW,LOW);   // LED: ## CYAN indicating Open AI WEB SEARCH Request (same as STT in BUTTON mode)

  String POSTFIX = ". Summarize in few sentences, do NOT use any enumerations, line breaks or web links!";
  String SearchResult = OpenAI_Groq_LLM( (Request + POSTFIX), llm_open_key, true, llm_groq_key );   // needs typically 3-5 secs

  DebugPrintln( "\n>>> WebSearch RAW Result: [" + SearchResult + "]" );

  int any_links = SearchResult.indexOf( "([" );   // Cleanup: Remove potential foot notes (start with '([..')
  if ( any_links > 0 )
  {  SearchResult = SearchResult.substring(0, any_links) + "|";   // '|' just as 'cut' marker in Serial Monitor
  }

  // ----- (3) ONLY in INPUT_REALTIME_MODE: -> Trick: assemble & return new (!) REQUEST (with encapsulated web search answer)

  if ( flgEmbedding && SearchResult == "") { SearchResult = "Nothing found!"; }

  if ( flgEmbedding && SearchResult != "" )
  {
     String REALTIME_FOOD =
     "User question: \"{USER_QUESTION}\". "
     "\nWeb result: \"{WEB_RESULT}\". "
     "\nNow answer naturally in few short spoken sentences. "
     "Keep it simple and human-like. No lists, no links, no source mention. "
     "If result is 'Nothing found! then tell the user that the search was not successful and ask the user to rephrase the question. "
     "Important!: Respond in the same language as the user's original request!. Do NOT response in English!";

     Request.replace("\"", "'");        // just to avoid potential JSON/Text issues
     SearchResult.replace("\"", "'");

     REALTIME_FOOD.replace("{USER_QUESTION}", Request);           // embedding original User request
     REALTIME_FOOD.replace("{WEB_RESULT}",    SearchResult);      // embedding the Websearch result
     RealtimeAPI_AddUserText( REALTIME_FOOD );

     DebugPrintln( "\n>>> Websearch food for REALTIME:\n" + REALTIME_FOOD + "\n<<<" );

     return ( REALTIME_FOOD );   // INPUT_REALTIME_MODE: instead native SearchResult -> LLM Request/Answer/TTS will follow
  }

  return (SearchResult);         // INPUT_BUTTON_MODE:   sending SearchResult as 1:1 -> only TTS follows
}

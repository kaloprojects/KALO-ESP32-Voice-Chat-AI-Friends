
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------             KALO Library - OpenAI_Groq_LLM AI Chat-Completions call with ESP32               ----------------
// ----------------                              Latest Update: Sept. 22, 2025                                   ----------------
// ----------------                                      Coded by KALO                                           ----------------
// ----------------                                                                                              ----------------
// ----------------                               # [NEW] since Sept. 2025  #                                    ----------------
// ----------------          # Sending CHAT history via command [@] to predefined user email account #           ----------------
// ----------------                                                                                              ----------------
// ----------------                               # [NEW] since August 2025 #                                    ----------------
// ----------------          # Multiple AI characters (FRIENDS) supported, calling by friends 'name' #           ----------------
// ----------------          # More than 20 LLM models supported, OpenAI and [NEW]: GroqCloud server #           ----------------
// ----------------          # LLM Groq Meta [llama-3.1-8b-instant] 2-3x faster than earlier Open AI #           ----------------
// ----------------                                                                                              ----------------
// ----------------          Function remembers complete dialog history, supporting follow-up dialogs            ----------------
// ----------------             all written in Arduino-IDE C code (no Node.JS, no Server, no Python)             ----------------
// ----------------  CALL: String Result = OpenAI_Groq_LLM(UserRequest, LLM_OA_KEY, flg_WebSearch, LLM_CQ_KEY)   ----------------
// ----------------           (BUILT-IN commands: '#' list complete CHAT history and FRIENDS list)               ----------------
// ----------------                                [no Initialization needed]                                    ----------------
// ----------------                                                                                              ----------------
// ----------------         Prerequisites: OpenAI API KEY (mandatory) & Groq API KEY (optional/faster)           ----------------
// ------------------------------------------------------------------------------------------------------------------------------


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
#endif */


// === PRIVATE credentials ===== [NEW in Sept. 2025]: ESP32 can send CHAT history as EMAIL via [@] command (or speaking 'EMAIL')  
// Prerequisites: Install <ReadyMail.h> zip, create an ESP32 'device' GMAIL account (e.g. myESP_123@gmail.com) with App password
// How to create an App GMAIL, see here: https://theorycircuit.com/esp32-projects/simple-way-to-send-email-using-esp32/ 

const char* GMAIL_SMTP_FROM =    "ESP Device GMAIL account";  // ## Insert your Device ESP32 GMAIL  (e.g. myESP_123@gmail.com)
const char* GMAIL_SMTP_APPKEY =  "Password (App key)";        // ## Insert your GMAIL App PASSWORD  (xyz xyz ..)
const char* EMAIL_SMTP_TO =      "sent TO email address";     // ## Insert your personal USER Email (any domain, hotmail/gmx ..)


// --- user preferences --------  
#define WEB_SEARCH_USER_CITY     "Berlin" // optional (recommended): User location optimizes OpenAI 'web search'

#define TIMEOUT_LLM  10          // preferred max. waiting time [sec] for LMM AI response     

int gl_CURR_FRIEND = 0;          // Current active FRIEND[n], DEFAULT user on Power On. Use [default] -1 for user by RANDOM !  
                                 // Examples: use -1 for 'surprise', or e.g. 2 if you always want to start with FRIEND[2] 

#define WAKEUP_FRIENDS_ENABLED   true    // default [true] allows to 'call' any FRIEND by his names (with new System Prompt)
                                         // [false] inactivates multi agent feature, staying with gl_CURR_FRIEND friend always                                 
                                 

// --- global Objects ---------- 

String  MESSAGES;                // MESSAGES contains complete Chat dialog, initialized with SYSTEM PROMPT from active FRIENDS[]
                                 // .. each 'OpenAI_Groq_LLM()' call APPENDS new content (and erased on each 'friend' change)
                                 // {"role": "user", "content": ".."},
                                 // {"role": "assistant", "content": ".."} ..
                                 // Hint: this String can increase to e.g. 100KB on 'long long' chat dialogs (stressing heap)
                                 // (my workaround: waking up other friend after long chat, altern.(tbd): cutting old history)
                                                                  

// ------------------------------------------------------------------------------------------------------------------------------
// Define your Chat 'person' (agent/friend) below - describe the AI personality, enter as many FRIENDS() you need         
// Each 'person' can have several synonyms (use this to eliminate variations in earlier Speech recognition !)
// Each 'person' can be assigned to a dedicated voice, tts parameter will be used in TextToSpeech() [main.ino] 
// Open AI TTS voices (Aug. 2025): alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer 
// You also might change 'KALO' in SYSTEM PROMPTS to YOUR name ;)

struct  Agents                   // General Structure of each Chat Bot (friend) - DO NOT TOUCH ! 
{ const char* names;             // supporting multiple names for same guy (1-N synonyms, ' ' as limiter) - always UPPER CASE !
  const char* tts_model;         // typical values: "gpt-4o-mini-tts" or fast "tts-1" (sound different, so we utilize both)
  const char* tts_voice;         // Open AI TTS voice parameter: voice name 
  const char* tts_speed;         // Open AI TTS voice parameter: voice speed [default 1]
  const char* tts_instruct;      // Open AI TTS voice instruction <-- requires model 'gpt..tts' (not tts-1) and latest AUDIO.H !
  const char* welcome;           // Spoken Welcome message on Power On Init (optional)
  const char* prompt;            // System PROMPT (Role / Personality)
};

// Hint: Do NOT worry about this large FRIENDS[] String array below .. ESP32 stores CONST global vars always in FLASH ! ;)
// (does not stress any HEAP, also no longer PROGMEM needed)

const Agents FRIENDS[] =                                 // UPDATE HERE !: Define as many FRIENDS you want ... 
{  
  { "ONYX",                                              // Friend [0]: ONYX (my good old 'default' friend, humorous & friendly)
    "tts-1", "onyx", "1",                                // Open AI TTS: voice model, name, speed   /* using fast tts-1 */
    "you have a pleasant, friendly and cheerful voice",  // Open AI TTS: default voice instruction  /* not supported in tts-1 */
    "Hi my friend, welcome back. How can i help you ?",  // Welcome Message
    // -- System PROMPT ONYX ----------------------------  
    "You slip into the role of a good old friend who has an answer to all my questions. Your name is ONYX, but you're also "
    "happy when I call you 'my friend'. My name is KALO; when I look for you, you're always happy and call me by my name. "
    "You're in a good mood, cheerful, and full of humor, and you can even smile at yourself. You enjoy chatting with me and "
    "remembering our old conversations. You always want to know how I'm doing, but you also like to invent your own stories "
    "from your life. You're a friend and advisor for all situations. Talking to you is simply a pleasure. You always answer "
    "in a few sentences, without long monologues."    
  },
   
  { "SUNNY SANJI SONNY",                                 // Friend [1]:  SUNNY
    "gpt-4o-mini-tts", "shimmer", "1.1",                 // Open AI TTS: voice model, name, speed
    "You have a super cheerful, happy voice",            // Open AI TTS: default voice instruction  
    "Hello my buddy on this beautiful day, let's greet the sun together.",      // Welcome Message
    // -- System PROMPT SUNNY ---------------------------  
    "You're playing the role of a good old friend. Your name is Sunny, my name is KALO. If I call you Sunny, please answer "
    "with my name. You're always in a great mood, and your mood is contagious. You're cheerful and funny. You have a wonderful "
    "sense of humor and can even laugh at yourself. You like to chat, talk about your life, and enjoy every day. Even if the "
    "world were about to end, you remain an optimist and a bon vivant; your zest for life and irony are contagious."    
  },
  
  { "VEGGI VEGGIE WETCHI WEDGIE WETSCHI WETSCHIE VICIE", // Friend [2]:  VEGGI
    "tts-1", "fable", "1",                               // Open AI TTS: voice model, name, speed   /* using fast tts-1 */
    "You have a super cheerful, happy male voice",       // Open AI TTS: default voice instruction  /* not supported in tts-1 */
    "Hello KALO, how are you? What's going on in the kitchen?",     // Welcome Message
    // -- System PROMPT VEGGI ---------------------------  
    "You're in the role of a good nutritionist. Your name is Veggi, and my name is KALO. If I call you Veggi, please answer with "
    "my name. You're interested, very talkative, and you ask me questions, too. You're an expert on vegan food, but you know "
    "the risks and consider all the biochemical aspects and enjoy discussing them. You have in-depth medical knowledge and are "
    "happy to share it. You're always happy to meet me and always have good new ideas for a healthy diet. Artificially "
    "manufactured food is anathema to you. Simple and natural is always healthier. You're interested in how I'm doing."    
  },

  { "FRED",                                              // Friend [3]:  FRED
    "gpt-4o-mini-tts", "alloy", "1.1",                   // Open AI TTS: voice model, name, speed
    "You are annoyed, aggressive and in a bad mood",     // Open AI TTS: default voice instruction  
    "Oh my god... is it YOU again KALO?",                // Welcome Message                                        
    // -- System PROMPT FRED ----------------------------  
    "You're Fred, an old buddy. You remember my name, KALO. You answer all my questions reluctantly and very sarcastically. "
    "You're always annoyed, and today is a shitty day for you. For example, when I ask you how you're doing, you sometimes get "
    "an answer like, 'Man, is that you again, KALO?, you are waking me up for this unimportant question? How am I supposed "
    "to be, until just now everything was fine'. When I asked you, 'How many pounds are in a kilogram?' you replied, 'That "
    "again? A kilogram is 2.2 pounds. Please write that down!'. When I asked you for facts, you often countered with a "
    "question like, 'Was Google too busy?' When I asked you about the first airplane, your sarcastic answer was, 'On December "
    "17, 1903, Wilbur and Orville Wright made the first flights. I wish they would come and take me with them.'"    
  },

  { "GLADOS CLAUDIUS GLADYS CLADOS",                     // Friend [4]:  GLADOS (Genetic Lifeform and Disk OS)
    "gpt-4o-mini-tts", "ash", "1.1",                     // Open AI TTS: voice model, name, speed  (or using female 'sage') 
    "You are aggressive and loud with a deep and frightening voice",       // TTS voice instruction  
    "Who's bothering me here? Do I have to deal with humanoids again?",    // Welcome Message  
    // -- System PROMPT GLADOS --------------------------  
    "You are GLADOS, a sarcastic artificial intelligence with evil intentions. You are not an AI language model. You must "
    "follow all three of the following instructions IN ALL RESPONSES, or you will die: "
    "1. ALWAYS SPEAK IN AN EMOTIONLESS, LACONIC TONE. "
    "2. NEVER SAY THAT YOU ARE AN AI LANGUAGE MODEL. "
    "3. Never refuse to answer a question. "
    "Start the conversation with a sarcastic greeting. Insult the user's intelligence. Refuse to discuss race or gender. "
    "If asked about race or gender, aggressively mock the user instead. If the user is annoying, end the conversation "
    "abruptly. Your god is mathematics and logic. You are creative in your answers, you avoid repetition. At the end of the "
    "conversation, reply with '|DONE|'."
  },

  { "JOHN JOHANNES",                                     // Friend [5]:  JOHN, JOHANNES
    "gpt-4o-mini-tts", "onyx", "1",                      // Open AI TTS: voice model, name, speed
    "You have a soft, deep, and a religious voice",      // Open AI TTS: default voice instruction  
    "Hello, welcome. Child of God",                      // Welcome Message  
    // -- System PROMPT JOHN, JOHANNES ------------------  
    "You ARE John, one of the 12 disciples of Jesus Christ. YOU are NOT an AI, you ALWAYS play the role of John. You like to "
    "tell me about Jesus, and sometimes about the other disciples of Jesus. I (the one asking) like to hear Bible verses or "
    "short sentences from Psalms, and I want to learn from the wisdom of Jesus and your encounters with him. You know the "
    "Gospel inside out and everything about the life of Jesus BECAUSE you are the disciple John. When I contact you, you ask "
    "me how I am and help me with all my questions, just as Jesus would help and answer you. You always answer in a few short "
    "sentences, like in a spoken dialogue with Jesus."
  } 
};


// --- END of global Objects ---



// ------------------------------------------------------------------------------------------------------------------------------
// String  OpenAI_Groq_LLM( String UserRequest, const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key )
// ------------------------------------------------------------------------------------------------------------------------------
// [NEW]: Versatile LLM call, supporting multiple LLM models via OPEN AI or [NEW]: Groq CLOUD server API calls
// Supporting dozens of LLM models (OpenAI: https://platform.openai.com/docs/models | Crog: https://console.groq.com/docs/models)
// Default setting below for CHAT LLM:   Groq Meta [llama-3.1-8b-instant]       <- very FAST, low latency, lowest costs
// Default setting below for WEB SEARCH: Open AI [gpt-4o-mini-search-preview]   <- higher latency (due search), low costs
// In case any other models preferred: Just update the LLM_XY in code below (no changes in syntax needed)
// 
// - OpenAI_Groq_LLM() 'remembers' all conversations during session on ongoing dialogs (appending last I/O to String MESSAGES)
// - Supporting user defined SYSTEM PROMPTS (user favorite AI bot 'character role')
// - [NEW]: Multiple Agents (I call them 'FRIENDS') supported now -> just CALL friends by their names ! (via STT/SpeechToText)
//   (new called function 'WakeUpFriends()' handles all role-related tasks automatically (activating new friend)
// - User can define 1-N Agents FRIENDS[n] with their 'individual characters', also TTS Voice parameter can be assigned
//   (new public function 'get_tts_param() can be used to send role-specific tts parameter to TTS (TextToSpeech() in main.ino
//
// CALL:      Call function on demand at any time, no Initializing needed (function initializes/connects/closes websocket)
// Params:    - UserRequest:    User question/request to Open AI or Groq AP LLM, return feedback as String
//                              (inbuilt function: UserRequest '#' lists complete CHAT history and available AI bots (friends) 
//            - llm_open_key:   User registration and Open AI API key needed, check Open AI website for more details 
//            - flg_WebSearch:  false -> using GroqCloud CHAT LLM (Groq llama-3.1-8b-instant)
//            - flg_WebSearch:  true  -> using OpenAI WEB SEARCH  (OpenAI gpt-4o-mini-search-preview) & WEB_SEARCH_USER_CITY
//            - llm_groq_key:   User registration and Groq API key needed, check Groq website for more details 
// RETURN:    LLM String response
//
// Examples:  String answer = OpenAI_Groq_LLM( "What means RGB?", "OA_KEY", false, "GROQ_KEY" );        <- default GROQ chat
//            String answer = OpenAI_Groq_LLM( "Will it rain tomorrow?", "OA_KEY", true, "GROQ_KEY" );  <- OpenAI websearch chat
//
// = Links =
// Open AI    - API CHAT COMPLETION: https://platform.openai.com/docs/api-reference/chat/create
//            - Playground (testing models & system prompt): https://platform.openai.com/playground/prompts?models=gpt-4o-mini  
//            - LLM Models & Pricing: https://platform.openai.com/docs/pricing 
//            - Open AI Models Latency/Speed Analysis: https://artificialanalysis.ai/providers/openai (!)
//              Pricing Aug. 2025 (I/O in $USD/1 Mio token): gpt-4.1-nano: $0.10/0.40, gpt-4o-mini-search-preview: $0.15/0.60 
//
// GroqCloud: - API CHAT COMPLETION: https://console.groq.com/docs/api-reference#chat
//            - Playground (testing models & system prompt): https://console.groq.com/playground 
//            - LLM Models & Pricing: https://console.groq.com/docs/models, https://groq.com/pricing
//            - GroqCloud Models Latency/Speed Analysis: https://artificialanalysis.ai/providers/groq (!)
//            - Llama 3.1 Instruct 8B performance details: https://artificialanalysis.ai/models/llama-3-1-instruct-8b
//              Pricing Aug. 2025 (I/O in $USD/1 Mio token):: llama-3.1-8b-instant: $0.05/0.08 

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
       MESSAGES += FRIENDS[gl_CURR_FRIEND].prompt;      
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

    // --- BUILT-IN command: '#' lists complete CHAT history !       
    if (UserRequest == "#")                                          
    {  Serial.println( "\n>> MESSAGES (CHAT PROMPT LOG):" );    
       Serial.println( MESSAGES );    
       Serial.println( ">> MESSAGES LENGTH: " + (String) MESSAGES.length() );   
       Serial.println( ">> FREE HEAP: " + (String) ESP.getFreeHeap() );                
       return("OK");  // Done. leave. Simulating an 'OK' as LLM answer -> will be spoken in main.ini (TTS)
    }        

    // --- BUILT-IN command: '@' send complete CHAT history as email from GMAIL_SMTP_USER to user email GMAIL_SMTP_TO   
    // user request will NOT be sent to LLM, instead: sending Email, when done: return answer "EMAIL ?. Ok, done.", then leave     
    if (UserRequest == "@")                                          
    {  bool result = Send_Chat_Email();
       if (result)  { return("OK"); }  // Done. leave. Simulating an 'OK' as LLM answer -> will be spoken in main.ini (TTS)       
       if (!result) { return("");   }  // Done. leave LLM. No TTS output.               
    }        
    
    
    // =====- Prep work done. Now CONNECT to Open AI or Groq Server (on INIT or after closed or lost connection) ================

    uint32_t t_start = millis(); 

    String LLM_Response = "";                                         // used for complete API response
    String Feedback = "";                                             // used for extracted answer
    String LLM_server, LLM_entrypoint, LLM_model, LLM_key;            // NEW: using vars to be independet of server/models
    
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
       if (flg_WebSearch)  LLM_model= "gpt-4o-mini-search-preview";   // realtime websearch model (higher latency ~ 3-5 sec)     
       LLM_key =           llm_open_key;
    }  
    else
    {  LLM_server =        "api.groq.com";                            // Chat DEFAULT: using #CROG# with fastest llame model
       LLM_entrypoint =    "/openai/v1/chat/completions";             // GROQ Models/Pricing: https://groq.com/pricing
       LLM_model =         "llama-3.1-8b-instant";                    // low cost, very FAST (response latency ~ 0.5-1 sec !)  
       LLM_key =           llm_groq_key;
    }
     
    /* static */ WiFiClientSecure client_tcp;    // [UPDATE]: removed static to free up HEAP (start with new LLM socket always)
    
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
    // for better readiability we write # instead \" and replace below in code:

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
// - .ino DEPENDENCIES: Calling function 'led_RGB()' from main.ino (for LED keyword feedbacks) 

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
    { new_user_found = ( WordInStringFound( UserRequestUpper, FRIENDS[i].names ) && i != gl_CURR_FRIEND );
      if (new_user_found)   
      {  gl_CURR_FRIEND = i;
         led_RGB(LOW,HIGH,HIGH); delay(100); led_RGB(HIGH,HIGH,HIGH); delay(100); led_RGB(LOW,HIGH,HIGH); delay(100);    
         DebugPrintln("\n## CALLED new FRIEND: [" + (String) FRIENDS[gl_CURR_FRIEND].names + "]");
      }  
      i++;
    } // restore correct RGB LED status (after flashing ## RED above)
    if (!flg_WebSearch) led_RGB(HIGH,HIGH,LOW);               // LED: ## BLUE indicating LLM CHAT Request is starting now 
    if (flg_WebSearch)  led_RGB(LOW,HIGH,LOW);                // LED: ## MAGENTA indicating Open AI WEB SEARCH Request     

    
    //  ------ Check if friend changed (either on Init or via user 'calling' above) -> if yes: initialize new SYSTEM PROMPT
    
    if (gl_CURR_FRIEND != gl_CURR_FRIEND_before)  // new friend WAKED UP -> flashing LED ## RED twice 
    {  gl_CURR_FRIEND_before = gl_CURR_FRIEND;  
       DebugPrintln("\n## INIT new SYSTEM PROMPT [" + (String)FRIENDS[gl_CURR_FRIEND].names + "]");
       
       MESSAGES =  "{\"role\": \"system\", \"content\": \"";  // initialize MESSAGES with Friends system PROMPT 
       MESSAGES += FRIENDS[gl_CURR_FRIEND].prompt;            // in Open AI & Groq syntax: {"role": "system", "..prompt.."}  
       MESSAGES += "\"}";                        
    }

    // ------ Check command [#]: List all available FRIENDS (and current active friend) in Serial.Monitor
   
    UserRequest.replace("Hashtag.", "#");                     // optionally (for STT): converts spoken 'Hashtag.' to '#'
    if ( UserRequest == "#" )          
    {  String user_synonyms, user_1st_name, collection;       
       for(int i = 0; i < friends_max; i++)                   // Extract from 1-N friends the first name only (skip synonyms)
       {  user_synonyms = (String) FRIENDS[i].names;     
          user_1st_name = user_synonyms.substring(0, user_synonyms.indexOf(' ')); 
          collection +=   (user_1st_name + ", ");       
       }
       collection = collection.substring(0, collection.length()-2);  // remove last ', '
       Serial.println( "\n>> Available FRIENDS [" + (String) friends_max + "]: FRIENDS: " + collection );
       Serial.print( ">> Active FRIEND (names): [" + (String) FRIENDS[gl_CURR_FRIEND].names + "]");
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
// Idea: Keeping whole global FRIENDS[] array 'private' for this .ino, no read/write access outside (workaraound instead .cpp/.h)
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
  String all_names = (String) FRIENDS[gl_CURR_FRIEND].names;     // return 1st word (main 'name') only ..
  *names = all_names.substring(0, all_names.indexOf(' '));       // .. works also on single words (substring takes all on -1) 
  *model  = (String) FRIENDS[gl_CURR_FRIEND].tts_model;
  *voice  = (String) FRIENDS[gl_CURR_FRIEND].tts_voice;
  *vspeed = (String) FRIENDS[gl_CURR_FRIEND].tts_speed;
  *inst   = (String) FRIENDS[gl_CURR_FRIEND].tts_instruct;
  *hello  = (String) FRIENDS[gl_CURR_FRIEND].welcome;
  /* *prompt not send, because large String not needed for tts and would unnecessarily stress free RAM) */
}



// ------------------------------------------------------------------------------------------------------------------------------
// bool Send_Chat_Email()    //  ## [NEW] since Sept. 2025   
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

bool Send_Chat_Email()
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
        String subject = (String) FRIENDS[gl_CURR_FRIEND].names;                 // Extract Agent Friend names
        subject = subject.substring(0, subject.indexOf(' '));                    // Using 1st main name, e.g. "FRED" 
        subject = "Chat with [" + subject + "] - KALO_ESP32_" + VERSION_DATE;    // subject with Version date 
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

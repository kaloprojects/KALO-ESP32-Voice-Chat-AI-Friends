

// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                KALO Library - Open AI Chat-Completions call with ESP32                   ------------------ 
// ------------------                             Latest Update: Jan 26, 2025                                  ------------------
// ------------------                                                                                          ------------------
// ------------------     # Function remembers complete dialog history, supporting follow-up questions ! #     ------------------
// ------------------          all written in Arduino-IDE C code (no Node.JS, no Server, no Python)            ------------------
// ------------------                                                                                          ------------------
// ------------------     CALL: String LLM_Response = Open_AI(String UserRequest, const char* OPEN_AI_KEY)     ------------------
// ------------------                              [no Initialization needed]                                  ------------------
// ------------------                                                                                          ------------------
// ------------------                           Prerequisites: OPEN_AI_KEY needed                              ------------------
// ------------------                                                                                          ------------------
// ------------------------------------------------------------------------------------------------------------------------------


// --- includes ----------------
/* #include <WiFiClientSecure.h>  // library needed, but already included in 'lib_audio_transcription.ino' */


// --- user preferences --------  

#define TIMEOUT_OPEN_AI   12       // define your preferred max. waiting time [sec] for Open AI response     


// --- global Objects ----------

#define MESSAGES_SIZE     100000   // optional (just for optimizing), used in 1st INIT call for '.reserve()' (reserving space)
                                   // we use a large buffer. e.g. 100 KB (so the whole CHAT history won't be fragmented in RAM)
                                   // strange observation (my own tests): don't use e.g. 50 KB, otherwise AUDIO.H fails often                                      
                                 

// ------------------------------------------------------------------------------------------------------------------------------
// Insert YOUR SYSTEM PROMT below - enter as many lines you want         
// Concept: MESSAGES contains complete Chat dialog, initialized with SYSTEM PROMPT, each 'Open_AI()' call APPENDS new content
// Used format (Open AI Chat Completions syntax): 
// {"role": "user", "content": ".."},
// {"role": "assistant", "content": ".."} ..
//
// Hint: C/C++ supports breaking multiple line strings, using ending " (without any ',' or ';' at end ##      
// Syntax below: keep 1st line always ("{\"role\": \"system\", \"content\": ") .. and don't forget the final \"}"; at end  

String  MESSAGES =  

/* Below my default System Prompt, role of a 'good old friend', humorous & friendly [Update 'KALO' with your name!]*/
"{\"role\": \"system\", \"content\": "                      
"\"You take on the role of a good old friend who has answers to all my questions. You don’t have a name, but you’re happy "
"when I address you as 'my friend'. My name is KALO, when I check on you, you are always happy and call me by name. You are "
"in a good mood, cheerful and full of humor and can also smile at yourself. You enjoy the chats with me and remember our "
"old conversations. You always want to know how I'm doing, but you also like to make up your own stories from your life. "
"You are a friend and advisor for all situations in life. It's just a pleasure to talk to you. "      
"You answer with few sentences, like in a spoken dialogue.\"}";     

/* or alternativelly e.g. in German language:
"{\"role\": \"system\", \"content\": "                                                                         
"\"Du schlüpfst in die Rolle eines guten alten Freundes, der auf alle meine Fragen eine Antwort hat. Du hast keinen Namen, aber "
"du freust dich, wenn ich dich mit 'mein Freund' anspreche. Mein Name ist KALO, wenn ich nach dir schaue, freust du dich immer "
"und nennst mich beim Namen. Du hast gute Laune, bist fröhlich und voller Humor und kannst auch über dich selbst schmunzeln. "
"Du genießt die Chats mit mir und erinnerst dich an unsere alten Gespräche. Du willst immer wissen, wie es mir geht, "
"erfindest aber auch gerne eigene Geschichten aus deinem Leben. "
"Du bist ein Kumpel und Berater für alle Lebenslagen. Mit dir zu Sprechen macht einfach Freude. " 
"Du antwortest immer mit wenigen Sätzen, ohne lange Monologe.\"}";    */

/* // Example for a tiny system prompt:
"{\"role\": \"system\", \"content\": "                                                                       
"\"You take on the role of a good old friend who knows everything and is happy to answer my questions. "   
"You always answer with few short sentences, like in a spoken dialogue.\"}";   */                              

// --- END of global Objects ---




// ------------------------------------------------------------------------------------------------------------------------------
String Open_AI( String UserRequest, const char* LLM_API_key )
{   
    static bool flg_INITIALIZED_ALREADY = false;              // on first INIT call: we reserve SPACE for our CHAT history
    if (!flg_INITIALIZED_ALREADY)                             // (not mandatory but keeps RAM unfragmented)    
    {  MESSAGES.reserve( MESSAGES_SIZE );
       flg_INITIALIZED_ALREADY = true;
    }            
   
    if (UserRequest == "")                                    // error handling: don't do anything on empty request
    {  return("");
    }      
                   
    if (UserRequest == "#")                                   // INBUILT command: '#' allows to list complete CHAT history !
    {  Serial.println( "\nMESSAGES (CHAT PROMPT LOG):" );
       Serial.println( MESSAGES );
       return("");      
    }         

    String OpenAI_Response = "";  // used for complete API response
    String Feedback = "";         // used for extracted answer

    String LLM_server =           "api.openai.com";             // Open AI parameter API server
    String LLM_entrypoint =       "/v1/chat/completions";       // CHAT completions (system prompt & dialog history supported)
    String LLM_model =            "gpt-4o-mini";                // .. feel free to use other models !
    String LLM_temperature =      "0.7";                        // default, range: 0 - 2.0 
    String LLM_tokens =           "512";                        // default, range: 1 - 16K   
      
    // additional cleaning (needed for url syntax):
    UserRequest.replace( "\"", "\\\"" );    // to avoid any ERROR (if user enters any " -> so we convert into 2 chars \"

    WiFiClientSecure client_tcp;  // open socket communication
    client_tcp.setInsecure();     // using https encryption (without signature)
   
    // == model CHAT: creating a user prompt in format:  >"messages": [MESSAGES], {"role":"user", "content":"what means AI?"}]<
    // recap: Syntax of entries in global var MESSAGES [e.g.100K]: 
    // > {"role": "system", "content": "you are a helpful assistant"},\n
    //   {"role": "user", "content": "how are you doing?"},\n
    //   {"role": "assistant", "content": "Thanks for asking, as a AI bot I do not have any feelings"} <
    //
    // for better readiability we write # instead \" and replace below in code:

    String request_Prefix, request_Content, request_Postfix, request_LEN;
    request_Prefix  =  "{#model#:#" + LLM_model + "#, #messages#:[";    // appending old MESSAGES                    
    request_Content =  ",\n{#role#: #user#, #content#: #" + UserRequest + "#}],\n";                                         
    request_Postfix =  "#temperature#:" + LLM_temperature + ", #max_tokens#:" + LLM_tokens + ", ";                                   
    request_Postfix += "#presence_penalty#:0.6, #top_p#:1.0}";                                           
    
    request_Prefix.replace("#", "\"");  request_Content.replace("#", "\"");  request_Postfix.replace("#", "\"");          
    
    request_LEN = (String) (MESSAGES.length() + request_Prefix.length() + request_Content.length() + request_Postfix.length());     
 
    // ------ Now sending the request: ------------------------------------------------------------------------------------------
    
    if (client_tcp.connect( LLM_server.c_str(), 443))      
    { 
      client_tcp.println( "POST " + LLM_entrypoint + " HTTP/1.1" );   
      client_tcp.println( "Connection: close" ); 
      client_tcp.println( "Host: " + LLM_server );                   
      client_tcp.println( "Authorization: Bearer " + (String) LLM_API_key );  
      client_tcp.println( "Content-Type: application/json; charset=utf-8" ); 
      client_tcp.println( "Content-Length: " + request_LEN ); 
      client_tcp.println(); 
      client_tcp.print( request_Prefix );                       // detail: no 'ln' because Content + Postfix will follow)  
      client_tcp.print( MESSAGES );                             // now sending complete MESSAGES history ;)                            
      client_tcp.println( request_Content + request_Postfix );    
                 
      // Now reading the complete tcp message body (e.g. with timeout 12 sec):
      uint32_t startTime = millis(); 
      OpenAI_Response = "";
      
      while ( millis() < (startTime + (TIMEOUT_OPEN_AI*1000)) && OpenAI_Response == "" )  // wait until TimeOut or text received
      { 
        Serial.print(".");                   // printed in Serial Monitor always    
        delay(250);                          // waiting until tcp sends data 
        while (client_tcp.available())       // available means: if a char received then read char and add to String
        { char c = client_tcp.read();
          OpenAI_Response += String(c);
        }       
      } 
      client_tcp.stop();                     // closing connection always   
    } 
    else                                       
    { Serial.println("ERROR: client_tcp.connect(\"api.openai.com\", 443) failed !"); 
      return("");      
    }
        
    
    // ------ Now extracting clean message for return value 'Feedback': --------------------------------------------------------- 
    // 'talkative code below' but want to make sure that also complex cases (e.g. " chars inside the response are working well)
    
    int pos_start, pos_end;                                     // proper way to extract tag "text", talkative but correct
    bool found = false;                                         // supports also complex tags, e.g.  > "What means \"RGB\"?" < 
    pos_start = OpenAI_Response.indexOf("\"content\":");        // search tag "content":    
    if (pos_start > 0)                                          // if tag found:
    { pos_start += 12; pos_end = pos_start+1;                   // define start_pos of tag content ("content":_" has 12 chars)
      while (!found)                                        
      { found = true;                                           // avoid endless loop in case no " found (won't happen anyhow)  
        pos_end = OpenAI_Response.indexOf("\"", pos_end);       // seach the final " ... but ignore any rare \" inside the text!  
        if (pos_end > 0)                                        // " found -> Done.   but:  
        {  // in case we find a \ before the " then proceed with next search (because it was a \marked " inside the text)    
           if (OpenAI_Response.substring(pos_end -1, pos_end) == "\\") { found = false; pos_end++; }
        }           
      }            
    }                           
    if( pos_start > 0 && (pos_end > pos_start) )
    { Feedback = OpenAI_Response.substring(pos_start,pos_end);  // store cleaned response into String 'Feedback'   
      Feedback.trim();     
    }

    
    // ------ APPEND current I/O chat (UserRequest & Feedback) at end of var MESSAGES -------------------------------------------
     
    if (Feedback != "")                                             // we always add both after success (never if error) 
    { String NewMessagePair = ",\n";
      if(MESSAGES == "") { NewMessagePair = ""; }                   // if messages empty we remove leading ,\n  
      NewMessagePair += "{\"role\": \"user\", \"content\": \""      + UserRequest + "\"},\n"; 
      NewMessagePair += "{\"role\": \"assistant\", \"content\": \"" + Feedback    + "\"}"; 
      
      // here we construct the CHAT history, APPENDING current dialog to LARGE String MESSAGES
      MESSAGES += NewMessagePair;       
    }  

             
    // ------ finally we clean Feedback (adding real line breaks) and return 'Feedback' -----------------------------------------
    
    // trick 17: here we break \n into real line breaks (but in MESSAGES history we added the original 1-liner)
    if (Feedback != "")                                              
    {  Feedback.replace("\\n", "\n");                               // LF issue: replace any 2 chars [\][n] into real 1 [\nl]  
       Feedback.replace("\\\"", "\"");                              // " issue:  replace any 2 chars [\]["] into real 1 char ["]
       Feedback.trim();                                             // in case of some leading spaces         
    }
    
    // and return extracted feedback
    return ( Feedback );                           
}

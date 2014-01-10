/*
 *
 * 2011-08-21 - moved to C++ by Simson Garfinkel.
 *
 * 2007-08-24 - other changes
 *
 *
Copyright (c) 2005 JSON.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <stdlib.h>
#include <stdint.h>

class json_checker {
    static const int stacksize=256;	// max stack
    uint8_t state;
    int maxtop;			// how far did we get
    int top;
    uint8_t stack[stacksize];
    bool reject;			// in reject mode
    int push(uint8_t mode);
    int pop(uint8_t mode);
public:
    uint32_t comma_count;		// number of commas, a measure of complexity
    json_checker();
    virtual ~json_checker(){};
    int check_char(int next_char);
    int done();
    bool check_if_done();
};


#define __   -1     /* the universal error code */

/*
    Characters are mapped into these 31 character classes. This allows for
    a significant reduction in the size of the state transition table.
*/

enum classes {
    C_SPACE,  /* space */
    C_WHITE,  /* other whitespace */
    C_LCURB,  /* {  */
    C_RCURB,  /* } */
    C_LSQRB,  /* [ */
    C_RSQRB,  /* ] */
    C_COLON,  /* : */
    C_COMMA,  /* , */
    C_QUOTE,  /* " */
    C_BACKS,  /* \ */
    C_SLASH,  /* / */
    C_PLUS,   /* + */
    C_MINUS,  /* - */
    C_POINT,  /* . */
    C_ZERO ,  /* 0 */
    C_DIGIT,  /* 123456789 */
    C_LOW_A,  /* a */
    C_LOW_B,  /* b */
    C_LOW_C,  /* c */
    C_LOW_D,  /* d */
    C_LOW_E,  /* e */
    C_LOW_F,  /* f */
    C_LOW_L,  /* l */
    C_LOW_N,  /* n */
    C_LOW_R,  /* r */
    C_LOW_S,  /* s */
    C_LOW_T,  /* t */
    C_LOW_U,  /* u */
    C_ABCDF,  /* ABCDF */
    C_E,      /* E */
    C_ETC,    /* everything else */
    NR_CLASSES
};

static int ascii_class[128] = {
/*
    This array maps the 128 ASCII characters into character classes.
    The remaining Unicode characters should be mapped to C_ETC.
    Non-whitespace control characters are errors.
*/
    __,      __,      __,      __,      __,      __,      __,      __,
    __,      C_WHITE, C_WHITE, __,      __,      C_WHITE, __,      __,
    __,      __,      __,      __,      __,      __,      __,      __,
    __,      __,      __,      __,      __,      __,      __,      __,

    C_SPACE, C_ETC,   C_QUOTE, C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_PLUS,  C_COMMA, C_MINUS, C_POINT, C_SLASH,
    C_ZERO,  C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT,
    C_DIGIT, C_DIGIT, C_COLON, C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,

    C_ETC,   C_ABCDF, C_ABCDF, C_ABCDF, C_ABCDF, C_E,     C_ABCDF, C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_LSQRB, C_BACKS, C_RSQRB, C_ETC,   C_ETC,

    C_ETC,   C_LOW_A, C_LOW_B, C_LOW_C, C_LOW_D, C_LOW_E, C_LOW_F, C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_LOW_L, C_ETC,   C_LOW_N, C_ETC,
    C_ETC,   C_ETC,   C_LOW_R, C_LOW_S, C_LOW_T, C_LOW_U, C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_LCURB, C_ETC,   C_RCURB, C_ETC,   C_ETC
};


/* Strangely , IN is defined on mingw */
#ifdef IN
#undef IN
#endif

/*
    The state codes.
*/
enum states {
    GO=0,  /* start    */
    OK,  /* ok       */
    OB,  /* object   */
    KE,  /* key      */
    CO,  /* colon    */
    VA,  /* value    */
    AR,  /* array    */
    ST,  /* string   */
    ES,  /* escape   */
    U1,  /* u1       */
    U2,  /* u2       */
    U3,  /* u3       */
    U4,  /* u4       */
    MI,  /* minus    */
    ZE,  /* zero     */
    IN,  /* integer  */
    FR,  /* fraction */
    E1,  /* e        */
    E2,  /* ex       */
    E3,  /* exp      */
    T1,  /* tr       */
    T2,  /* tru      */
    T3,  /* true     */
    F1,  /* fa       */
    F2,  /* fal      */
    F3,  /* fals     */
    F4,  /* false    */
    N1,  /* nu       */
    N2,  /* nul      */
    N3,  /* null     */
    NR_STATES
};


static int state_transition_table[NR_STATES][NR_CLASSES] = {
/*
    The state transition table takes the current state and the current symbol,
    and returns either a new state or an action. An action is represented as a
    negative number. A JSON text is accepted if at the end of the text the
    state is OK and if the mode is MODE_DONE.

                 white                                      1-9                                   ABCDF  etc
             space |  {  }  [  ]  :  ,  "  \  /  +  -  .  0  |  a  b  c  d  e  f  l  n  r  s  t  u  |  E  |*/
/*start  GO*/ {GO,GO,-6,__,-5,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*ok     OK*/ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*object OB*/ {OB,OB,__,-9,__,__,__,__,ST,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*key    KE*/ {KE,KE,__,__,__,__,__,__,ST,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*colon  CO*/ {CO,CO,__,__,__,__,-2,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*value  VA*/ {VA,VA,-6,__,-5,__,__,__,ST,__,__,__,MI,__,ZE,IN,__,__,__,__,__,F1,__,N1,__,__,T1,__,__,__,__},
/*array  AR*/ {AR,AR,-6,__,-5,-7,__,__,ST,__,__,__,MI,__,ZE,IN,__,__,__,__,__,F1,__,N1,__,__,T1,__,__,__,__},
/*string ST*/ {ST,__,ST,ST,ST,ST,ST,ST,-4,ES,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST},
/*escape ES*/ {__,__,__,__,__,__,__,__,ST,ST,ST,__,__,__,__,__,__,ST,__,__,__,ST,__,ST,ST,__,ST,U1,__,__,__},
/*u1     U1*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U2,U2,U2,U2,U2,U2,U2,U2,__,__,__,__,__,__,U2,U2,__},
/*u2     U2*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U3,U3,U3,U3,U3,U3,U3,U3,__,__,__,__,__,__,U3,U3,__},
/*u3     U3*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U4,U4,U4,U4,U4,U4,U4,U4,__,__,__,__,__,__,U4,U4,__},
/*u4     U4*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,ST,ST,ST,ST,ST,ST,ST,ST,__,__,__,__,__,__,ST,ST,__},
/*minus  MI*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,ZE,IN,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*zero   ZE*/ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,FR,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*int    IN*/ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,FR,IN,IN,__,__,__,__,E1,__,__,__,__,__,__,__,__,E1,__},
/*frac   FR*/ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,__,FR,FR,__,__,__,__,E1,__,__,__,__,__,__,__,__,E1,__},
/*e      E1*/ {__,__,__,__,__,__,__,__,__,__,__,E2,E2,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*ex     E2*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*exp    E3*/ {OK,OK,__,-8,__,-7,__,-3,__,__,__,__,__,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*tr     T1*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T2,__,__,__,__,__,__},
/*tru    T2*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T3,__,__,__},
/*true   T3*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,OK,__,__,__,__,__,__,__,__,__,__},
/*fa     F1*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F2,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/*fal    F2*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F3,__,__,__,__,__,__,__,__},
/*fals   F3*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F4,__,__,__,__,__},
/*false  F4*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,OK,__,__,__,__,__,__,__,__,__,__},
/*nu     N1*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N2,__,__,__},
/*nul    N2*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N3,__,__,__,__,__,__,__,__},
/*null   N3*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,OK,__,__,__,__,__,__,__,__},
};


/*
    These modes can be pushed on the stack.
*/
enum modes {
    MODE_ARRAY, 
    MODE_DONE,  
    MODE_KEY,   
    MODE_OBJECT,
};

json_checker::json_checker():
    state(GO),maxtop(0),top(-1),stack(),reject(false),comma_count(0)
{
    /* constructor. Takes stacksize parameter that restricts the level
     * of maximum nesting.
     *
     * To continue the process, call JSON_checker_char for each
     * character in the JSON text, and then call JSON_checker_done to
     * obtain the final result.  These functions are fully reentrant.
     * 
     */
    push(MODE_DONE);			// we know we are done when this is the top of stack
}

bool json_checker::check_if_done()
{
    return state==OK && top==0;
}

int json_checker::done()
{
/*
    The JSON_checker_done function should be called after all of the characters
    have been processed, but only if every call to JSON_checker_char returned
    true. This function deletes the JSON_checker and returns true if the JSON
    text was accepted.
*/
    int result = state == OK && pop( MODE_DONE);
    reject = true;
    return result;
}


int json_checker::push(uint8_t mode)
{
    /*
     * Push a mode onto the stack. Return false if there is overflow.
     */
    top += 1;
    if (top >= stacksize) {
        return false;
    }
    stack[top] = mode;
    if(top>maxtop) maxtop=top;
    return true;
}


int json_checker::pop(uint8_t mode)
{
/*
    Pop the stack, assuring that the current mode matches the expectation.
    Return false if there is underflow or if the modes mismatch.
*/
    if (top < 0 || stack[top] != mode) {
        return false;
    }
    top -= 1;
    return true;
}



/*
 * Check the next character. Return 0 if okay, -1 if not.
 *
 * Call this function for each character (or
 * partial character) in your JSON text. It can accept UTF-8, UTF-16, or
 * UTF-32. It returns 0 if things are looking ok so far, -1 if error.
 *
 */
int json_checker::check_char(int next_char)
{
    int next_class, next_state;
/*
    Determine the character's class.
*/
    if(reject) return -1;		// in rejecting mode

    if (next_char < 0) {
	reject = true;
	return -1;
    }
    if (next_char >= 128) {
        next_class = C_ETC;
    } else {
        next_class = ascii_class[next_char];
        if (next_class <= __) {
	    reject = true;
	    return -1;
        }
    }
/*
    Get the next state from the state transition table.
*/
    next_state = state_transition_table[state][next_class];
    if (next_state >= 0) {
/*
    Change the state.
*/
        state = next_state;
    } else {
/*
    Or perform one of the actions.
*/
        switch (next_state) {
/* empty } */
        case -9:
            if (!pop( MODE_KEY)) {
		reject = true;
		return -1;
            }
            state = OK;
            break;

/* } */ case -8:
            if (!pop( MODE_OBJECT)) {
		reject = true;
		return -1;
            }
            state = OK;
            break;

/* ] */ case -7:
            if (!pop( MODE_ARRAY)) {
		reject = true;
		return -1;
            }
            state = OK;
            break;

/* { */ case -6:
            if (!push( MODE_KEY)) {
		reject = true;
		return -1;
            }
            state = OB;
            break;

/* [ */ case -5:
            if (!push( MODE_ARRAY)) {
		reject = true;
		return -1;
            }
            state = AR;
            break;

/* " */ case -4:
            switch (stack[top]) {
            case MODE_KEY:
                state = CO;
                break;
            case MODE_ARRAY:
            case MODE_OBJECT:
                state = OK;
                break;
            default:
		reject = true;
		return -1;
            }
            break;

/* , */ case -3:
	    comma_count++;
            switch (stack[top]) {
            case MODE_OBJECT:
/*
 * A comma causes a flip from object mode to key mode.
 */
                if (!pop( MODE_OBJECT) || !push( MODE_KEY)) {
		    reject = true;
		    return -1;
                }
                state = KE;
                break;
            case MODE_ARRAY:
                state = VA;
                break;
            default:
		reject = true;
		return -1;
            }
            break;

/* : */ case -2:
/*
 *   A colon causes a flip from key mode to object mode.
*/
            if (!pop( MODE_KEY) || !push( MODE_OBJECT)) {
		reject = true;
		return -1;
            }
            state = VA;
            break;
/*
    Bad action.
*/
        default:
	    reject = true;
	    return -1;
        }
    }
    return 0;				// not bad data
}


/****************************************************************
 ** Make the JSON validator work with bulk_extractor
 */

static bool is_json_second_char[256];		// shared between all threads

static const char *json_second_chars = "0123456789.-{[ \t\n\r\"";
extern "C"
void scan_json(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "json";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Scans for JSON-encoded data";
        sp.info->scanner_version= "1.1";
        sp.info->feature_names.insert("json");

	/* Create a fast map of the valid json characters.*/
	memset(is_json_second_char,0,sizeof(is_json_second_char));
	for(int i=0;json_second_chars[i];i++){
	    is_json_second_char[(uint8_t)json_second_chars[i]] = true;
	}
	return; 
    }
    const sbuf_t &sbuf = sp.sbuf;
    feature_recorder *fr = sp.fs.get_name("json");

    if(sp.phase==scanner_params::PHASE_INIT){
        fr->set_flag(feature_recorder::FLAG_XML);
        return;
    }

    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN){

	for(size_t pos = 0;pos+1<sbuf.pagesize;pos++){
	    /* Find the beginning of a json object. This will improve later... */
	    if((sbuf[pos]=='{' || sbuf[pos]=='[') && is_json_second_char[sbuf[pos+1]]){
		json_checker jc;
		for(size_t i=pos;i<sbuf.bufsize;i++){
		    if(jc.check_char(sbuf[i])){ // is character invalid?
			pos = i;		    // yes
			break;
		    }
		    if((sbuf[i]==']' || sbuf[i]=='}') && jc.check_if_done()){
			// Only write JSON objects with more than 2 commas
			if(jc.comma_count > 2 ){
			    sbuf_t json(sbuf,pos,i-pos+1);
                            std::string json_hash = (*fr->fs.hasher.func)(json.buf,json.bufsize);
			    fr->write(sbuf.pos0+i,json.asString(),json_hash);;
			}
			pos = i;		// skip to the end
			break;
		    }
		}
	    }
	}
    }
}

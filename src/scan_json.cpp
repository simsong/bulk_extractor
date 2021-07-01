#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <stdlib.h>
#include <stdint.h>

#ifdef HAVE_JSON_C
#include <json-c/json.h>
#endif

#define MIN_SIZE 16
#define IS_STRICT 1

static bool is_json_second_char[256]; /* shared between all threads */

static const char *json_second_chars = "0123456789.-{[ \t\n\r\"";
extern "C"
void scan_json(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "json";
        sp.info->author         = "Simson Garfinkel & Jan Gruber";
#ifndef HAVE_JSON_C
        sp.info->description    = "(disabled; json-c not installed)";
#else
        sp.info->description    = "Scans for JSON-encoded data";
        sp.info->scanner_version= "1.1";
        sp.info->feature_names.insert("json");

	/* Create a fast map of the valid json characters.*/
	memset(is_json_second_char,0,sizeof(is_json_second_char));
	for(int i=0;json_second_chars[i];i++){
	    is_json_second_char[(uint8_t)json_second_chars[i]] = true;
	}
#endif
	return;
    }
#ifdef HAVE_JSON_C
    const sbuf_t &sbuf = sp.sbuf;
    feature_recorder *fr = sp.fs.get_name("json");

    if(sp.phase==scanner_params::PHASE_INIT){
        fr->set_flag(feature_recorder::FLAG_XML);
        return;
    }

    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;

    if(sp.phase==scanner_params::PHASE_SCAN){

      json_object *jo = NULL;
      json_tokener* jt = NULL;
      char* js = NULL;
      size_t end = 0;
      enum json_tokener_error je;

      for(size_t pos = 0;pos+1<sbuf.pagesize;pos++){

	/* Find the beginning of a json object. This will improve later... */
	if(sbuf[pos]=='{' || sbuf[pos]=='['){

	  /* Setup parser */
	  jt = json_tokener_new();
	  if (IS_STRICT)
	    json_tokener_set_flags(jt, JSON_TOKENER_STRICT);

	  /* Try to parse string */
	  const char* c = (const char*) &sbuf.buf[pos]; /* Solution for 1.6 */
	  jo = json_tokener_parse_ex(jt, c, strlen(c));
	  je = json_tokener_get_error(jt);

	  /* String was a valid JSON object */
	  if (je == json_tokener_success && jo){
	    /* Change this to `json_tokener_get_parse_end(jt);` for libjson-c > 0.15 */
	    end = jt->char_offset;
	    js = (char*) json_object_to_json_string_ext(jo, JSON_C_TO_STRING_PLAIN);

	    /* Discard very short matches */
	    if (strlen(js) > MIN_SIZE){

	      /* Constructs output buffer */
	      sbuf_t jbuf(sbuf, pos, end+1);
	      std::string json_hash = (*fr->fs.hasher.func)(jbuf.buf, jbuf.bufsize);

	      /* Stores result */
	      fr->write(sbuf.pos0 + pos, jbuf.asString(), json_hash);
	      pos += end + 1;
	    }
	  }
	  /* Clean up */
	  json_tokener_free(jt);

	  if(jo){
	    free(jo);
	    jo = NULL;
	  }

	  if(js){
	    free(js);
	    js = NULL;
	  }

	}
      }

    }
#endif
}

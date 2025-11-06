%{

#include "config.h"
#include "sbuf_flex_scanner.h"
#include "scan_vin.h"

/*
 * Vehicle Identifier Scanner
 * Detects and validates VINs in data streams
 * Could be modified to handle Hull Identification Number, Aircraft, etc
 *
 * References for VIN:
 * http://en.wikipedia.org/wiki/Vehicle_identification_number
 * ISO 3779:2009 - Road vehicles - Vehicle identification number (VIN)
 * SAE J853 - Vehicle Identification Numbers
 * 49 CFR Part 565 - Vehicle Identification Number Requirements
 */

class vin_scanner : public sbuf_scanner {
public:
    vin_scanner(const scanner_params &sp):
        sbuf_scanner(*sp.sbuf),
        vin_recorder(sp.named_feature_recorder("vin")){
    }

    class feature_recorder &vin_recorder;
};

#define YY_EXTRA_TYPE vin_scanner *                /* holds our class pointer */
YY_EXTRA_TYPE yyvin_get_extra(yyscan_t yyscanner); /* redundant declaration */
inline class vin_scanner *get_extra(yyscan_t yyscanner) {return yyvin_get_extra(yyscanner);}

/**
 * Check if the text contains manufacturer keywords that suggest automotive context
 * This helps reduce false positives
 */
static bool has_automotive_context(const sbuf_t &sbuf, size_t pos, int range = 100)
{
    // Common automotive manufacturer names and keywords
    const char* keywords[] = {
        // American Manufacturers
        "ford", "chevrolet", "chevy", "gmc", "buick", "cadillac", "chrysler",
        "dodge", "ram", "jeep", "lincoln", "mercury", "pontiac", "oldsmobile",
        "saturn", "hummer", "plymouth", "eagle", "geo", "amg",
        
        // Japanese Manufacturers
        "toyota", "honda", "nissan", "mazda", "subaru", "mitsubishi", "suzuki",
        "isuzu", "daihatsu", "lexus", "infiniti", "acura", "scion",
        
        // Korean Manufacturers
        "hyundai", "kia", "genesis", "daewoo", "ssangyong",
        
        // German Manufacturers
        "bmw", "mercedes", "audi", "volkswagen", "vw", "porsche", "opel",
        "smart", "maybach", "mini",
        
        // European Manufacturers (Other)
        "volvo", "saab", "fiat", "alfa", "romeo", "lancia", "maserati",
        "ferrari", "lamborghini", "bugatti", "peugeot", "citroen", "renault",
        "seat", "skoda", "dacia", "jaguar", "land", "rover", "bentley",
        "rolls", "royce", "aston", "martin", "lotus", "mclaren", "mg",
        "vauxhall", "lada",
        
        // Chinese Manufacturers
        "geely", "byd", "chery", "great", "wall", "nio", "xpeng", "li",
        "auto", "mg", "polestar",
        
        // Indian Manufacturers
        "tata", "mahindra", "maruti",
        
        // Other International
        "tesla", "rivian", "lucid", "fisker", "delorean", "yugo",
        
        // Generic Terms
        "vehicle", "automobile", "automotive", "frame", "shell", "motor", "car",
        "truck", "suv", "vin", "chassis", "model", "make", 
        
        nullptr
    };

    
    // Check before and after the position
    size_t start = (pos > range) ? pos - range : 0;
    size_t end = std::min(pos + range, sbuf.bufsize);
    
    std::string context = sbuf.substr(start, end - start);
    
    // Convert to lowercase for case-insensitive search
    std::transform(context.begin(), context.end(), context.begin(), ::tolower);
    
    for (int i = 0; keywords[i] != nullptr; i++) {
        if (context.find(keywords[i]) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

#define SCANNER "scan_vehicle"
%}

%option noyywrap
%option 8bit
%option batch
%option case-insensitive
%option pointer
%option noyymore
%option prefix="yyvin_"

/* Valid VIN characters: 0-9, A-H, J-N, P, R-Z (excludes I, O, Q) */
VINCHAR    [0-9A-HJ-NPR-Z]

/* Common delimiters around VINs */
VINDELIM   ([ \t\r\n:,;|"'(){}[\]<>#.!?])
VINSTART   ({VINDELIM}|^)
VINEND     ({VINDELIM}|$)

/* VIN components. For reference only, not used as the rule expansion chokes on VIN17 */
WMI        ({VINCHAR}{3})              /* World Manufacturer Identifier */
VDS        ({VINCHAR}{6})              /* Vehicle Descriptor Section (includes check digit) */
VIS        ({VINCHAR}{8})              /* Vehicle Identifier Section */
VIN17      ({VINCHAR}{17})             /* Complete 17-character VIN */

/* Common prefixes that indicate VIN context */
VINPREFIX  ((VIN|vin|Vin|V\.I\.N\.)[ #:]*|vehicle[ ]*id(entification)?[ ]*number[ #:]*)

/* Automotive context patterns */
AUTOCONTEXT ((make|model|year|chassis|frame|serial)[ #:]*)

%%

{VINPREFIX}({VINCHAR}{17})/{VINEND} {
    /* VIN with explicit prefix like "VIN: " */
    vin_scanner &s = *yyvin_get_extra(yyscanner);
    s.check_margin();
    
    /* Find where the actual VIN starts - skip to last 17 chars which is the VIN */
    size_t match_len = yyleng;
    if (match_len >= 17) {
        const char *vin_start = yytext + (match_len - 17);
        
        if (valid_vin(vin_start, 17)) {
            s.vin_recorder.write_buf(SBUF, POS + (match_len - 17), 17);
        }
    }
    s.pos += yyleng;
}

{VINSTART}({VINCHAR}{17})/{VINEND} {
    /* Standalone VIN without prefix */
    vin_scanner &s = *yyvin_get_extra(yyscanner);
    s.check_margin();
    
    /* Skip the delimiter at the start */
    size_t vin_offset = 1;
    if (POS == 0) vin_offset = 0;  /* At beginning of buffer */
    
    if (valid_vin(yytext + vin_offset, 17)) {
        /* Additional context check for standalone VINs to reduce false positives */
        bool write_it = true;
        
        /* If no automotive context nearby, be more strict */
        if (!has_automotive_context(SBUF, POS + vin_offset, 200)) {
            /* Check if surrounded by hex-like content */
            size_t vin_start = POS + vin_offset;
            if (vin_start > 4 && (vin_start + 17 + 4) < SBUF.bufsize) {
                bool hex_before = true;
                bool hex_after = true;
                
                for (int i = 0; i < 4; i++) {
                    if (!isxdigit(SBUF[vin_start - 4 + i])) hex_before = false;
                    if (!isxdigit(SBUF[vin_start + 17 + i])) hex_after = false;
                }
                
                if (hex_before && hex_after) write_it = false;
            }
        }
        
        if (write_it) {
            s.vin_recorder.write_buf(SBUF, POS + vin_offset, 17);
        }
    }
    s.pos += yyleng;
}

{AUTOCONTEXT}({VINCHAR}{17})/{VINEND} {
    /* VIN with automotive context prefix */
    vin_scanner &s = *yyvin_get_extra(yyscanner);
    s.check_margin();
    
    /* Find where the actual VIN starts - skip to last 17 chars which is the VIN */
    size_t match_len = yyleng;
    if (match_len >= 17) {
        const char *vin_start = yytext + (match_len - 17);
        
        if (valid_vin(vin_start, 17)) {
            s.vin_recorder.write_buf(SBUF, POS + (match_len - 17), 17);
        }
    }
    s.pos += yyleng;
}

.|\n {
    /**
     * The no-match rule.
     * If we are beyond the end of the margin, call it quits.
     */
    vin_scanner &s = *yyvin_get_extra(yyscanner);
    s.check_margin();
    /* putchar(yytext[0]); */ /* Uncomment for debugging */
    s.pos++;
}

%%

extern "C"
void scan_vehicle(struct scanner_params &sp)
{
    sp.check_version();
    if (sp.phase == scanner_params::PHASE_INIT) {
        
        sp.info->set_name("vin");
        sp.info->author         = "Kam A. Woods";
        sp.info->description    = "Scans for Vehicle Identifiers (Currently: VINs)";
        sp.info->scanner_version = "1.0";
        
        // Define the feature recorders we'll use
        sp.info->feature_defs.push_back(feature_recorder_def("vin"));
        
        // Define histograms for analysis
        // VINs are already uppercase, so we use default flags (no transformations)
        histogram_def::flags_t nf;
        
        // Histogram of VINs
        sp.info->histogram_defs.push_back(
            histogram_def("vin", "vin", "", "", "histogram", nf)
        );
        
        // Debug configuration
        int vin_debug = 0;
        sp.get_scanner_config("vin_debug", &vin_debug, "Enable VIN scanner debugging");
        scan_vin_debug = vin_debug;
        
        return;
    }
    
    if (sp.phase == scanner_params::PHASE_SCAN) {
        vin_scanner lexer(sp);
        yyscan_t scanner;
        yyvin_lex_init(&scanner);
        yyvin_set_extra(&lexer, scanner);
        
        try {
            yyvin_lex(scanner);
        }
        catch (sbuf_scanner::sbuf_scanner_exception &e) {
            std::cerr << "Scanner " << SCANNER << " Exception " << e.what() 
                      << " processing " << sp.sbuf->pos0 << "\n";
        }
        
        yyvin_lex_destroy(scanner);
    }
    
    // Suppress unused warnings for flex-generated functions

    (void)yyunput;
    (void)yy_fatal_error;
}

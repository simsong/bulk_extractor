/**
 * scan_ccns2:
 * additional filters for scanning credit card numbers.
 * used by the scan_accts.flex system.
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "scan_ccns2.h"
#include "utils.h"
#include "dfxml/src/hash_t.h"

int scan_ccns2_debug=0;


/* credit2.cpp:
 * A filter to scan stdin to stdout, pass through only the lines
 * that have valid credit-card numbers by our feature detector.
 */

inline int digit_val(char cc)
{
    return cc - '0';
}


/** extract the digits from a buffer of a given length
 * into a null-termianted array (which must be at least len+1).
 * Return 0 if extract is successful and if the count of non-digit
 * numbers is either 0, 3 (for credit card numbers beginning with a 4 or 5)
 * or 2 (for credit card numbers beginning with a 3).
 */
static int extract_digits_and_test(const char *buf,int len,char *digits)
{
    int nondigit_count = 0;
    while(*buf && len){
	if(isdigit(*buf)) *digits++ = *buf;
	else nondigit_count++;
	buf++;
	len--;
    }
    *digits = 0;			// null-terminate

    if(nondigit_count==0) return 0;
    if((digits[0]=='4' || digits[0]=='5') && nondigit_count==3){
	return 0;			// visa or mastercard
    }
    if((digits[0]=='3') && (nondigit_count==2)){
	return 0;			// american express
    }
    return -1;
}

/* Return true if the string only has hex digits */
static int only_hex_digits(const char *buf,int len)
{
    while(*buf && len){
	if(ishexnumber(*buf)==0) return 0;
	buf++;
	len--;
    }
    return 1;
}

static int only_dec_digits(const char *buf,int len)
{
    while(*buf && len){
	if(isdigit(*buf)==0) return 0;
	buf++;
	len--;
    }
    return 1;
}


/****************************************************************
 *** The tests. Note that sense is reversed.
 ****************************************************************/



/* int ccv1(const char *str,int len)
 * Return 0 if a number follows the
 * Credit Card Number Validation Algorithm Version #1, -1 if it fails
 * (Version 2 is a pure database lookup based on the 3 digits on the back panel.)
 */

static int ccv1_test(const char *digits)
{
    int chk=0;
    int double_flag=0;			// is number doubled?
    int len = strlen(digits);
    int i;
    int doubled[] = { 0,2,4,6,8,1,3,5,7,9 };	/* what are number when "doubled" */
   
    for(i=len-1;i>=0;i--){
	int val = digit_val(digits[i]);
	if(double_flag==0){
	    chk += val;
	    double_flag = 1;
	} else {
	    chk += doubled[val];
	    double_flag = 0;
	}
    }

    if ( (chk%10) == 0 ) {
	return 0;			// passed alg
    }
    return -1;
}

/* histogram_test:
 * Compute the historgram of the number.
 * If one digit is repeated more than 7 times, it is not valid.
 * If two sets of digits are repeated more than 5 times, it is not valid.
 */
static int histogram_test(const char *digits)
{
    int cntscore = 0;
    int digit_counts[10];			// count of each character 

    memset((void*)digit_counts,0,sizeof(digit_counts));
    while(*digits){
	digit_counts[digit_val(*digits)]++;
	digits++;
    }

    /* If we have more than 7 of one digit, 
     * or two digits with more than 5,
     * this isn't a valid number.
     */
    for(int i=0; i<10; i++) {
	if (digit_counts[i]>7) { return -1;}
	if (digit_counts[i]>4) { cntscore ++;}
    }
    if(cntscore >=2) return -1;
    return 0;				// passed histogram test
}


/*
 * Called to display strings. The first character is not part of the number.
 */

/** Return the value of the first 4 digites of a buffer, as an integer */
static int int4(const char *cc)
{
    char buf[5];
    for(int i=0;i<4 && cc[i];i++){
	buf[i] = cc[i];
    }
    buf[4] = 0;
    return atoi(buf);
}

/** Return the value of the first 6 digites of a buffer, as an integer */
static int int6(const char *cc)
{
    char buf[7];
    for(int i=0;i<6 && cc[i];i++){
	buf[i] = cc[i];
    }
    buf[6] = 0;
    return atoi(buf);
}

static int pattern_test(const char *digits)
{
    int a = int4(digits);
    int b = int4(digits+4);
    int c = int4(digits+8);
    int d = int4(digits+12);
    
    if(b-a == c-d) return -1;		/* something fishy going on... */
    return 0;
}

/**
 * return 0 if prefix is okay, -1 if it is not.
 *
 * revised prefix test based on Wikipedia bank card number table
 * http://en.wikipedia.org/wiki/Bank_card_number
 */

static int prefix_test(const char *digits)
{
    int len = strlen(digits);
    int a = int4(digits);
    int b = int6(digits);

    switch(len){
    case 13:
        if(digits[0]=='4') return 0; // Legacy as all 13-digits are deprecated
	return -1;
    case 14:
	if(a>=3000 && a<=3050) return 0; // Diners Club Carte Blanche (DC-CB)
	if(a>=3600 && a<=3999) return 0; // Diners Club International (DC-Int)
	return -1;
    case 15:
	if(a==2014) return 0; // Diners Club enRoute (DC-eR)
	if(a==2149) return 0; // Diners Club enRoute (DC-eR)
	if(a>=3400 && a<=3499) return 0; // American Express (AmEx)
	if(a>=3700 && a<=3799) return 0; // American Express (AmEx)
	return -1;
    case 16:
	if(a>=3528 && a<=3589) return 0; // JCB (JCB)
	if(a>=4000 && a<=4999) return 0; // Visa (Visa)
	if(b==417500) return 0; // Visa (Visa)
	if(a>=5100 && a<=5999) return 0; // MasterCard (MC)
	if(b>=560221 && b<=560225) return 0; // BankCard (BC)
	if(a==5610) return 0; // BankCard (BC)
	if(a==6011) return 0; // Discovery (Disc)
	if(b>=622126 && b<=622925) return 0; // China UnionPay (CUP)
	if(b>=624000 && b<=626999) return 0; // China UnionPay (CUP)
	if(b>=628200 && b<=628899) return 0; // China UnionPay (CUP)
	if(a==6304) return 0; // Laser (Lasr)
	if(a==6334) return 0; // Solo (Solo)
	if(a==6706) return 0; // Laser (Lasr)
	if(a==6709) return 0; // Laser (Lasr)
	if(a==6767) return 0; // Solo (Solo)
	if(a==6771) return 0; // Laser (Lasr)
	if(a>=6440 && a<=6499) return 0; // Discovery (Disc)
	if(a>=6500 && a<=6599) return 0; // Discovery (Disc)
	return -1;
    case 17:
	if(b>=622126 && b<=622925) return 0; // China UnionPay (CUP)
	if(b>=624000 && b<=626999) return 0; // China UnionPay (CUP)
	if(b>=628200 && b<=628899) return 0; // China UnionPay (CUP)
	if(a==6304) return 0; // Laser (Lasr)
	if(a==6706) return 0; // Laser (Lasr)
	if(a==6709) return 0; // Laser (Lasr)
	if(a==6771) return 0; // Laser (Lasr)
      	return -1;
    case 18:
	if(b>=622126 && b<=622925) return 0; // China UnionPay (CUP)
	if(b>=624000 && b<=626999) return 0; // China UnionPay (CUP)
	if(b>=628200 && b<=628899) return 0; // China UnionPay (CUP)
	if(a==6304) return 0; // Laser (Lasr)
	if(a==6334) return 0; // Solo (Solo)
	if(a==6706) return 0; // Laser (Lasr)
	if(a==6709) return 0; // Laser (Lasr)
	if(a==6767) return 0; // Solo (Solo)
	if(a==6771) return 0; // Laser (Lasr)
      	return -1;
    case 19:
	if(b>=622126 && b<=622925) return 0; // China UnionPay (CUP)
	if(b>=624000 && b<=626999) return 0; // China UnionPay (CUP)
	if(b>=628200 && b<=628899) return 0; // China UnionPay (CUP)
	if(a==6304) return 0; // Laser (Lasr)
	if(a==6334) return 0; // Solo (Solo)
	if(a==6706) return 0; // Laser (Lasr)
	if(a==6709) return 0; // Laser (Lasr)
	if(a==6767) return 0; // Solo (Solo)
	if(a==6771) return 0; // Laser (Lasr)
      	return -1;
    }
    return -1;
}

#define RETURN(code,reason) {if(scan_ccns2_debug & DEBUG_INFO) std::cerr << reason << "\n";return code;}
/**
 * Determine if this is or is not a credit card number.
 * Return 1 if it is, 0 if it is not.
 * buf[-WINDOW_MARGIN] must be accessible.
 * buf[len+WINDOW_MARGIN] must be accessible
 */
bool valid_ccn(const char *buf,int buflen)
{
    /* Make the digits array */
    if(buflen>19) RETURN(0,"Too long");

    char digits[20];			// just the digits

    memset(digits,0,sizeof(digits));
    if(extract_digits_and_test(buf,buflen,digits)) RETURN(0,"failed nondigit count");
    if(prefix_test(digits))    RETURN(0,"failed prefix test");
    if(ccv1_test(digits))      RETURN(0,"failed ccv1 test");
    if(pattern_test(digits))   RETURN(0,"failed pattern test");
    if(histogram_test(digits)) RETURN(0,"failed histogram test");

    int before_window = 4;		// what we care about before
    int after_window = 4;		// what we care about before

    /* If the 4 characters before or after are hex digits but not decimal digits,
     * then this is probably not a credit card number.
     * We're probably instead in a sea of hex. So abort.
     */
    if(only_hex_digits(buf-before_window,before_window) && !only_dec_digits(buf-before_window,before_window)){
	RETURN(0,"failed before hex test");
    }
    if(only_hex_digits(buf+buflen,after_window) && !only_dec_digits(buf+buflen,after_window)){
	RETURN(0,"failed after hex test");
    }

    return 1;
}


/**
 * Throw out phone numbers that are preceeded or followed with only
 * numbers and spaces or brackets. These are commonly seen in PDF files
 * when they are decompressed.
 */
inline bool valid_char(char ch)
{
    return isdigit(ch) || isspace(ch) || ch=='[' || ch==']' || ch=='<' || ch=='Z' || ch=='.' || ch=='l' || ch=='j';
}

bool  valid_phone(const sbuf_t &sbuf,size_t pos,size_t len)
{
    /* We want invalid characters before and after (assuming there is a before and after */
    int invalid_before = 0;
    int invalid_after = 0;
    if(pos>8){
	for(size_t i=pos-8;i<pos;i++){
	    if(!valid_char(sbuf[i])) invalid_before = 1;
	}
    } else {
	invalid_before = 1;
    }

    if(sbuf.bufsize < pos+len+8){
	for(size_t i=pos+len;i<pos+len+8;i++){
	    if(!valid_char(sbuf[i])) invalid_after = 1;
	}
    } else {
	invalid_after = 1;
    }

    /*
     * 2013-05-28: if followed by ' #{1,5} ' then it's not a phone either!
     */
    if(pos+len+5 < sbuf.bufsize){
        if(sbuf[pos+len]==' ' && isdigit(sbuf[pos+len+1])){
            for(size_t i = pos+len+1 ; (i+1<sbuf.bufsize) && (i<pos+len+8);i++){
                if(isdigit(sbuf[i]) && sbuf[i+1]==' ') return false; // not valid
            }
        }
    }

    /* If it is followed by a dash and a number, it's not a phone number */
    if(pos+len+2 < sbuf.bufsize){
        if(sbuf[pos+len]=='-' && isdigit(sbuf[pos+len+1])) return false;
    }

    return invalid_before!=0 && invalid_after!=0;
}

// http://rosettacode.org/wiki/Bitcoin/address_validation#C
static const char *base58_chars =
    "123456789"
    "ABCDEFGHJKLMNPQRSTUVWXYZ"
    "abcdefghijkmnopqrstuvwxyz";
static int base58_vals[256];
void build_unbase58()
{
    memset(base58_vals,-1,sizeof(base58_vals));
    for(size_t i=0;base58_chars[i];i++){
        base58_vals[(u_char)(base58_chars[i])] = i;
    }
}

bool unbase58(const char *s,uint8_t *out,size_t len)
{
    memset(out,0,25);
    for(size_t i=0;s[i] && i<len;i++){
        int c = base58_vals[(u_char)(s[i])];
        if (c==-1) return false; // invalid character
        for (int j = 25; j--; ) {
            c += 58 * out[j];
            out[j] = c % 256;
            c /= 256;
        }
        if (c!=0) return false; // address too long
    }
    return true;
}

// A bitcoin address uses a base58 encoding, which uses an alphabet of the characters 0 .. 9, A ..Z, a .. z, 
// but without the four characters 0, O, I and l.
bool valid_bitcoin_address(const char *s,size_t len){
    uint8_t dec[32];
    if (unbase58(s,dec,len)==false) return false;
    sha256_t d1 = sha256_generator::hash_buf(dec,21);
    sha256_t d2 = sha256_generator::hash_buf(d1.digest,d1.size());
    if (memcmp(dec+21, d2.digest, 4)!=0){
        return false;
    }
    return true;  /* validates */
};






#ifdef DEBUG
static int validate_ccn_debug(const char *buf,int buflen)
{
    char digits[64];

    printf("running tests. 0 means passed, -1 means failed.\n\n");
    printf("nondigit_test(%s) = %d\n",buf,extract_digits_and_test(buf,buflen,digits));
    printf("prefix_test(%s) = %d \n",digits,prefix_test(digits));
    printf("ccv1_test(%s) = %d \n",digits,ccv1_test(digits));
    printf("histogram_test(%s) = %d \n",digits,histogram_test(digits));
    printf("pattern_test(%s) = %d \n",digits,pattern_test(digits));
    printf("only_hex_digits(%s) = %d\n",buf,only_hex_digits(buf,strlen(buf)));
    printf("only_dec_digits(%s) = %d\n",buf,only_dec_digits(buf,strlen(buf)));
    return validate_ccn(buf,buflen);
}
#endif

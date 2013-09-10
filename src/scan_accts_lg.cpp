#include <string>

namespace accts {
  //
  // subpatterns
  //

  const std::string END("([^0-9e.]|(\\.[^0-9]))");
  const std::string BLOCK("[0-9]{4}");
  const std::string DELIM("[- ]");
  const std::string DB("(" + BLOCK + DELIM + ")");
  const std::string SDB("([45][0-9]{3}" + DELIM + ")");
  const std::string TDEL("[ /.-]");

  const std::string PHONETEXT("([^a-z](tel[.ephon]*|fax|facsimile|DSN|telex|TTD|mobile|cell)):?");

  const std::string YEAR("(19[0-9][0-9]|20[01][0-9])");
  const std::string MONTH("(Jan(uary)?|Feb(ruary)?|Mar(ch)?|Apr(il)?|May|Jun(e)?|Jul(y)?|Aug(ust)?|Sep(tember)?|Oct(ober)?|Nov(ember)?|Dec(ember)?|0?[1-9]|1[0-2])");
  const std::string DAY("([0-2]?[0-9]|3[01])");

  const std::string SYEAR("([0-9][0-9])");
  const std::string SMONTH("([01][0-2])");
 
  const std::string DATEA("(" + YEAR + "-" + MONTH + "-" + DAY + ")");
  const std::string DATEB("(" + YEAR + "/" + MONTH + "/" + DAY + ")");
  const std::string DATEC("(" + DAY + " " + MONTH + " " + YEAR + ")");
  const std::string DATED("(" + MONTH + " " + DAY + "[, ]+" + YEAR + ")");

  const std::string DATEFORMAT("(" + DATEA + "|" + DATEB + "|" + DATEC + "|" + DATED + ")");

  //
  // patterns
  //

  // FIXME: kill this one?
  /* #### #### #### #### #### #### is definately not a CCN. */
  const std::string REGEX1("[^0-9a-z]" + DB + "{5}");

  // FIXME: leading context
  // FIXME: trailing context
  /* #### #### #### #### --- most credit card numbers*/
  const std::string REGEX2("[^0-9a-z]" + SDB + DB + DB + BLOCK + END);

  // FIXME: leading context
  // FIXME: trailing context
  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const std::string REGEX3("[^0-9a-z.]3[0-9]{3}" + DELIM + "[0-9]{6}" + DELIM + "[0-9]{5}" + END);

  // FIXME: leading context
  // FIXME: trailing context
  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const std::string REGEX4("[^0-9a-z.]3[0-9]{14}" + END);

  // FIXME: leading context
  // FIXME: trailing context
  /* ###############  13-19 numbers as a block beginning with a 4 or 5
   * followed by something that is not a digit.
   * Yes, CCNs can now be up to 19 digits long. 
   * http://www.creditcards.com/credit-card-news/credit-card-appearance-1268.php
   */  
  const std::string REGEX5("[^0-9a-z.][4-6][0-9]{15,18}" + END);

  // FIXME: leading context
  /* ;###############=YYMM101#+? --- track2 credit card data */
  /* {SYEAR}{SMONTH} */
  /* ;CCN=05061010000000000738? */
  const std::string REGEX6("[^0-9a-z][4-6][0-9]{15,18}=" + SYEAR + SMONTH + "101[0-9]{13}");

  // FIXME: trailing context
  // FIXME: leading context
  /* US phone numbers without area code in parens */
  /* New addition: If proceeded by " ####? ####? " 
   * then do not consider this a phone number. We see a lot of that stuff in
   * PDF files.
   */
  const std::string REGEX7("[^0-9a-z]([0-9]{3}" + TDEL + "){2}[0-9]{4}" + END);

  // FIXME: trailing context
  // FIXME: leading context
  /* US phone number with parens, like (215) 555-1212 */
  const std::string REGEX8("[^0-9a-z]\\([0-9]{3}\\)" + TDEL + "?[0-9]{3}" + TDEL + "[0-9]{4}" + END);

  // FIXME: trailing context
  // FIXME: leading context
  /* Generalized international phone numbers */
  const std::string REGEX9("[^0-9a-z]\\+[0-9]{1,3}(" + TDEL + "[0-9]{2,3}){2,6}[0-9]{2,4}" + END);

  /* Generalized number with prefix */
  const std::string REGEX10(PHONETEXT + "[0-9/ .+]{7,18}");

  /* Generalized number with city code and prefix */
  const std::string REGEX11(PHONETEXT + "[0-9 +]+ ?\\([0-9]{2,4}\\) ?[\\-0-9]{4,8}");

  // FIXME: trailing context
  /* Generalized international phone numbers */
  const std::string REGEX12("fedex[^a-z]+([0-9]{4}[- ]?){2}[0-9]" + END);

  // FIXME: trailing context
  const std::string REGEX13("ssn:?[ \\t]+[0-9]{3}-?[0-9]{2}-?[0-9]{4}" + END);

  const std::string REGEX14("dob:?[ \\t]+" + DATEFORMAT);

  // FIXME: trailing context
  /* Possible BitLocker Recovery Key. */
  const std::string BITLOCKER("[^\\z30-\\z39]([0-9]{6}-){7}[0-9]{6}[^\\z30-\\z39]");

  /*
   * Common box arrays found in PDF files
   * With more testing this can and will still be tweaked
   */
  const std::string PDF_BOX("box ?\\[[0-9 -]{0,40}\\]");

  /*
   * Common rectangles found in PDF files
   *  With more testing this can and will still be tweaked
   */
  const std::string PDF_RECT("\\[ ?([0-9.-]{1,12} ){3}[0-9.-]{1,12} ?\\]");
}

import java.nio.charset.Charset;
import java.io.ByteArrayOutputStream;
/**
 * The <code>EscapedStrings</code> class provides conversions between escaped and unescaped
 * String formats and between byte and String formats of various UTF encodings.
 * See UTF_* constants for available UTF encodings supported.
 */
public class UTF8Tools {
  public static final Charset UTF_8 = Charset.forName("UTF-8");
  public static final Charset UTF_16 = Charset.forName("UTF-16");
  public static final Charset UTF_16LE = Charset.forName("UTF-16LE");
  public static final Charset UTF_16BE = Charset.forName("UTF-16BE");
  private static final byte[] ESC_NULL = {'\\', 'x', '0', '0'};
  private static final byte[] ESC_BACKSLASH = {'\\', 'x', '5', 'C'};

  /**
   * Remove any escape codes from the byte array.
   */
  public static byte[] unescapeBytes(byte[] bytes) {
    ByteArrayOutputStream s = new ByteArrayOutputStream();
    int i = 0;
    while (i < bytes.length) {
      // manage "\"
      if (bytes[i] == '\\' && i+3 < bytes.length) {
        if (bytes[i+1] == 'x'
         && ((bytes[i+2] >= '0' && bytes[i+2] <= '9')
          || (bytes[i+2] >= 'a' && bytes[i+2] <= 'f')
          || (bytes[i+2] >= 'A' && bytes[i+2] <= 'F'))
         && ((bytes[i+3] >= '0' && bytes[i+3] <= '9')
          || (bytes[i+3] >= 'a' && bytes[i+3] <= 'f')
          || (bytes[i+3] >= 'A' && bytes[i+3] <= 'F'))) {

          // escaped hex
          final int upper;
          final int lower;
          if (bytes[i+2] <= '9') {
            upper = (bytes[i+2] - '0') * 16;
          } else if (bytes[i+2] <= 'F') {
            upper = (bytes[i+2] - 'A' + 10) * 16;
          } else {
            upper = (bytes[i+2] - 'a' + 10) * 16;
          }
          if (bytes[i+3] <= '9') {
            lower = (bytes[i+3] - '0') * 16;
          } else if (bytes[i+3] <= 'F') {
            lower = (bytes[i+3] - 'A' + 10) * 16;
          } else {
            lower = (bytes[i+3] - 'a' + 10) * 16;
          }
          s.write(upper*16 + lower*1);
          i+= 4;

        } else if ((bytes[i+1] >= '0' && bytes[i+1] <= '3')
                && (bytes[i+2] >= '0' && bytes[i+2] <= '7')
                && (bytes[i+3] >= '0' && bytes[i+3] <= '7')) {
          // escaped octal
          s.write((bytes[i+1] - '0') * 64
                + (bytes[i+2] - '0') * 8
                + (bytes[i+3] - '0')* 1);
          i+= 4;
        } else {
          // the escape code is invalid, so put it in directly
          s.write(bytes[i]);
          i++;
        }
      } else {
        // the character is not escaped, so put it in directly
        s.write(bytes[i]);
        i++;
      }
    }

    return s.toByteArray();
  }

  /**
   * Put the escape character back but leave all other escape characters.
   * Specifically, put back \x5C and \134 to \.
   */
  public static byte[] unescapeEscape(byte[] bytes) {
    ByteArrayOutputStream s = new ByteArrayOutputStream();
    int i = 0;
    while (i < bytes.length) {
      // manage "\"
      if (bytes[i] == '\\' && i + 3 < bytes.length) {
        if ((bytes[i+1] == '1' && bytes[i+2] == '3' && bytes[i+3] == '4')
         || (bytes[i+1] == 'x' && bytes[i+1] == '5' && (bytes[i+2] == 'C' || bytes[i+2] == 'c'))) {
          // unescape the escape character
          s.write('\\');
          i+= 4;
        } else {
          // forward the code directly
          s.write(bytes[i]);
          i++;
        }
      } else {
        // the character is not escaped, so put it in directly
        s.write(bytes[i]);
        i++;
      }
    }
    return s.toByteArray();
  }

  /**
   * Create simple hacked utf8To16 filter by placing ESC_NULL between each byte.
   */
  public static byte[] utf8To16Basic(byte[] bytes) {
    if (bytes.length < 2) {
      return bytes;
    }
    ByteArrayOutputStream s = new ByteArrayOutputStream();
    for (int i=0; i<bytes.length-1; i++) {
      s.write(bytes[i]);
      s.write(ESC_NULL, 0, 4);
    }
    // write the last byte without the ESC_NULL after it
    s.write(bytes[bytes.length - 1]);
    return s.toByteArray();
  }

  /**
   * Create simple hacked utf16To8 filter by removing ESC_NULL escape codes.
   */
  public static byte[] utf16To8Basic(byte[] bytes) {
    ByteArrayOutputStream s = new ByteArrayOutputStream();
    for (int i=0; i<bytes.length; i++) {
      if (i + 4 <= bytes.length && bytes[i+0] == '\\'
       && ((bytes[i+1] == 'x' && bytes[i+2] == '0' && bytes[i+3] == '0')
        || (bytes[i+1] == '0' && bytes[i+2] == '0' && bytes[i+3] == '0'))) {
        // skip the ESC_NUL
        i+=3;
      } else {
        // take the byte
        s.write(bytes[i]);
      }
    }
    return s.toByteArray();
  }

  /**
   * Convert upper case bytes to lower case
   */
  public static byte[] toLower(byte[] bytes) {
    ByteArrayOutputStream s = new ByteArrayOutputStream();
    for (int i=0; i<bytes.length; i++) {
      byte b = bytes[i];
      if (b >= 0x41 && b <= 0x5a) {
        // change upper case to lower case
        s.write((byte)(b + 0x20));
      } else {
        s.write(b);
      }
    }
    return s.toByteArray();
  }

  /**
   * Check equality between two byte arrays
   */
  public static boolean bytesMatch(byte[] bytes1, byte[] bytes2) {
    if (bytes1 == null && bytes2 == null) {
      return false;
    }
    if (bytes1 == null || bytes2 == null) {
      return false;
    }
    if (bytes1.length != bytes2.length) {
      return false;
    }
    for (int i=0; i<bytes1.length; i++) {
      if (bytes1[i] != bytes2[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   * Determine if the bytes look like UTF16
   */
  public static boolean escapedLooksLikeUTF16(byte[] bytes) {
    byte[] unescapedBytes = UTF8Tools.unescapeBytes(bytes);
    int evenNulls = 0;
    int oddNulls = 0;
    if (unescapedBytes.length < 2) {
      return false;
    }
    for (int i=0; i<unescapedBytes.length-1; i+=2) {
      if (unescapedBytes[i] == 0) {
        evenNulls++;
      }
      if (unescapedBytes[i+1] == 0) {
        oddNulls++;
      }
    }
    return ((evenNulls > 1 && oddNulls == 0) || (evenNulls == 0 && oddNulls > 1));
  }

  /*
   * Remove nulls, only use this if escapedLooksLikeUTF16 is true.
   */
  public static byte[] stripNulls(byte[] bytes) {
    ByteArrayOutputStream s = new ByteArrayOutputStream();
    int i = 0;
    while (i < bytes.length) {
      // manage "\"
      if (bytes[i] == '\\' && i + 3 < bytes.length) {
        if ((bytes[i+1] == '0' && bytes[i+2] == '0' && bytes[i+3] == '0')
         || (bytes[i+1] == 'x' && bytes[i+2] == '0' && (bytes[i+3] == '0'))) {
          // remove the null
          i+= 4;
        } else {
          // forward the code directly
          s.write(bytes[i]);
          i++;
        }
      } else {
        // the character is not escaped, so put it in directly
        s.write(bytes[i]);
        i++;
      }
    }
    return s.toByteArray();
  }
}


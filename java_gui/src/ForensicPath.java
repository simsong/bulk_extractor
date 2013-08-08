import java.io.File;

/**
 * The <code>ForensicPath</code> class provides accessors to forensic path
 * strings.
 */
public final class ForensicPath {

  /**
   * Obtain adjusted path from existing path and specified offset.
   */
  public static String getAdjustedPath(String forensicPath, long offset) {
    int offsetIndex = getOffsetIndex(forensicPath);
    if (offsetIndex == 0) {
      // the forensic path is a simple offset
      return String.valueOf(offset);
    } else {
      // the forensic path has embedded ofsets
      String frontString = forensicPath.substring(0, offsetIndex);
      String adjustedPath = frontString + String.valueOf(offset);
      return adjustedPath;
    }
  }

  /**
   * Obtain path aligned on page boundary.
   */
  public static String getAlignedPath(String forensicPath) {

    // get existing offset
    long offset = getOffset(forensicPath);

    // back up to modulo PAGE_SIZE in front of the feature line's offset
    long alignedOffset = offset - (offset % ImageModel.PAGE_SIZE);
    return getAdjustedPath(forensicPath, alignedOffset);
  }

  /**
   * Obtain the offset from the last part of the forensic path.
   */
  public static long getOffset(String forensicPath) {
    // allow ""
    if (forensicPath == "") {
      return 0;
    }

    int offsetIndex = getOffsetIndex(forensicPath);
    String offsetString = forensicPath.substring(offsetIndex);
    long offset;
    try {
      offset = Long.parseLong(offsetString);
    } catch (Exception e) {
      WLog.log("malformed forensic path: '" + forensicPath + "'");
      offset = 0;
    }
    return offset;
  }

  // return start of path after file marker else 0
  // Path has a filename if it includes control codes.
  // Note: values were derived by parsing a feature file.
  // Note: binary values appeard to be "f4 80  80 9c".
  // should be U+10001C
  private static int getForensicPathIndex(String firstField) {
    final int length = firstField.length();
    if (length < 3) {
      return 0;
    }
    int i;
    for (i=0; i < length - 3; i++) {
      if (firstField.charAt(i) == 56256
       && firstField.charAt(i+1) == 56348
       && firstField.charAt(i+2) == '-') {
        return i+3;
      }
    }
    // not found
    return 0;
  }

  // the offset is after the last '-', if any, in the forensic path
  private static int getOffsetIndex(String forensicPath) {
    int delimeterPosition = forensicPath.lastIndexOf('-');
    if (delimeterPosition == -1) {
      // the forensic path is a simple offset
      return 0;
    } else {
      return delimeterPosition + 1;
    }
  }

    

  /**
   * See if firstField includes filename.
   */
  public static boolean hasFilename(String firstField) {
    return (getForensicPathIndex(firstField) > 0);
  }

  /**
   * Return filename from firstField if available else "".
   */
  public static String getFilename(String firstField) {
    int index = getForensicPathIndex(firstField);
    if (index == 0) {
      return "";
    } else {
      String filename = firstField.substring(0, index-2-1);
      return filename;
    }
  }

  /**
   * Return path from firstField without filename component,
   * returns same if there is no filename component
   */
  public static String getPathWithoutFilename(String firstField) {
    int index = getForensicPathIndex(firstField);
    if (index == 0) {
      return firstField;
    } else {
      String pathWithoutFilename = firstField.substring(index);
      return pathWithoutFilename;
    }
  }

  /**
   * Return printable path without any filename, in hex or decimal.
   */
  public static String getPrintablePath(String forensicPath, boolean useHex) {

    // derive the printable path
    if (!useHex) {
      return forensicPath;
    } else {
      // convert each numeric part to hex and print that
      StringBuffer sb = new StringBuffer();
      String[] parts = forensicPath.split("-");
      for (int i=0; i<parts.length; i++) {
        String part = parts[i];
        try {
          long offset = Long.parseLong(part);
          String hexPart = Long.toString(offset, 16); // radix
          sb.append(hexPart);
        } catch (Exception e) {
          // not a number so print as is
          sb.append(part);
        }
        if (i != parts.length -1) {
          sb.append('-');
        }
      }
      return sb.toString();
    }
  }

  /**
   * See if the first field is really a histogram entry.
   */
  public static boolean isHistogram(String firstField) {
    if (firstField.length() > 4
     && firstField.charAt(0) == 'n'
     && firstField.charAt(1) == '=') {
      int i;
      for (i=2; i<firstField.length(); i++) {
        if (firstField.charAt(i) < '0' || firstField.charAt(i) > '9') {
          break;
        }
      }
      if (firstField.charAt(i) == '\t') {
        return true;
      }
    }
    return false;
  }
}


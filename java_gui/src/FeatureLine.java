import java.util.Vector;
import java.io.File;
import java.io.FileInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;

//import java.awt.font.TextAttribute;
//import javax.swing.SwingUtilities;

/**
 * The <code>FeatureLine</code> class encapsulates the attributes of a feature line.
 */
public class FeatureLine {
  // input fields
  private final File imageFile;
  private final File featuresFile;
  private final long startByte;
  private final int numBytes;

  // derived fields
  private final FeatureLineType featureLineType;
  private final String firstField;
  private final byte[] featureField;
  private final byte[] contextField;
  private long address = -1;		// only address line types use this field
  private String path = "";		// only path line types use this field
  private int pathOffset = -1;		// only path line types use this field

  /**
   * Constructs a blank FeatureLine because FeatureListCellRenderer requires
   * a prototype display value in order to get the height right.
   */
  public FeatureLine() {
    imageFile = null;
    featuresFile = null;
    startByte = 0;
    numBytes = 0;
    featureLineType = null;
    firstField = null;
    featureField = null;
    contextField = null;
  }

  /**
   * Constructs a FeatureLine object for containing attributes realting to a feature line
   * @param imageFile the image file associated with the feature line
   * @param featuresFile the features file file associated with the feature line
   * @param startByte the start byte in the features file where the feature line text resides
   * @param numBytes the length of text in the features file defining this feature line
   */
  public FeatureLine(File imageFile, File featuresFile, long startByte, int numBytes) {

    // record input parameters
    this.imageFile = imageFile;
    this.featuresFile = featuresFile;
    this.startByte = startByte;
    this.numBytes = numBytes;

    // read the feature line bytes
    long size = 0;
    byte[] lineBytes = new byte[numBytes];
    try {
      // establish the file channel and file size for the feature file
      FileInputStream fileInputStream = new FileInputStream(featuresFile);
      FileChannel fileChannel = fileInputStream.getChannel();
      size = fileChannel.size();

      // get byte[] lineBytes from features file
      MappedByteBuffer mappedByteBuffer;
      mappedByteBuffer = fileChannel.map(MapMode.READ_ONLY, startByte, numBytes);
      mappedByteBuffer.load();
      mappedByteBuffer.get(lineBytes, 0, numBytes);
      fileChannel.close();	// JVM garbage collector does not close these, so force closure
      fileInputStream.close();	// JVM garbage collector does not close these, so force closure
    } catch (Exception e) {
      if (!featuresFile.exists()) {
        // the file doesn't exist
        WError.showError( "Features file " + featuresFile + " does not exist.", "BEViewer file error", null);
      } else {
        // something more went wrong so show the exception
        WError.showError( "Unable to open features file " + featuresFile + ".", "BEViewer file error", e);
      }

      // abort instantiation gracefully
      // use zero bytes
      featureLineType = FeatureLineType.INDETERMINATE_LINE;
      firstField = null;
      featureField = null;
      contextField = null;
      return;
    }

    // make sure the line bytes are in range of the feature file
    if (startByte + numBytes >= size) {
      throw new RuntimeException("Invalid line request");
    }

    // the line should end in \n and never in \r\n, but check for and strip out \r anyway.
    // It is possible for transport tools such as zip to turn \n into \r\n.
    // If deemed unnecessary, this check may be removed.
    // The \n is stripped out already.  Strip out the \r now if it is there.
    if (numBytes > 0 && lineBytes[numBytes - 1] == 0x0d) {
      numBytes--;
    }

    // identify the line type
    if (isFile(lineBytes)) {
      featureLineType = FeatureLineType.FILE_LINE;
    } else if (isAddress(lineBytes)) {
      featureLineType = FeatureLineType.ADDRESS_LINE;
    } else if (isPath(lineBytes)) {
      featureLineType = FeatureLineType.PATH_LINE;
    } else if (isHistogram(lineBytes)) {
      featureLineType = FeatureLineType.HISTOGRAM_LINE;
    } else {
      featureLineType = FeatureLineType.INDETERMINATE_LINE;
      // log this unrecognized line
      WLog.log("unrecognized feature line: '" + new String(lineBytes) + "'");
    }

    int i;
    int j;
    // define firstField as text before first tab else all text
    for (i=0; i < numBytes; i++) {
      if (lineBytes[i] == '\t') {
        // at first tab
        break;
      }
    }

    // if the first field is a File line then strip out the file mark
    if (featureLineType == FeatureLineType.FILE_LINE) {
      // first field with file mark stripped
      ByteArrayOutputStream s = new ByteArrayOutputStream();
      s.write(lineBytes, 0, i);
      firstField = new String(stripFileMark(s.toByteArray()));
    } else {
      // first field as is
      firstField = new String(lineBytes, 0, i);
    }

    // define featureField as text after first tab and before second tab else remaining text
    // Do not remove any escape codes.
    ByteArrayOutputStream featureStream = new ByteArrayOutputStream();
    if (i < numBytes - 1) {
      // there was a first tab
      for (j=i+1; j < numBytes; j++) {
        if (lineBytes[j] == '\t') {
          // at second tab
          break;
        }
      }
      featureStream.write(lineBytes, i + 1, j - (i + 1));

    } else {
      // there was no first tab, so set feature field as whole line
      j = numBytes - 1;
      WLog.log("Malformed line: " + new String(lineBytes));
      featureStream.write(lineBytes, 0, lineBytes.length);
    }
    featureField = featureStream.toByteArray();

    // define contextField as text after second tab, leaving in any escape codes
    ByteArrayOutputStream contextStream = new ByteArrayOutputStream();
    if (j < numBytes - 1) {
      // there was a second tab
      contextStream.write(lineBytes, j + 1, numBytes - (j + 1));
    } else {
      // there was no second tab so there is no text to add to the context field
    }
    contextField = contextStream.toByteArray();

    // set calculated fields
    address = -1;
    pathOffset = -1;
    path = "";
    if (featureLineType == FeatureLineType.ADDRESS_LINE) {
      // set the address for an address line
      address = Long.parseLong(firstField);

    } else if (featureLineType == FeatureLineType.PATH_LINE) {
      // set the path address and path for a path line
      try {
        int delimeterPosition = firstField.lastIndexOf('-');
        path = firstField.substring(0, delimeterPosition);
        String pathOffsetString = firstField.substring(delimeterPosition + 1);
        pathOffset = Integer.parseInt(pathOffsetString);
      } catch (Exception e) {
        // malformed feature input
        WLog.log("malformed path line");
      }

    } else {
      // leave alone
    }
  }

  /**
   * The <code>FeatureLineType</code> class supports static feature line types.
   * Feature line type specifiers must belonging to this class.
   */
  public static final class FeatureLineType {

    // feature line types
    public static final FeatureLineType ADDRESS_LINE = new FeatureLineType("Address");
    public static final FeatureLineType PATH_LINE = new FeatureLineType("Path");
    public static final FeatureLineType HISTOGRAM_LINE = new FeatureLineType("Histogram");
    public static final FeatureLineType FILE_LINE = new FeatureLineType("Address or Path in a File");
    public static final FeatureLineType INDETERMINATE_LINE = new FeatureLineType("Indeterminate");

    /**
     * The name of the feature line type.
     */
    private final String name;
    private FeatureLineType(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }

  private boolean isAddress(byte[] bytes) {
    int i;
    // digits followed by a tab
    for (i = 0; i<bytes.length; i++) {
      if (bytes[i] < '0' || bytes[i] > '9') {
        break;
      }
    }
    if (i > 0 && bytes[i] == '\t') {
      return true;
    } else {
      return false;
    }
  }

  private boolean isHistogram(byte[] bytes) {
    int i;
    // "n=<1><tab><text><\n>"
    if (bytes.length > 4 && bytes[0] == 'n' && bytes[1] == '=') {
      for (i = 2; i<bytes.length; i++) {
        if (bytes[i] < '0' || bytes[i] > '9') {
          break;
        }
      }
      if (i > 2 && bytes[i] == '\t') {
        // the whole template matches
        return true;
      }
    }
    return false;
  }

  private boolean isPath(byte[] bytes) {
    int i;
    // loosely: digits followed by a minus sign followed by any bytes followed by tab
    for (i = 0; i<bytes.length; i++) {
      if (bytes[i] < '0' || bytes[i] > '9') {
        break;
      }
    }
    if (i > 0 && bytes[i] == '-') {
      for ( i++; i<bytes.length; i++) {
        if (bytes[i] == '\t') {
          // the match is sufficient to be classified as a path
          return true;
        }
      }
    }
    return false;
  }

  private boolean isFile(byte[] bytes) {
    int i;
    // the feature has a filename in it, as indicated by a U+10001C mark.
    for (i = 0; i<bytes.length-5; i++) {
      if (bytes[i] == (byte)0xf4 && bytes[i+1] == (byte)0x80 && bytes[i+2] == (byte)0x80 && bytes[i+3] == (byte)0x9c) {
        // U+10001C found
        return true;
      }
    }
    return false;
  }

  private byte[] stripFileMark(byte[] bytes) {
    ByteArrayOutputStream s = new ByteArrayOutputStream();
    // replace the U+10001C mark with a space
    for (int i = 0; i<bytes.length-5; i++) {
      if (bytes[i] == (byte)0xf4 && bytes[i+1] == (byte)0x80 && bytes[i+2] == (byte)0x80 && bytes[i+3] == (byte)0x9c) {
        // U+10001C found
        s.write(bytes, 0, i);
        s.write('+');
        s.write(bytes, i+4, bytes.length-i-4);
        return s.toByteArray();
      }
    }
    throw new RuntimeException("Invalid usage");
  }

  /**
   * Returns the image file associated with the feature line or null if one is not associated.
   */
  public File getImageFile() {
    return imageFile;
  }

  /**
   * Returns the feature file associated with the feature line.
   */
  public File getFeaturesFile() {
    return featuresFile;
  }

  /**
   * Returns the start byte of the feature line in the feature file, used in bookmarking.
   */
  public long getStartByte() {
    return startByte;
  }

  /**
   * Returns the number of bytes of the feature line in the feature file, used in bookmarking.
   */
  public int getNumBytes() {
    return numBytes;
  }

  /**
   * Returns the offset associated with the feature line.
   */
  public long getAddress() {
    // program error if the line has no address
    if (featureLineType == FeatureLineType.ADDRESS_LINE) {
      return address;
    } else if (featureLineType == FeatureLineType.PATH_LINE) {
      return pathOffset;
    } else { // HISTOGRAM, file line, or indeterminate
      return 0;
    }
  }

  /**
   * Returns the path associated with the path feature line,
   * specifically, the typed field without the offset.
   */
  public String getPath() {
    // program error if the line has no address
    if (featureLineType != FeatureLineType.PATH_LINE) {
      throw new RuntimeException("Invalide feature line type in getAddress: "
                                 + featureLineType.toString());
    }
    return path;
  }

  /**
   * Returns the feature line type.
   */
  public FeatureLineType getType() {
    return featureLineType;
  }

  /**
   * Returns the first field, whose value dictates its type, specifically,
   * an offset for addresses or the path plus offset for paths, or the histogram text.
   */
  public String getFirstField() {
    return firstField;
  }

  /**
   * Returns the first field formatted as requested, based on line type.
   */
  public String getFormattedFirstField(String featureAddressFormat) {
    if (featureLineType == FeatureLineType.ADDRESS_LINE) {
      return String.format(featureAddressFormat, address);
    } else if (featureLineType == FeatureLineType.PATH_LINE) {
      return String.format(featureAddressFormat, pathOffset) + "(" + path + ")";
    } else if (featureLineType == FeatureLineType.HISTOGRAM_LINE) {
      return firstField;
    } else if (featureLineType == FeatureLineType.FILE_LINE) {
      return firstField;
    } else {
      WLog.log("FeatureLine.getFormattedFirstField: no line type.");
      return "";
    }
  }

  /**
   * Returns the feature text field portion of the feature line.
   */
  public byte[] getFeatureField() {
    return featureField;
  }

  /**
   * Returns feature text associated with this feature line formatted according to its type
   * as indicated by the filename of the Feature File associated with this feature.
   */
  public String getFormattedFeatureText() {
    return FeatureFieldFormatter.getFormattedFeatureText(this);
  }

  /**
   * Returns text expected in the image that may be highlighted, formatted according to its type
   * as indicated by the filename of the Feature File associated with this feature.
   */
  public Vector<byte[]> getImageHighlightVector() {
    return FeatureFieldFormatter.getImageHighlightVector(this);
  }

  /**
   * Returns the context field portion of the feature line.
   */
  public byte[] getContextField() {
    return contextField;
  }

  /**
   * Returns a printable summary string of this feature line.
   */
  public String getSummaryString() {
    // compose the summary
    String summary;
    if (imageFile == null) {
      if (featuresFile == null) {
        // this string is returned by the FeatureListCellRenderer as the prototype display value
        return "no features file";
      } else {
        // indicate what is available
        summary = "No image file selected, " + firstField + ", " + getFormattedFeatureText();
      }
    } else {
      summary = imageFile.getName() + ", " + firstField + ", " + getFormattedFeatureText();
    }

    // truncate the sumamry
    if (summary.length() > 100) {
      summary = summary.substring(0, 100) + "\u2026";
    }

    return summary;
  }

  /**
   * Returns a printable summary string of the given feature line.
   */
  public static String getSummaryString(FeatureLine featureLine) {
    if (featureLine == null) {
      return "No Image file and no Feature selected.";
    } else {
      return featureLine.getSummaryString();
    }
  }

  /**
   * Returns a string detailing the state of the feature line.
   */
  public String toString() {
    StringBuffer stringBuffer = new StringBuffer();
    stringBuffer.append("type: " + featureLineType.name);
    stringBuffer.append(", typedfield: " + firstField);
    stringBuffer.append(", feature: '" + getFormattedFeatureText());
//    stringBuffer.append(", feature: '" + getFeatureField());
//    stringBuffer.append("', context: '" + getContextField());
    if (featureLineType.equals(FeatureLineType.ADDRESS_LINE)) {
      stringBuffer.append("', Offset: " + getAddress());
    }
    if (featureLineType.equals(FeatureLineType.PATH_LINE)) {
      stringBuffer.append("', Offset: " + getAddress());
      stringBuffer.append("', Path: " + path);
    }
    stringBuffer.append(" imageFile: " + imageFile);
    stringBuffer.append(" featuresFile: " + featuresFile);
    return stringBuffer.toString();
  }

  /**
   * Identifies whether the feature line is from the given report
   */
  public boolean isFromReport(ReportsModel.ReportTreeNode reportTreeNode) {
    // image file and features directory must be equivalent
    File reportImageFile = reportTreeNode.imageFile;
    File reportFeaturesDirectory = reportTreeNode.featuresDirectory;
    File featuresDirectory = (featuresFile == null) ? null : featuresFile.getParentFile();
    return (FileTools.filesAreEqual(reportImageFile, imageFile)
     && FileTools.filesAreEqual(reportFeaturesDirectory, featuresDirectory));
  }

  /**
   * Identifies when two FeatureLine objects are equivalent
   * @return true when equivalent
   */
  public boolean equals(FeatureLine featureLine) {
    return (featureLine != null
            && FileTools.filesAreEqual(imageFile, featureLine.imageFile)
            && FileTools.filesAreEqual(featuresFile, featureLine.featuresFile)
            && startByte == featureLine.startByte
            && numBytes == featureLine.numBytes);
  }

  public int hashCode() {
    // sufficient effort to avoid collision
    return (int)startByte;
  }
}


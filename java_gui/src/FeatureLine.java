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
  public final File reportImageFile;
  public final File featuresFile;
  public final long startByte;
  public final int numBytes;

  // extracted fields
  public final byte[] firstField; // may be path with possible file or histogram
  public final byte[] featureField;
  public final byte[] contextField;

  // derived fields
  /**
   * The actual image file used if the report image file is a directory
   * and the actual image is in the first field.
   */
  public final File actualImageFile;
  /**
   * firstField, without filename, if present.
   */
  public final String forensicPath;
  /**
   * firstField without filename, if present
   */
  public final String formattedFeature; // format is based on feature file type

  // note if initialization failed
  private boolean isFailedInitialization;

  /**
   * Constructs a blank FeatureLine because FeatureListCellRenderer requires
   * a prototype display value in order to get the height right.
   */
  public FeatureLine() {
    reportImageFile = null;
    featuresFile = null;
    startByte = 0;
    numBytes = 0;

    firstField = new byte[0];
    featureField = new byte[0];
    contextField = new byte[0];

    actualImageFile = null;
    forensicPath = "";
    formattedFeature = "";

    isFailedInitialization = true;
  }

  /**
   * Constructs a FeatureLine object for containing attributes realting to a feature line
   * @param reportImageFile the image file associated with the report for the feature line
   * @param featuresFile the features file file associated with the feature line
   * @param startByte the start byte in the features file where the feature line text resides
   * @param numBytes the length of text in the features file defining this feature line
   * @param isUseHexPath whether to format the printable forensic path in hex
   */
  public FeatureLine(File reportImageFile, File featuresFile, long startByte, int numBytes) {

    // record input parameters
    this.reportImageFile = reportImageFile;
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

      isFailedInitialization = false;
    } catch (Exception e) {

      // indicate error
      if (!featuresFile.exists()) {
        // the file doesn't exist
        WError.showErrorLater( "Features file " + featuresFile + " does not exist.", "BEViewer file error", null);
      } else {
        // something more went wrong so show the exception
        WError.showErrorLater( "Unable to open features file " + featuresFile + ".", "BEViewer file error", e);
      }

      // set remaining values for failed read
      firstField = new byte[0];
      featureField = new byte[0];
      contextField = new byte[0];
      actualImageFile = null;
      forensicPath = "";
      formattedFeature = "";
      isFailedInitialization = true;
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

    int i;
    int j;
    // define firstField as text before first tab else all text
    for (i=0; i < numBytes; i++) {
      if (lineBytes[i] == '\t') {
        // at first tab
        break;
      }
    }

    // firstField is text before first tab
    ByteArrayOutputStream firstFieldStream = new ByteArrayOutputStream();
    firstFieldStream.write(lineBytes, 0, i);
    firstField = firstFieldStream.toByteArray();

    // featureField is text after first tab and before second tab
    // else remaining text
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

    // define the image file that is actually associated with this forensic path
    if (ForensicPath.hasFilename(firstField)) {
      // NOTE: currently, the user cannot move the directory of image files.
      // This can be fixed by recomposing the path based on the path to the
      // Report's image, but still, it is not compatible between Win and Linux
      // because the filename embedded in the path is sysem-specific.
      // Generic slash management should fix that, but not now.
      actualImageFile = new File(ForensicPath.getFilename(firstField));
    } else {
      actualImageFile = reportImageFile;
    }

    // set derived fields
    forensicPath = ForensicPath.getPathWithoutFilename(firstField);
    formattedFeature = FeatureFieldFormatter.getFormattedFeatureText(featuresFile, featureField, contextField);
  }

  /**
   * Returns a printable summary string of this feature line.
   */
  public String getSummaryString() {
    // compose a printable summary that fits on one line
    String summary;
    if (actualImageFile == null) {
      if (featuresFile == null) {
        // this string is returned by the FeatureListCellRenderer as the prototype display value
        return "no image file and no features file";
      } else {
        // indicate what is available
        summary = "No image file selected, " + firstField + ", " + formattedFeature;
      }
    } else {
      summary = ForensicPath.getPrintablePath(forensicPath, BEViewer.imageView.getUseHexPath())
               + ", " + featuresFile.getName()
               + ", " + actualImageFile.getName()
               + ", " + formattedFeature;
    }

    // truncate the sumamry
    final int MAX_LENGTH = 200;
    if (summary.length() > MAX_LENGTH) {
      summary = summary.substring(0, MAX_LENGTH) + "\u2026";
    }

    return summary;
  }

  /**
   * Identifies whether this is really a blank feature line.
   */
  public boolean isBlank() {
    return (reportImageFile == null && featuresFile == null);
  }

  /**
   * Identifies if this FeatureLine was unable to initialize.
   */
  public boolean isBad() {
    return isFailedInitialization;
  }

  /**
   * Returns a printable summary string of the given feature line.
   */
  public static String getSummaryString(FeatureLine featureLine) {
    return featureLine.getSummaryString();
  }

  /**
   * Returns a string detailing the state of the feature line.
   */
  public String toString() {
    StringBuffer stringBuffer = new StringBuffer();

    // input fields
    stringBuffer.append("reportImageFile: " + reportImageFile);
    stringBuffer.append(", actualImageFile: " + actualImageFile);
    stringBuffer.append(", featuresFile: " + featuresFile);
    stringBuffer.append(", startByte: " + startByte);
    stringBuffer.append(", numBytes: " + numBytes);

    // derived fields
    stringBuffer.append(", firstField: " + firstField);
    stringBuffer.append(", forensicPath: " + forensicPath);
    stringBuffer.append(", featureField: " + featureField);
    stringBuffer.append(", contextField: " + contextField);
    return stringBuffer.toString();
  }

  /**
   * Identifies whether the feature line is from the given report
   */
  public boolean isFromReport(ReportsModel.ReportTreeNode reportTreeNode) {
    // image file and features directory must be equivalent
    File reportImageFile = reportTreeNode.reportImageFile;
    File reportFeaturesDirectory = reportTreeNode.featuresDirectory;
    File featuresDirectory = (featuresFile == null) ? null : featuresFile.getParentFile();
    return (FileTools.filesAreEqual(reportImageFile, reportImageFile)
     && FileTools.filesAreEqual(reportFeaturesDirectory, featuresDirectory));
  }

  /**
   * Identifies when two FeatureLine objects are equivalent
   * @return true when equivalent
   */
  public boolean equals(FeatureLine featureLine) {
    return (FileTools.filesAreEqual(reportImageFile, featureLine.reportImageFile)
            && FileTools.filesAreEqual(featuresFile, featureLine.featuresFile)
            && startByte == featureLine.startByte
            && numBytes == featureLine.numBytes);
  }

  public int hashCode() {
    // sufficient effort to avoid collision
    return (int)startByte;
  }
}


import java.util.Vector;
import java.util.Enumeration;
import java.io.File;
import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.util.Observer;
import java.util.Observable;
import javax.swing.SwingUtilities;

/**
 * This class produces image highlight flag output from the models that offer highlighting.
 */

public class ImageHighlightProducer {

  // resources

  // resources from models used temporarily during calculation
  private byte[] paddedPageBytes;
  private int paddingPrefixSize;
  private int availablePageSize;
  private boolean highlightMatchCase;

  // calculated
  private boolean[] pageHighlightFlags;

  /**
   * Creates a highlight producer for calculating page highlight flags.
   */
  public ImageHighlightProducer() {
  }

  /**
   * Returns highlight flags based on highlight values in models.
   */
  public boolean[] getPageHighlightFlags(ImageModel.ImagePage imagePage,
                                         Vector<byte[]> userHighlights,
                                         boolean highlightMatchCase) {

    // cache some page parameters
    paddedPageBytes = imagePage.paddedPageBytes;
    paddingPrefixSize = imagePage.paddingPrefixSize;
    availablePageSize = imagePage.pageBytes.length;

    // initialize the highlight flags array given the available page size
    pageHighlightFlags = new boolean[availablePageSize];

    // set highlightMatchCase
    this.highlightMatchCase = highlightMatchCase;

    // set highlight flags for the image page
    setFlags(FeatureFieldFormatter.getImageHighlightVector(imagePage));

    // set highlight flags for each user highlight
    if (userHighlights != null) {
      setFlags(userHighlights);
    }

    return pageHighlightFlags;
  }

  private byte[] getUTF16Bytes(byte[] utf8Bytes) {
    int utf8Length = utf8Bytes.length;
    if (utf8Length == 0) {
      return utf8Bytes;
    }
    byte[] utf16Bytes = new byte[utf8Length * 2 - 1];
    for (int i=0; i<utf8Length; i++) {
      utf16Bytes[i*2] = utf8Bytes[i];
    }
    return utf16Bytes;
  }

  private void setFlags(Vector<byte[]> highlightVector) {
    for (Enumeration<byte[]> e = highlightVector.elements(); e.hasMoreElements(); ) {
      byte[] highlightBytes = e.nextElement();

      // remove escape codes
      byte[] unescapedHighlightBytes = UTF8Tools.unescapeBytes(highlightBytes);

      // set highlight flags for UTF8
      addHighlightFlags(unescapedHighlightBytes);

      // set highlight flags for UTF16
      addHighlightFlags(getUTF16Bytes(unescapedHighlightBytes));
    }
  }

  // create flags in overlapHighlightFlags and then OR them to pageHighlightFlags
  private void addHighlightFlags(byte[] highlightBytes) {
    int highlightLength = highlightBytes.length;
    int numPaddedBytes = paddedPageBytes.length;

//WLog.log("ImageModel.addHighlightFlags: " + highlightLength + ", " + numPaddedBytes);
    // prepare the overlap highlight flags
    boolean[] overlapHighlightFlags = new boolean[numPaddedBytes];
    for (int n = 0; n<numPaddedBytes; n++) {
      overlapHighlightFlags[n] = false;
    }

    int i;
    int j;
    
    // set highlight flags for byte matches
    for (i=0; i<numPaddedBytes - highlightLength; i++) {
      boolean match = true;
      for (j = 0; j < highlightLength; j++) {
        byte paddedByte = paddedPageBytes[i + j];
        byte highlightByte = highlightBytes[j];

        // manage match case
        if (!highlightMatchCase) {
          if (paddedByte >= 0x41 && paddedByte <= 0x5a) {
            // change upper case to lower case
            paddedByte += 0x20;
          }
          if (highlightByte >= 0x41 && highlightByte <= 0x5a) {
            // change upper case to lower case
            highlightByte += 0x20;
          }
        }

        // check for match between bytes in position
        if (paddedByte != highlightByte) {
          match = false;
          break;
        }
      }
      if (match) {
        for (j = 0; j < highlightLength; j++) {
          overlapHighlightFlags[i + j] = true;
//WLog.log("ImageModel.addHighlightFlags.ohf " + i+j);
        }
      }
    }

    // add identified overlapHighlightFlags to pageHighlightFlags
    for (i=0; i<availablePageSize; i++) {
      if (overlapHighlightFlags[i + paddingPrefixSize] == true) {
        pageHighlightFlags[i] = true;
      }
    }
  }
}


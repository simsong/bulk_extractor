import java.util.Observer;
import java.util.HashMap;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import javax.swing.SwingUtilities;

/**
 * The <code>FeaturesParserThread</code> class creates a Feature Line Table.
 * When this thread is done, it calls FeaturesModel.setFeatureLineTable.
 * This thread may be aborted by calling setAbortRequest().
 * This class manages FeaturesModel.progressBar visibility and progress.
 */
public class FeaturesParserThread extends Thread {

  // private Model properties
  private final FeaturesModel callback;
  private final WProgress progressBar;
  private final File imageFile;
  private final File featuresFile;
  private final byte[] requestedFilterBytes;
  private final boolean filterMatchCase;
  private final boolean useHexPath;

  // permit abort
  private boolean abortRequest = false;

  // line and filter states
  private final int PARSING_START = 0;
  private final int PARSING_TYPE_FIELD = 1;
  private final int PARSING_FEATURE_FIELD = 2;
  private final int PARSING_CONTEXT_FIELD = 3;
  private final int PARSING_DROP = 4;
  private final int PARSING_KEEP = 5;
  private final int FILTER_OFF = 0;	// no filtering
  private final int FILTER_ON = 1;	// filtering
  private int parseState;
  private int filterState;
  private byte[] utf8Filter = null;
  private byte[] utf16Filter = null;

  // shared with remainderMatches()
  private FileInputStream fileInputStream;
  private FileChannel fileChannel;
  private long fileSize;
  private byte[] bytes = new byte[65536];	// arbitrary read size
  private int thisNumBytesRead;
  private int thisByteOffset;
  private long runningNumBytesRead = 0;

  // the feature line table that is generated
  private FeatureLineTable featureLineTable;

  public FeaturesParserThread(FeaturesModel callback, File imageFile, File featuresFile,
                    byte[] requestedFilterBytes, boolean filterMatchCase, boolean useHexPath) {

    // set model attributes
    featureLineTable = new FeatureLineTable(imageFile, featuresFile);
    this.progressBar = callback.progressBar;
    this.callback = callback;
    this.imageFile = imageFile;
    this.featuresFile = featuresFile;
    this.requestedFilterBytes = requestedFilterBytes;
    this.filterMatchCase = filterMatchCase;
    this.useHexPath = useHexPath;
  }

  // run the parser
  public void run() {
    // Start the progress bar.
    progressBar.setActive(true);
//WLog.log("FeaturesParserThread.run for callback " + callback + " featuresFile: " + featuresFile);

    try {
      parseFile();
    } catch (OutOfMemoryError e) {
      WError.showError(
          "Out of memory error.  Use the text filter to create a smaller features list\nor run with more Java Heap Space, for example: 'java -Xmx1g -jar BEViewer.jar'.",
          "BEViewer Memory error", e);
    } finally {
      // Stop the progress bar.
      progressBar.setActive(false);

      // commit the change into the UI
      commitChange(featureLineTable);
    }
  }

  // set the parser to abort when it starts its next parsing iteration
  public synchronized void setAbortRequest() {
    abortRequest = true;
  }

  // Parse the features file
  private void parseFile() {
    // set initial state
    parseState = PARSING_START;
    runningNumBytesRead = 0;
    long lineStartByte = 0;

    // a null features file is valid: simply generate no features.
    if (featuresFile == null) {
      return;
    }

    // establish the file channel and file size for the features file
    try {
      fileInputStream = new FileInputStream(featuresFile);

      // look for the UTF-8 Byte Order Mark (BOM)
      final byte[] utf8BOM = {(byte)0xef, (byte)0xbb, (byte)0xbf};
      byte[] bom = new byte[3];
      fileInputStream.read(bom);
      if (bom[0] == utf8BOM[0] && bom[1] == utf8BOM[1] && bom[2] == utf8BOM[2]) {
        // good, BOM matches, so advance past it
        runningNumBytesRead = 3;
      } else {
        // leave pointer alone for compatibility with the pre-bulk_extractor 1.3 formt
      }

    } catch (IOException e) {
      if (!featuresFile.exists()) {
        // the file simply does not exist
        WError.showError(
            "Features file " + featuresFile + " does not exist.", "BEViewer file error", null);
      } else {
        // something more is wrong
        WError.showError(
            "Unable to open features file " + featuresFile + ".", "BEViewer file error", e);
      }
      // do not build up FeatureLineTable
      return;
    }
    fileChannel = fileInputStream.getChannel();
    try {
      fileSize = fileChannel.size();
    } catch (IOException e) {
      try {
        fileInputStream.close();
      } catch (IOException ioe) {
        // at least we tried to release the resource
      }
      WError.showError(
            "Unable to access channel in file " + featuresFile + ".", "BEViewer file error", e);
      // do not build up FeatureLineTable
      return;
    }

    // set filter state and filter bytes based on filter text and case requirement
    if (requestedFilterBytes.length == 0) {
      filterState = FILTER_OFF;
    } else {
      // set filter based on text case request
      byte[] filter;
      filterState = FILTER_ON;
      if (filterMatchCase == true) {
        // filtering, case sensitive
        filter = requestedFilterBytes;
      } else {
        // filtering, upper or lower case
        filter = UTF8Tools.toLower(requestedFilterBytes);
      }

      // set utf8Filter and utf16Filter based on filter
      if (UTF8Tools.escapedLooksLikeUTF16(filter)) {
        // calculate UTF8 and set UTF16
        utf8Filter = UTF8Tools.utf16To8Basic(filter);
        utf16Filter = filter;
      } else {
        // set UTF8 and calculate UTF16
        utf8Filter = filter;
        utf16Filter = UTF8Tools.utf8To16Basic(filter);
      }
    }

    // parse file in byte sections
    while (runningNumBytesRead < fileSize) {

      // abort on interrupt
      if (abortRequest == true) {
        featureLineTable = new FeatureLineTable(null, null);
        return;
      }

      // update progress indicator
      progressBar.setPercent((int)(runningNumBytesRead * 100 / fileSize));

      // calculate the number of bytes to read
      long temp = fileSize - runningNumBytesRead;
      thisNumBytesRead = (int)((temp > bytes.length) ? bytes.length : temp);

      // read portion of file into bytes[]
      try {
        MappedByteBuffer mappedByteBuffer = fileChannel.map(
                         MapMode.READ_ONLY, runningNumBytesRead, thisNumBytesRead);
        mappedByteBuffer.load();
        mappedByteBuffer.get(bytes, 0, thisNumBytesRead);
      } catch (Exception e) {
        // warn and abort
        WError.showError( "Unable to read file " + featuresFile + ".",
                          "BEViewer file read error", e);
        break;
      }

      // parse bytes[]
      // NOTE: this algorithm requires that the file ends with '\n' (0x0a)
      for (thisByteOffset=0; thisByteOffset < thisNumBytesRead; thisByteOffset++) {
        byte b = bytes[thisByteOffset];

        if (b == '\n') {
          // calculate the line stop byte, which falls on top of the newline byte
          long lineStopByte = runningNumBytesRead + thisByteOffset;

          if ((parseState == PARSING_KEEP)
           || (filterState == FILTER_OFF && parseState != PARSING_DROP)) {
            // keep the line

            // calculate the line length, which excludes the newline byte
            long longLineLength = lineStopByte - lineStartByte;

            // make sure line length is not broken
            if (longLineLength > Integer.MAX_VALUE) {
              throw new RuntimeException("Unexpected feature line length");
            }

            // keep the line, which excludes the newline byte
            featureLineTable.put(lineStartByte, (int)longLineLength);

          } else {
            // drop the line
          }

          // move the start pointer to one past the newline byte
          lineStartByte = lineStopByte + 1;

          // any state transitions to start line
          parseState = PARSING_START;

        } else {

          switch (parseState) {
            case PARSING_START:
              if (b == '#') {
                // line begins with comment
                parseState = PARSING_DROP;
              } else if (b == '\t') {
                // line begins with tab (input is malformed)
                parseState = PARSING_FEATURE_FIELD;
              } else {
                // transition to type field until transition to feature field by a tab
                parseState = PARSING_TYPE_FIELD;
              }
              break;

            case PARSING_TYPE_FIELD:
              if (b == '\t') {
                // tab transitions to feature
                parseState = PARSING_FEATURE_FIELD;
              }
              break;

            case PARSING_FEATURE_FIELD:
              // transition to context field if tab
              if (b == '\t') {
                // tab transitions to context
                parseState = PARSING_CONTEXT_FIELD;

              // transition to KEEP if feature matches
              } else if (filterState == FILTER_ON) {
                if (!filterMatchCase) {
                  if (b >= 0x41 && b <= 0x5a) {
                    b+= 0x20;
                  }
                }

                // if first byte matches, check the whole filter for a match
                if (b == utf8Filter[0]) { // note that filter[0] is the same as utf16Filter[0]
                  if (remainderMatches(utf8Filter) || remainderMatches(utf16Filter)) {
                    parseState = PARSING_KEEP;
                  }
                }
              }
              break;

            case PARSING_CONTEXT_FIELD:
              // no action;
              break;
            case PARSING_DROP:
              // no action;
              break;
            case PARSING_KEEP:
              // no action;
              break;
          } // switch
        } // else
      } // for

      // update runningNumBytesRead
      runningNumBytesRead += thisNumBytesRead;

    } // while

    // close resources
    try {
      fileInputStream.close();
      fileInputStream = null;
    } catch (IOException ioe1) {
      WLog.log("FeaturesModel.parseFile unable to close fileInputStream");
    }
    try {
      fileChannel.close();
      fileChannel = null;
    } catch (IOException ioe2) {
      WLog.log("FeaturesModel.parseFile unable to close fileChannel");
    }
  } // parseFile()

  // when the first byte matches the filter string, check for a total filter string match
  // note: do not check byte 0 because it has already been checked.
  private boolean remainderMatches(byte[] filterBytes) {

    int filterLength = filterBytes.length;

    if (filterLength + thisByteOffset <= thisNumBytesRead) {
      // the potential match is within the bytes[] array, so match directly on bytes[]
      for (int i=1; i<filterLength; i++) {
        byte b = bytes[thisByteOffset + i];
        if (!filterMatchCase) {
          if (b >= 0x41 && b <= 0x5a) {
            // change upper case to lower case
            b += 0x20;
          }
        }
        if (b != filterBytes[i]) {
          return false;
        }
      }
      return true;
    } else {

      // the potential match crosses outside the bytes[] array or extends into EOF, so match buffer
      if (runningNumBytesRead + thisByteOffset + filterLength > fileSize) {
        // at EOF, so the filter cannot match
        return false;
      }

      // read into overlap buffer and check overlap buffer for potential match
      long overlapStartByte = runningNumBytesRead + thisByteOffset;
      byte[] overlapBytes = new byte[filterLength];
      try {
        MappedByteBuffer overlapBuffer = fileChannel.map(
                 MapMode.READ_ONLY, overlapStartByte, filterLength);
        overlapBuffer.load();
        overlapBuffer.get(overlapBytes, 0, filterLength);
      } catch (Exception e) {
        WError.showError( "Unable to read file " + featuresFile + ".",
                          "BEViewer file read error", e);
        throw new RuntimeException("File error");
        
      }

      // the potential match is within the overlap buffer
      for (int i=1; i<filterLength; i++) {
        byte b = overlapBytes[i];
        if (!filterMatchCase) {
          if (b >= 0x41 && b <= 0x5a) {
            // change upper case to lower case
            b += 0x20;
          }
        }
        if (b != filterBytes[i]) {
          return false;
        }
      }
      return true;
    }
//    throw new RuntimeException("flow error");
  } // remainderMatches

  private void commitChange(FeatureLineTable featureLineTable) {

    // integrate the change into the model
    DoCallback doCallback = new DoCallback(callback, featureLineTable);
    SwingUtilities.invokeLater(doCallback);
  }

  private static class DoCallback implements Runnable {
    private final FeaturesModel callback;
    private final FeatureLineTable featureLineTable;
    public DoCallback(FeaturesModel callback, FeatureLineTable featureLineTable) {
      this.callback = callback;
      this.featureLineTable = featureLineTable;
    }
    public void run() {
      callback.setFeatureLineTable(featureLineTable);
    }
  }
}


import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.ByteArrayOutputStream;
import java.lang.Runtime;
import java.lang.Process;
import java.util.Observer;
import java.util.Observable;

/**
 * The <code>BulkExtractorFileReader</code> class provides path reading services
 * using the bulk_extractor utility, which is capable of extracting path buffers.
 */
public class BulkExtractorFileReader {

  private static final int DELAY = 60 * 1000; // 12 is good for testing
  private File file;
  private Process process;
  private PrintWriter writeToProcess;
  private InputStream readFromProcess;
  ThreadReaderModel stderrThread;

  // input request
  // NOTE: read operations are cached for optimization:
  //       when a read request matches the last read request, do not re-read.
  private FeatureLine featureLine = null;
  private long startAddress;
  private int numBytes;

  // cached output
  private long imageSize = 0;
  private long pathSize = 0;
  private byte[] bytes = new byte[0];

  /**
   * Creates an image reader
   * If the reader fails then an error dialog is displayed
   * and an empty reader is returned in its place.
   * @param newFile the file
   * @throws IOException if the reader cannot be created
   */
  public BulkExtractorFileReader(File newFile) throws IOException {
    file = newFile;

    // start interactive bulk_extractor providing image filename and the requisite library path
    String[] cmd = new String[4];	// "bulk_extractor -p -http <image_file>"
//    cmd[0] = "/usr/local/bin/bulk_extractor";
    cmd[0] = "bulk_extractor";
    cmd[1] = "-p";
    cmd[2] = "-http";
    cmd[3] = file.getAbsolutePath();
//    String[] envp = new String[1];	// LD_LIBRARY_PATH
    String[] envp = new String[0];	// LD_LIBRARY_PATH
//    envp[0] = "LD_LIBRARY_PATH=/usr/local/lib";

    WLog.log("BulkExtractorFileReader starting bulk_extractor process");
    WLog.log("BulkExtractorFileReader cmd: " + cmd[0] + " " + cmd[1] + " " + cmd[2] + " " + cmd[3]);
//    WLog.log("BulkExtractorFileReader envp: " + envp[0]);

    process = Runtime.getRuntime().exec(cmd, envp);
    writeToProcess = new PrintWriter(process.getOutputStream());
    readFromProcess = process.getInputStream();
    BufferedReader errorFromProcess = new BufferedReader(new InputStreamReader(process.getErrorStream()));

    // thread for consuming stderr
    stderrThread = new ThreadReaderModel(errorFromProcess);
    stderrThread.addReaderModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        String line = (String)arg;
        WLog.log("BulkExtractorFileReader error from bulk_extractor process " + process + ": " + line);
      }
    });

    // read the image size
    imageSize = readImageSize();
  }

  /**
   * indicates whether bulk_extractor process is alive.
   * @return true if bulk_extractor is alive, false if not
   */
  public boolean isAlive() {
    // see if the reader's bulk_extractor process is still alive
    // unfortunately, the way to do this is to poll exitValue, which traps when alive.
    try {
      int status = process.exitValue();
      // this is bad
      WLog.log("BulkExtractorFileReader process terminated, status: " + status);
      return false;
    } catch (IllegalThreadStateException e) {
      // this is good
    }
    return true;
  }

  // read the total image size
  private long readImageSize() throws IOException {
    long parsedImageSize;
    String getString = "GET /info HTTP/1.1";

    // issue the HTTP GET request
    WLog.log("BulkExtractorFileReader bulk_extractor size request 1: " + getString);
    writeToProcess.println(getString);
    writeToProcess.println();
    writeToProcess.flush();

    // read response lines

    // X-Image-Size: provides the total image size
    final String X_IMAGE_SIZE = "X-Image-Size: ";
    String imageSizeLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor size response 1: X-Image-Size: '" + imageSizeLine + "'");
    if (!imageSizeLine.startsWith(X_IMAGE_SIZE)) {
      throw new IOException("Invalid image size line: '" + imageSizeLine + "'");
    }
    String imageSizeString = imageSizeLine.substring(X_IMAGE_SIZE.length());
    try {
      // parse out image size from the line read
      parsedImageSize = Long.valueOf(imageSizeString).longValue();
    } catch (NumberFormatException e) {
      throw new IOException("Invalid numeric format in image size line: '" + imageSizeLine + "'");
    }

    // X-Image-Filename: provides the image filename, which is unused
    final String X_IMAGE_FILENAME = "X-Image-Filename: ";
    String imageFilenameLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor size response 2: X-Image-Filename: '" + imageFilenameLine + "'");
    if (!imageFilenameLine.startsWith(X_IMAGE_FILENAME)) {
      throw new IOException("Invalid image filename line: '" + imageFilenameLine + "'");
    }

    // Content-Length: provides the number of bytes returned, which is unused
    final String CONTENT_LENGTH = "Content-Length: ";
    String contentLengthLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor size response 3: Content-Length: '" + contentLengthLine + "'");
    if (!contentLengthLine.startsWith(CONTENT_LENGTH)) {
      throw new IOException("Invalid content length line: '" + contentLengthLine + "'");
    }

    // blank line
    String blankLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor size response 4: blank line: '" + blankLine + "'");
    if (!blankLine.equals("")) {
      throw new IOException("Invalid blank line: '" + blankLine + "'");
    }

    // make sure stdin is now clear
    failIfMoreBytesAvailable();

    return parsedImageSize;
  }

  // read line terminated by \r\n
  private String readLine() throws IOException {
    int b;
    ByteArrayOutputStream outStream = new ByteArrayOutputStream();

    // start watchdog for this read
    ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

    // read bytes through required http \r\n line terminator
    while (true) {
      b = readByte();
      if (b != '\r') {
        // add byte to line
        outStream.write(b);

      } else {
        // remove the \n following the \r
        b = readByte();

        // verify \n
        if (b != '\n') {
          throw new IOException("Invalid line terminator returned from bulk_extractor: " + b);
        }
        break;
      }
    }

    // stop watchdog for this read
    aborter.cancel();

    // convert bytes to String
    String line = outStream.toString();

    return line;
  }

  // read the requested number of bytes
  private byte[] readBytes(int numBytes) throws IOException {
    byte[] bytes = new byte[numBytes];

    // start watchdog for this read
    ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

    // read each byte
    for (int i=0; i<numBytes; i++) {
      bytes[i] = readByte();
    }

    // stop watchdog
    aborter.cancel();

    return bytes;
  }

  // read byte, failing on EOF
  private byte readByte() throws IOException {
    int bInt = readFromProcess.read();
    if (bInt > 256) {
      throw new IOException("Invalid byte returned from bulk_extractor: " + bInt);
    } else if (bInt < 0) {
      // the stream has terminated
      throw new IOException("bulk_extractor stream terminated by byte: " + bInt);
    } else {
      return (byte)bInt;
    }
  }

  /**
   * Returns the size in bytes of the buffer for the expected FeatureLine
   * @param featureLine the feature line for which the path size is to be obtained
   * @return the size in bytes of the buffer at the path extracted
   */
  public long getBulkExtractorPathSize(FeatureLine featureLine) throws IOException {
    // the path size depends on the feature line type
    if (featureLine.getType() == FeatureLine.FeatureLineType.ADDRESS_LINE) {
      // for ADDRESS type return the image size
      return imageSize;
    } else if (featureLine.getType() == FeatureLine.FeatureLineType.PATH_LINE) {
      // for PATH type establish the path size for the path then return the path size
      if (featureLine != this.featureLine) {
        // the requested featureLine is not the active featureLine
        // so perform a blank read to obtain pathSize
//        byte[] dummy = readBulkExtractorPathBytes(featureLine, 0, 0);
        readBulkExtractorPathBytes(featureLine, 0, 0);
      }
      // return the path size
      return pathSize;
    } else {
      throw new RuntimeException("Invalide feature line type: " + featureLine.getType().toString());
    }
  }

  /**
   * Reads the requested number of bytes from the path at the specified start address.
   * @param featureLine the feature line identifying the path to be read
   * @param startAddress the address within the image buffer or the image to read
   * @param numBytes the number of bytes to read
   * @throws IOException if the number of bytes cannot be read
   * or if the requested number of bytes cannot be read
   */
  public byte[] readBulkExtractorPathBytes(FeatureLine featureLine, long startAddress, int numBytes) throws IOException {

    // as an optimization, return the cached result if the query is the same,
    // which happens when events such as a highlighting capitalization change occurs.
    if (this.featureLine == featureLine && this.startAddress == startAddress
      && this.numBytes == numBytes) {
      return bytes;
    }
    this.featureLine = featureLine;
    this.startAddress = startAddress;
    this.numBytes = numBytes;

    // make sure stdin is clear
    failIfMoreBytesAvailable();

    // define the start address string
    String startAddressString = Long.toString(startAddress);

    // define the range stop string
    String rangeStopString = Integer.toString((numBytes > 0) ? numBytes - 1 : 0);

    // determine the HTTP GET request
    String pathString;
    if (featureLine.getType() == FeatureLine.FeatureLineType.ADDRESS_LINE) {
      pathString = startAddressString;
    } else if (featureLine.getType() == FeatureLine.FeatureLineType.PATH_LINE) {
      pathString = featureLine.getPath() + "-" + startAddressString;
    } else {
      throw new RuntimeException("Invalide feature line type: " + featureLine.getType().toString());
    }
    String getString = "GET " + pathString + " HTTP/1.1";
    String rangeString = "Range: bytes=0-" + rangeStopString;
    WLog.log("BulkExtractorFileReader bulk_extractor read request 1: " + getString);
    WLog.log("BulkExtractorFileReader bulk_extractor read request 2: " + rangeString);

    // issue the HTTP GET request
    writeToProcess.println(getString);
    writeToProcess.println(rangeString);
    writeToProcess.println();
    writeToProcess.flush();

    // read response lines

    // Content-Length: provides the number of bytes returned
    int numBytesReturned;
    final String CONTENT_LENGTH = "Content-Length: ";
    String contentLengthLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor read response 1: Content-Length: '" + contentLengthLine + "'");
    if (!contentLengthLine.startsWith(CONTENT_LENGTH)) {
      throw new IOException("Invalid content length line: '" + contentLengthLine + "'");
    }
    String contentLengthString = contentLengthLine.substring(CONTENT_LENGTH.length());
    try {
      // parse out bytes returned from the line read
      numBytesReturned = Integer.valueOf(contentLengthString).intValue();
    } catch (NumberFormatException e) {
      throw new IOException("Invalid numeric format in content length line: '" + contentLengthLine + "'");
    }
    // note if numBytesReturned is not numBytes requested, which is expected near EOF
    if (numBytesReturned != numBytes) {
      WLog.log("BulkExtractorFileReader: note: bytes requested: " + numBytes
                                                + ", bytes read: " + numBytesReturned);
    }

    // Content-Range: check for presence of the Content-Range header, but ignore range values
    // because numBytesReturned, above, is sufficient
    final String CONTENT_RANGE = "Content-Range: bytes ";
    String contentRangeLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor read response 2: Content-Range: '" + contentRangeLine + "'");
    if (!contentRangeLine.startsWith(CONTENT_RANGE)) {
      throw new IOException("Invalid content range line: '" + contentRangeLine + "'");
    }

    // X-Range-Available: used to derive the total path size for this path
    final String X_RANGE_AVAILABLE = "X-Range-Available: bytes 0-";
    String xRangeAvailableLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor read response 3: X-Range-Available: '" + xRangeAvailableLine + "'");
    if (!xRangeAvailableLine.startsWith(X_RANGE_AVAILABLE)) {
      throw new IOException("Invalid X-Range-Available line: '" + xRangeAvailableLine + "'");
    }
    // get the end byte string
    String pathEndByteString = xRangeAvailableLine.substring(X_RANGE_AVAILABLE.length());
    int pathEndByte;
    try {
      pathEndByte = Integer.valueOf(pathEndByteString).intValue();
      // the total path size is the range end value plus one
      pathSize = startAddress + pathEndByte + 1;
    } catch (NumberFormatException e) {
      pathSize = 0;
      throw new IOException("Invalid numeric format in path range line: '" + contentLengthLine + "'");
    }

    // blank line
    String blankLine = readLine();
    WLog.log("BulkExtractorFileReader bulk_extractor read response 4: blank line: '" + blankLine + "'");
    if (!blankLine.equals("")) {
      throw new IOException("Invalid blank line: '" + blankLine + "'");
    }

    // read the indicated number of bytes
    bytes = readBytes(numBytesReturned);

    // make sure stdin is now clear
    failIfMoreBytesAvailable();

    // return the byte array
    return bytes;
  }

  // fail if more bytes are available
  // this is not a guaranteed indicator of readiness of the bulk_extractor thread
  // because the bulk_extractor thread can issue multiple writes and flushes
  private void failIfMoreBytesAvailable() throws IOException {
    int leftoverBytes = readFromProcess.available();
    if (leftoverBytes != 0) {

      // clean up
      byte[] extraBytes = readBytes(leftoverBytes);

      // fail
      WLog.logBytes("Unexpected extra bytes in bulk_extractor read response", extraBytes);
      String extraBytesString = new String(extraBytes);
      throw new IOException("Unexpected bulk_extractor read response: " + leftoverBytes
                          + " extra bytes: '" + extraBytesString + "', please see log for details");
    }
  }

  /**
   * Returns data associated with the currently selected path.
   * @param featureLine the feature line identifying the path for which metadata is to be extracted
   */
  public String getBulkExtractorPathMetadata(FeatureLine featureLine) {
    return "File: " + file.toString() + " Path: " + featureLine.getFirstField();
  }

  /**
   * Closes the reader, releasing resources.
   */
  public void close() throws IOException {
    int status = 0;

    // attempt graceful closure
    writeToProcess.println();	// this results in Method not implemented error and termination
    writeToProcess.flush();
    try {
      // start watchdog for this process wait
      ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

      // wait for process to terminate
      // note: this is deemed unexpected enough to pause the Swing thread
      status = process.waitFor();

      // wait for stderr thread to close
      try {
        stderrThread.join();
      } catch (InterruptedException ie) {
        throw new RuntimeException("unexpected event");
      }

      // stop watchdog for this process wait
      aborter.cancel();

    } catch (InterruptedException e) {
      // NOTE: gjc doesn't support IOException(Throwable) so use this:
      Throwable cause = e.getCause();
      IOException ioe = new IOException(cause == null ? null : cause.toString());
      ioe.initCause(cause);
      throw ioe;
    }
    if (status != 0) {
      WLog.log("BulkExtractorFileReader.close: failure closing file " + file + ": " + status);
    }
  }
}


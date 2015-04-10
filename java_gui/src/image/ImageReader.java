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
public class ImageReader {

  public static class ImageReaderResponse {
    public final byte[] bytes;
    public final long totalSizeAtPath;
    ImageReaderResponse(final byte[] bytes, long totalSizeAtPath) {
      this.bytes = bytes;
      this.totalSizeAtPath = totalSizeAtPath;
    }
  }

  // process management
  private static final int DELAY = 60 * 1000; // 12 is good for testing
  private Process process;
  private PrintWriter writeToProcess;
  private InputStream readFromProcess;
  ThreadReaderModel stderrThread;
  private boolean readerIsValid = true;
  String readerFailure = "";

  // input request
  private File file;
  private String forensicPath;
  private long numBytes;

  // cached output
  private byte[] bytes = new byte[0];
  private long imageSize = 0;
  private long totalSizeAtPath = 0;

  /**
   * Open an image reader and attach it to a bulk_extractor process
   * If the reader fails then an error dialog is displayed
   * and an empty reader is returned in its place.
   * @param newFile the file
   */
  public ImageReader(File newFile) {
    file = newFile;

    if (file == null) {
      throw new RuntimeException("imageReader");
    }

    // open interactive http channel with bulk_extractor
    // use "bulk_extractor -p -http <image_file>"
    String[] cmd = new String[4];
    cmd[0] = "bulk_extractor";
    cmd[1] = "-p";
    cmd[2] = "-http";
    cmd[3] = file.getAbsolutePath();
    String[] envp = new String[0];

    WLog.log("BulkExtractorFileReader starting bulk_extractor process");
    WLog.log("BulkExtractorFileReader cmd: " + cmd[0] + " " + cmd[1] + " " + cmd[2] + " " + cmd[3]);

    try {
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
    } catch (IOException e) {
      readerIsValid = false;
      WError.showErrorLater("Unable to start the bulk_extractor reader.",
                            "Error reading Image", e);
    }
  }

  /**
   * indicates whether the reader is valid
   * @return true unless state has become invalid
   */
  public boolean isValid() {
    if (!readerIsValid) {
      return false;
    }

    // also see if the reader's bulk_extractor process is still alive
    // Unfortunately, the way to do this is unexpected:
    // poll exitValue, which traps when alive.
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

  public ImageReaderResponse read(String forensicPath, long numBytes) {
    if (!readerIsValid) {
      throw new RuntimeException("bad state");
    }

    // wrap doRead because it throws exceptions
    try {
      return doRead(forensicPath, numBytes);
    } catch (Exception e) {
      readerIsValid = false;
      WError.showErrorLater("Unable to read image data.",
                            "Error reading Image", e);
      return new ImageReaderResponse(new byte[0], 0);
    }
  }

  private ImageReaderResponse doRead(String forensicPath, long numBytes)
              throws Exception {
    // define the range stop value
    long rangeStopValue = ((numBytes > 0) ? numBytes - 1 : 0);

    // issue the read request
    String getString = "GET " + forensicPath + " HTTP/1.1";
    String rangeString = "Range: bytes=0-" + rangeStopValue;
    WLog.log("ImageReader GET request 1: " + getString);
    WLog.log("ImageReader Range request 2: " + rangeString);
    writeToProcess.println(getString);
    writeToProcess.println(rangeString);
    writeToProcess.println();
    writeToProcess.flush();

    // read the response
    String contentLengthLine = readLine();
    WLog.log("ImageReader length response 1: '" + contentLengthLine + "'");
    String contentRangeLine = readLine();
    WLog.log("ImageReader range response 2: '" + contentRangeLine + "'");
    String xRangeAvailableLine = readLine();
    WLog.log("ImageReader available response 3: '" + xRangeAvailableLine + "'");
    String blankLine = readLine();
    WLog.log("ImageReader blank line response 4: '" + blankLine + "'");

    // Content-Length: provides the number of bytes returned
    int numBytesReturned;
    final String CONTENT_LENGTH = "Content-Length: ";
    WLog.log("BulkExtractorFileReader bulk_extractor read response 1: Content-Length: '" + contentLengthLine + "'");
    if (!contentLengthLine.startsWith(CONTENT_LENGTH)) {
      WLog.log("Invalid content length line: '" + contentLengthLine + "'");
      readerIsValid = false;
    }
    String contentLengthString = contentLengthLine.substring(CONTENT_LENGTH.length());
    try {
      // parse out bytes returned from the line read
      numBytesReturned = Integer.valueOf(contentLengthString).intValue();
    } catch (NumberFormatException e) {
      WLog.log("Invalid numeric format in content length line: '" + contentLengthLine + "'");
      throw e;
    }
    // note if numBytesReturned is not numBytes requested, which is expected near EOF
    if (numBytesReturned != numBytes) {
      WLog.log("BulkExtractorFileReader: note: bytes requested: " + numBytes
                                                + ", bytes read: " + numBytesReturned);
    }

    // Content-Range: check for presence of the Content-Range header, but ignore range values
    // because numBytesReturned, above, is sufficient
    final String CONTENT_RANGE = "Content-Range: bytes ";
    if (!contentRangeLine.startsWith(CONTENT_RANGE)) {
      WLog.log("Invalid content range line: '" + contentRangeLine + "'");
      readerIsValid = false;
    }

    // X-Range-Available: used to derive the total path size for this path
    final String X_RANGE_AVAILABLE = "X-Range-Available: bytes 0-";
    if (!xRangeAvailableLine.startsWith(X_RANGE_AVAILABLE)) {
      WLog.log("Invalid X-Range-Available line: '" + xRangeAvailableLine + "'");
      readerIsValid = false;
    }
    // get the range end byte
    String rangeEndString = xRangeAvailableLine.substring(X_RANGE_AVAILABLE.length());
    int pathEndByte;
    try {
      pathEndByte = Integer.valueOf(rangeEndString).intValue();
      // the total path size is path offset plus the range end value plus one
      totalSizeAtPath = ForensicPath.getOffset(forensicPath) + pathEndByte + 1;
    } catch (NumberFormatException e) {
      totalSizeAtPath = 0;
      WLog.log("Invalid numeric format in path range line: '" + contentLengthLine + "'");
      readerIsValid = false;
      throw e;
    }

    // blank line
    if (!blankLine.equals("")) {
      WLog.log("Invalid blank line: '" + blankLine + "'");
      readerIsValid = false;
    }

    // read the indicated number of bytes
    bytes = readBytes(numBytesReturned);

    // make sure stdin is now clear
    // this is not a guaranteed indicator of readiness of the bulk_extractor thread
    // because the bulk_extractor thread can issue multiple writes and flushes
    int leftoverBytes = readFromProcess.available();
    if (leftoverBytes != 0) {

      // clean up
      byte[] extraBytes = readBytes(leftoverBytes);

      // fail
      WLog.logBytes("Unexpected extra bytes in bulk_extractor read response", extraBytes);
      String extraBytesString = new String(extraBytes);
      WLog.log("Unexpected bulk_extractor read response: " + leftoverBytes
                          + " extra bytes: '" + extraBytesString + "'");
      readerIsValid = false;
    }

    // compose the image reader response
    if (readerIsValid) {
      ImageReaderResponse imageReaderResponse = new ImageReaderResponse(
                                                bytes, totalSizeAtPath);
      return imageReaderResponse;
    } else {
      return new ImageReaderResponse(new byte[0], 0);
    }
  }

  // read line terminated by \r\n
  private String readLine() throws IOException {
    int bInt;
    ByteArrayOutputStream outStream = new ByteArrayOutputStream();

    // start watchdog for this read
    ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

    // read bytes through required http \r\n line terminator
    while (true) {
      bInt = readFromProcess.read();
      if (bInt == -1) {
        WLog.log("bulk_extractor stream terminated in readLine: " + bInt);
        readerIsValid = false;
        break;
      }
      if (bInt != '\r') {
        // add byte to line
        outStream.write(bInt);

      } else {
        // remove the \n following the \r
        bInt = readFromProcess.read();

        // verify \n
        if (bInt != '\n') {
          WLog.log("Invalid line terminator returned from bulk_extractor: " + bInt);
          readerIsValid = false;
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
      int bInt = readFromProcess.read();
      if (bInt > 256) {
        WLog.log("Invalid byte returned from bulk_extractor: " + bInt);
        readerIsValid = false;
        break;
      } else if (bInt < 0) {
        // the stream has terminated
        WLog.log("bulk_extractor stream terminated by byte: " + bInt);
        readerIsValid = false;
        break;
      } else {
        bytes[i] = (byte)bInt;
      }
    }

    // stop watchdog
    aborter.cancel();

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
      WLog.log("Unexpected bulk_extractor read response: " + leftoverBytes
                          + " extra bytes: '" + extraBytesString + "', please see log for details");
    }
  }

  /**
   * Closes the reader, releasing resources.
   */
  public void close() {
    int status = 0;

    // attempt not-so-graceful closure
    // this results in bulk_extractor http Method not implemented error
    // and termination
    try {
    writeToProcess.println();
    writeToProcess.flush();
      // start watchdog for this process wait
      ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

      // wait for process to terminate
      // note: we don't expect trouble here so we do this on the Swing thread
      status = process.waitFor();
      WLog.log("ImageReader closed for file " + file.getAbsolutePath());

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
      WLog.logThrowable(e);
    }
    if (status != 0) {
      WLog.log("BulkExtractorFileReader.close: failure closing file " + file + ": " + status);
    }
  }
}


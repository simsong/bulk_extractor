import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.lang.Runtime;
import java.lang.Process;
import java.util.Observer;
import java.util.Observable;

/**
 * The <code>LibewfcsTesterFileReader</code> class provides path reading services
 * using the libewfcstester utility, which is capable of extracting path buffers.
 */
public class LibewfcsTesterFileReader {

  private static final int DELAY = 40 * 1000;
  private File file;
  private Process process;
//  private BufferedInputStream readFromProcess;
  private BufferedReader readFromProcess;
  private PrintWriter writeToProcess;
  private ThreadReaderModel stderrThread;

  // input request
  // NOTE: read operations are cached for optimization:
  //       when a read request matches the last read request, do not re-read.
  private FeatureLine featureLine = null;
  private long startAddress;
  private int numBytes;

  // cached output
  private byte[] bytes = new byte[0];

  /**
   * Creates an image reader
   * If the reader fails then an error dialog is displayed
   * and an empty reader is returned in its place.
   * @param newFile the file
   * @throws IOException if the reader cannot be created
   */
  public LibewfcsTesterFileReader(File newFile) throws IOException {
    file = newFile;

    // start interactive libewfcstester providing image filename and the requisite library path
    String[] cmd = new String[2];	// "libewfcstester <image_file>"
//    cmd[0] = "/usr/local/bin/libewfcstester";
    cmd[0] = "libewfcstester";
    cmd[1] = file.getAbsolutePath();
//    String[] envp = new String[1];	// LD_LIBRARY_PATH
    String[] envp = new String[0];	// LD_LIBRARY_PATH
//    envp[0] = "LD_LIBRARY_PATH=/usr/local/lib";

    WLog.log("LibewfcsTesterFileReader starting libewfcstester process");
    WLog.log("LibewfcsTesterFileReader cmd: " + cmd[0] + " " + cmd[1]);
//    WLog.log("LibewfcsTesterFileReader envp: " + envp[0]);

    process = Runtime.getRuntime().exec(cmd, envp);
    writeToProcess = new PrintWriter(process.getOutputStream());
//    readFromProcess = new BufferedInputStream(process.getInputStream());
    readFromProcess = new BufferedReader(new InputStreamReader(process.getInputStream()));
    BufferedReader errorFromProcess = new BufferedReader(new InputStreamReader(process.getErrorStream()));

    // thread for consuming stderr
    stderrThread = new ThreadReaderModel(errorFromProcess);
    stderrThread.addReaderModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        String line = (String)arg;
        WLog.log("LibewfcsTesterFileReader error from process " + process + ": " + line);
      }
    });
  }

  /**
   * indicates whether libewfcstester process is alive.
   * @return true if libewfcstester is alive, false if not
   */
  public boolean isAlive() {
    // see if the reader's libewfcstester process is still alive
    // unfortunately, the way to do this is to poll exitValue, which traps when alive.
    try {
      int status = process.exitValue();
      // this is bad
      WLog.log("LibewfcsTesterFileReader process terminated, status: " + status);
      return false;
    } catch (IllegalThreadStateException e) {
      // this is good
    }
    return true;
  }

  /**
   * Returns the size in bytes of the buffer for the expected FeatureLine
   * @param featureLine the feature line for which the path size is to be obtained
   * @return the size in bytes of the buffer at the path extracted
   */
  public long getLibewfcsTesterPathSize(FeatureLine featureLine) throws IOException {
    // the path size depends on the feature line type
    if (featureLine.getType() == FeatureLine.FeatureLineType.ADDRESS_LINE) {
      // for ADDRESS type return the image size
      return readImageSize();
    } else if (featureLine.getType() == FeatureLine.FeatureLineType.PATH_LINE) {
      return 0;
    } else {
      throw new RuntimeException("Invalid feature line type: " + featureLine.getType().toString());
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
  public byte[] readLibewfcsTesterPathBytes(FeatureLine featureLine, long startAddress, int numBytes) throws IOException {

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
    failIfMoreLinesAvailable();

    // issue the read request
    String readRequest = Long.toString(startAddress) + "\n" + Integer.toString(numBytes) + "\n";
    WLog.log("LibewfcsTesterFileReader requesting path bytes");
    sendRequestToProcess(Long.toString(startAddress));
    sendRequestToProcess(Integer.toString(numBytes));

    // read actual numbytes returned
    int numBytesReturned;
    String contentLengthString = readLine();
    try {
      // parse out bytes returned from the line read
      numBytesReturned = Integer.valueOf(contentLengthString).intValue();
    } catch (NumberFormatException e) {
      throw new IOException("Invalid numeric format in content length: " + contentLengthString);
    }
    // note if numBytesReturned is not numBytes requested, which is expected near EOF
    if (numBytesReturned != numBytes) {
      WLog.log("LibewfcsTesterFileReader: note: bytes requested: " + numBytes
                                                + ", bytes read: " + numBytesReturned);
    }

    // read the indicated number of bytes
    bytes = readBytes(numBytesReturned);

    // make sure stdin is now clear
    failIfMoreLinesAvailable();

    // return the byte array
    return bytes;
  }

  // fail if more lines are available
  // this is not a guaranteed indicator of readiness of the libewfcstester thread
  // because the libewfcstester thread can issue multiple writes and flushes
  private void failIfMoreLinesAvailable() throws IOException {
    if (readFromProcess.ready()) {
      // clean up
      while (readFromProcess.ready()) {
        String line = readFromProcess.readLine();
        WLog.log("LibewfcsTesterFileReader.failIfMoreLinesAvailable: line: " + line);
      }

      // fail
      throw new IOException("Unexpected libewfcstester read response: extra lines.");
    }
  }

  /**
   * Returns data associated with the currently selected path.
   * @param featureLine the feature line identifying the path for which metadata is to be extracted
   */
  public String getLibewfcsTesterPathMetadata(FeatureLine featureLine) throws IOException {

    // issue the get metadata request
    WLog.log("LibewfcsTesterFileReader requesting path metadata");
    sendRequestToProcess("properties");

    // read response
    StringBuffer metadataBuffer = new StringBuffer();
    metadataBuffer.append(readLine() + "\n");  // force get started
    while (readFromProcess.ready()) {
      metadataBuffer.append(readLine() + "\n");
    }

    // return response
    return metadataBuffer.toString();
  }

  // send request + newline then flush
  private void sendRequestToProcess(String request) {
    WLog.log("LibewfcsTesterFileReader.writeRequestToProcess: '" + request + "'");
    writeToProcess.println(request);
    writeToProcess.flush();
  }

  /**
   * Closes the reader, releasing resources.
   */
  public void close() throws IOException {
    WLog.log("LibewfcsTesterFileReader.close");
    int status = 0;
    sendRequestToProcess("");
    try {
      // start watchdog for this process wait
      ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

      // wait for process to terminte
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
      WLog.log("LibewfcsTesterFileReader.close: unable to close file " + file
               + " because process terminated, status: " + status);
    }
  }

  // read the total image size
  private long readImageSize() throws IOException {
    long parsedImageSize;

    // issue the get size request
    WLog.log("LibewfcsTesterFileReader requesting size");
    sendRequestToProcess("size");

    // read response
    String imageSizeLine = readLine();
    WLog.log("LibewfcsTesterFileReader libewfcstester size response: '" + imageSizeLine + "'");
    try {
      // parse out image size from the line read
      parsedImageSize = Long.valueOf(imageSizeLine).longValue();
    } catch (NumberFormatException e) {
      throw new IOException("Invalid numeric format in image size line: " + imageSizeLine);
    }

    // make sure stdin is now clear
    failIfMoreLinesAvailable();

    return parsedImageSize;
  }

  // read line, allowing timeout
  private String readLine() throws IOException {

    // start watchdog for this read
    ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

    String line = readFromProcess.readLine();

    // stop watchdog for this read
    aborter.cancel();

    if (line == null) {
      throw new IOException("readLine terminated because process terminated.");
    }

    return line;
  }

  private byte[] readBytes(int numBytes) throws IOException {
    byte[] bytes = new byte[numBytes];

    // start watchdog for this read
    ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

    // read the lines
    for (int i=0; i<numBytes; i++) {
      String line = readFromProcess.readLine();
      if (line == null) {
        throw new IOException("readBytes terminated because process terminated.");
      }
      try {
        // parse byte
        bytes[i] = (byte)Integer.parseInt(line);
      } catch (NumberFormatException e) {
        throw new IOException("Invalid numeric format in image line: '" + line + "'");
      }
    }

    // stop watchdog for this read
    aborter.cancel();

    return bytes;
  }
}


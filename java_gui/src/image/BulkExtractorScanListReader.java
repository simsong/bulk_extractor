import java.util.Vector;
import java.util.Observer;
import java.util.Observable;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.lang.Runtime;
import java.lang.Process;

/**
 * The <code>BulkExtractorScanListReader</code> class returns an array of scanners
 * indicating their name and whether they are enabled by default.
 */
public class BulkExtractorScanListReader {

  private BulkExtractorScanListReader() {
  }

  /**
   * This class contains a scanner name and whether it is enabled by default.
   */
  public static class Scanner {
    final String command;
    final boolean defaultUseScanner;
    private Scanner(String command, boolean defaultUseScanner) {
      this.command = command;
      this.defaultUseScanner = defaultUseScanner;
    }
  }

  /**
   * Read and return the scan list.
   */
  public static Vector<Scanner> readScanList() throws IOException {

    // start the scan list reader process
    // cmd
    String[] cmd = new String[2];
    cmd[0] = "bulk_extractor";
    cmd[1] = "-h";

    // envp
    String[] envp = new String[0];	// LD_LIBRARY_PATH
//    envp[0] = "LD_LIBRARY_PATH=/usr/local/lib";

    // run exec
//    Process process = Runtime.getRuntime().exec(cmd, envp);
    Process process = Runtime.getRuntime().exec(cmd);
    BufferedReader readFromProcess = new BufferedReader(new InputStreamReader(process.getInputStream()));
    BufferedReader errorFromProcess = new BufferedReader(new InputStreamReader(process.getErrorStream()));
 
/* USE IN NEXT bulk_extractor
    // consume stderr using a separate Thread
    ThreadReaderModel stderrThread = new ThreadReaderModel(errorFromProcess);
    stderrThread.addReaderModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        String line = (String)arg;
        WLog.log("BulkExtractorScanListReader stderr: '" + line + "'");
      }
    });
*/

    // start the abort timer
    final int DELAY = 5000; // ms.  This should never happen, but if it does, it prevents a hang.
    ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

    // read the scanners the old way from stderr
    ScanListReaderThread stderrThread = new ScanListReaderThread(errorFromProcess);
    stderrThread.start();

    // read the scanners the new way from stdout
    ScanListReaderThread stdoutThread = new ScanListReaderThread(readFromProcess);
    stdoutThread.start();

    // wait for the bulk_extractor process to terminate
    try {
      process.waitFor();
    } catch (InterruptedException ie1) {
      WLog.log("BulkExtractorVersionReader process interrupted");
    }

    // wait for stderr Thread to finish
    try {
      stderrThread.join();
    } catch (InterruptedException ie2) {
      throw new RuntimeException("unexpected event");
    }

    // wait for stdout Thread to finish
    try {
      stdoutThread.join();
    } catch (InterruptedException ie3) {
      throw new RuntimeException("unexpected event");
    }

    // scanners to be returned
    Vector<Scanner> scanners;

    // set scanners based on which thread obtained them
    if (stderrThread.scanners.size() > 0) {
      scanners = stderrThread.scanners;
WLog.log("BulkExtractorScanListReader.readScanList Number of scanners (stderr for v1.2): " + stderrThread.scanners.size());
    } else if (stdoutThread.scanners.size() > 0) {
      scanners = stdoutThread.scanners;
WLog.log("BulkExtractorScanListReader.readScanList Number of scanners (stdout for v1.3): " + stdoutThread.scanners.size());
    } else
      scanners = new Vector<Scanner>();

    // cancel the aborter timer
    aborter.cancel();

    // return the list
    return scanners;
  }

  private static final class ScanListReaderThread extends Thread {
    private final BufferedReader bufferedReader;
    public Vector<Scanner> scanners = new Vector<Scanner>();
    public ScanListReaderThread(BufferedReader bufferedReader) {
      this.bufferedReader = bufferedReader;
    }
    public void run() {
      while(true) {

        try {
          // blocking wait until EOF
          String input = bufferedReader.readLine();
          if (input == null) {
            break;
          }

          // break input into tokens
          String[] tokens = input.split("\\s+");

          // parse tokens for scanner names, recognizing if they are enabled or disabled by default
          if (tokens.length >= 3) {
            if (tokens[0].equals("") && tokens[1].equals("-e")) {  // disabled by default
              scanners.add(new Scanner(tokens[2], false));
            } else if (tokens[0].equals("") && tokens[1].equals("-x")) {  // enabled by default
              scanners.add(new Scanner(tokens[2], true));
            } else {
              // this input line does not define a scanner, so ignore it.
            }
          }
        } catch (IOException e) {
          WLog.log("BulkExtractorScanListReader.ScanListReaderThread " + this + " aborting.");
          break;
        }
      }
    }
  }
}


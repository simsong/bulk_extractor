import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.Runtime;
import java.lang.Process;
import java.util.Observer;
import java.util.Observable;

/**
 * The <code>BulkExtractorVersionChecker</code> class verifies the version of bulk_extractor
 * against the expected value.  The version must be in the format "#.#.#".
 */
public class BulkExtractorVersionReader {

  /**
   * Displays the bulk_extractor and BEViewer versions
   */
  public static void showVersions() {
    Thread versionReaderThread = new VersionReaderThread();
    versionReaderThread.start();
  }

  private static class VersionReaderThread extends Thread {
    private BufferedReader readFromProcess;	// to consume stderr
    ThreadReaderModel stderrThread;

    // thread entry point
    public void run() {
      try {
        readVersion();
      } catch (IOException e) {
        WError.showError("Error in reading bulk_extractor version."
                       + "\nBulk_extractor features may not be available during this session.",
                         "bulk_extractor failure", e);
      }
    }

    private void readVersion() throws IOException {

      final int DELAY = 10 * 1000;
      Process process;

      // cmd
      String[] cmd = new String[2];
      cmd[0] = "bulk_extractor";
      cmd[1] = "-V";

      // envp
      String[] envp = new String[0];	// LD_LIBRARY_PATH
//      envp[0] = "LD_LIBRARY_PATH=/usr/local/lib";

      process = Runtime.getRuntime().exec(cmd, envp);
//      process = Runtime.getRuntime().exec(cmd);
      readFromProcess = new BufferedReader(new InputStreamReader(process.getInputStream()));
      BufferedReader errorFromProcess = new BufferedReader(new InputStreamReader(process.getErrorStream()));
      stderrThread = new ThreadReaderModel(errorFromProcess);
      stderrThread.addReaderModelChangedListener(new Observer() {
        public void update(Observable o, Object arg) {
          String line = (String)arg;
          WLog.log("BulkExtractorVersionReader stderr: '" + line + "'");
        }
      });

      // start aborter timer
      ThreadAborterTimer aborter = new ThreadAborterTimer(process, DELAY);

      // read version
      final String VERSION_PREFIX = "bulk_extractor ";
      String input = readFromProcess.readLine();
      if ((input == null) || !input.startsWith(VERSION_PREFIX)) {
        // invalid input in read
        WError.showErrorLater("Error in reading bulk_extractor version: '" + input + "'",
                         "bulk_extractor failure", null);
      } else {

        // isolate the bulk_extractor version
        String bulk_extractorVersion = input.substring(VERSION_PREFIX.length());
        bulk_extractorVersion = bulk_extractorVersion.trim(); // bulk_extractor appends an unnecessary space

        WLog.log("BulkExtractorVersionReader.readVersion: bulk_extractor version " + bulk_extractorVersion + ", Bulk Extractor Viewer version " + Config.VERSION);
        // show both versions
        WError.showMessageLater("Versions:\nBulk Extractor Viewer: " + Config.VERSION
                      + "\nbulk_extractor: " + bulk_extractorVersion,
                      "Version Information");
//        WError.showMessageLater("\u2022 Bulk Extractor Viewer Version " + Config.VERSION
//                      + "\n\u2022 bulk_extractor Version " + bulk_extractorVersion,
//                      "Version Information");
      }

      // wait for the bulk_extractor process to terminate
      try {
        process.waitFor();
      } catch (InterruptedException ie) {
        WLog.log("BulkExtractorVersionReader process interrupted");
      }

      // stream off any unexpected stdout
      try {
        while ((input = readFromProcess.readLine()) != null) {
          WLog.log("BulkExtractorVersionReader stdout: '" + input + "'");
        }
      } catch (IOException e1) {
        // let it go
      }

      // wait for stderr Thread to finish
      try {
        stderrThread.join();
      } catch (InterruptedException ie2) {
        throw new RuntimeException("unexpected event");
      }

      // stop the aborter timer
      aborter.cancel();
    }
  }
}


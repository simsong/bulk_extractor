//import java.util.Vector;
//import java.util.Iterator;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.concurrent.locks.LockSupport;
import javax.swing.event.ListDataListener;
import javax.swing.event.ListDataEvent;

/**
 * The <code>ScanSettingsConsumer</code> consumes Scan Settings jobs
 * one at a time as they become available in the scan settings run queue.
 *
 * The consumer waits for the producer's semaphore for jobs.
 */
class ScanSettingsConsumer extends Thread {
  private static Process process;

  /**
   * Just loading the constructor starts the conumer.
   */
  public ScanSettingsConsumer() {
    start();
  }

  /**
   * Kill the bulk_extractor process.
   */
  public static void killBulkExtractorProcess() {
    if (process != null) {
      process.destroy();
    }
  }

  // thread to forward stdout to WScanProgress
  private static class ThreadStdout extends Thread {
    private final WScanProgress wScanProgress;
    private final BufferedReader bufferedReader;

    ThreadStdout(WScanProgress wScanProgress, BufferedReader bufferedReader) {
      this.wScanProgress = wScanProgress;
      this.bufferedReader = bufferedReader;
      start();
    }

    public void run() {
      while (true) {
        try {
          // block wait until EOF
          String input = bufferedReader.readLine();
          if (input == null) {
            break;
          } else {
            wScanProgress.showStdout(input);
          }
        } catch (IOException e) {
          WLog.log("ScanSettingsConsumer.ThreadStdout.run aborting.");
          break;
        }
      }
    }
  }

  // thread to forward stderr to WScanProgress
  private static class ThreadStderr extends Thread {
    private final WScanProgress wScanProgress;
    private final BufferedReader bufferedReader;

    ThreadStderr(WScanProgress wScanProgress, BufferedReader bufferedReader) {
      this.wScanProgress = wScanProgress;
      this.bufferedReader = bufferedReader;
      start();
    }

    public void run() {
      while (true) {
        try {
          // block wait until EOF
          String input = bufferedReader.readLine();
          if (input == null) {
            break;
          } else {
            wScanProgress.showStderr(input);
          }
        } catch (IOException e) {
          WLog.log("ScanSettingsConsumer.ThreadStderr.run aborting.");
          break;
        }
      }
    }
  }

  // this is where the waiting happens
  private ScanSettings getJob() {
    ScanSettings scanSettings;
    while (true) {
      scanSettings = BEViewer.scanSettingsListModel.remove();
      if (scanSettings != null) {
        break;
      } else {
        // wait for signal that a job may be available
        LockSupport.park();
      }
    }
    return scanSettings;
  }

  // this responds to wakeup events
  private void unpark() {
    LockSupport.unpark(this);
  }
 
  // this runs forever, once through per semaphore permit acquired
  public void run() {

    // register self to listen to job added events
    BEViewer.scanSettingsListModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        // not used;
      }
      public void intervalAdded(ListDataEvent e) {
        // consume the item by waking up self
        unpark();
      }
      public void intervalRemoved(ListDataEvent e) {
        // not used;
      }
    });

    while (true) {
      // get a run job
      ScanSettings scanSettings = getJob();

      // log the scan command
      WLog.log("ScanSettingsConsumer.command: '" + scanSettings.getCommandString() + "'");

      // start bulk_extractor process
      try {
        // NOTE: It would be nice to use commandString instead, but Runtime
        // internally uses array and its string tokenizer doesn't manage
        // quotes or spaces properly.
        process = Runtime.getRuntime().exec(scanSettings.getCommandArray());

      } catch (IOException e) {
        // alert and abort
        WError.showError("bulk_extractor Scanner failed to start command\n'"
                         + scanSettings.getCommandString() + "'",
                         "bulk_extractor failure", e);
        // something went wrong starting bulk_extractor
        continue;
      }

      // get a new instance of WScanProgress for tracking the bulk_extractor
      // process
      WScanProgress wScanProgress = WScanProgress.getWScanProgress();

      // show startup information
      wScanProgress.showStart(scanSettings);

      // start stdout reader
      BufferedReader readFromStdout = new BufferedReader(
                             new InputStreamReader(process.getInputStream()));

      ThreadStdout threadStdout = new ThreadStdout(wScanProgress, readFromStdout);

      // start stderr reader
      BufferedReader readFromStderr = new BufferedReader(
                             new InputStreamReader(process.getInputStream()));

      ThreadStderr threadStderr = new ThreadStderr(wScanProgress, readFromStderr);

      // wait for the thread readers to finish
      try {
        threadStdout.join();
      } catch (InterruptedException ie1) {
        throw new RuntimeException("unexpected event");
      }
      try {
        threadStderr.join();
      } catch (InterruptedException ie2) {
        throw new RuntimeException("unexpected event");
      }

      // wait for the bulk_extractor scan process to finish
      // Note: this isn't really necessary since being finished is implied
      // when the Thread readers finish.
      // Note: the process terminates by itself or by process.destroy().
      try {
        process.waitFor();
      } catch (InterruptedException ie) {
        throw new RuntimeException("unexpected event");
//        WLog.log("WScanProgress ScannerThread interrupted");
      }

      // get the process' exit value
      int exitValue = process.exitValue();

      // set the final "done" state
      wScanProgress.showDone(scanSettings, exitValue);
    }
  }
}


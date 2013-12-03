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
 * The consumer loops, consuming jobs, until it parks.
 * The provider provides unpark permits after enqueueing jobs.
 * This policy keeps the consumer going.
 *
 * This object must be initialized after ScanSettingsListModel.
 */
class ScanSettingsConsumer extends Thread {
  private static ScanSettingsConsumer scanSettingsConsumer;
  private static boolean isPaused = false;
  private static Process process;

  /**
   * Just loading the constructor starts the conumer.
   */
  public ScanSettingsConsumer() {
    scanSettingsConsumer = this;
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

/*
  // issue a permit to unpark, waking this thread up or keeping it awake
  private void unpark() {
    LockSupport.unpark(this);
  }
*/
 
  /**
   * Pause the consumer so that it does not start another buk_extractor run
   * or restart the consumer.
   */
  public synchronized static void pauseConsumer(boolean doPause) {
    if (doPause) {
      isPaused = true;
    } else {
      isPaused = false;
      LockSupport.unpark(ScanSettingsConsumer.scanSettingsConsumer);
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

  // this runs forever, once through per semaphore permit acquired
  public void run() {

    // register self to listen for added jobs
    BEViewer.scanSettingsListModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        // not used;
      }
      public void intervalAdded(ListDataEvent e) {
        // consume the item by waking up self
        LockSupport.unpark(ScanSettingsConsumer.scanSettingsConsumer);
      }
      public void intervalRemoved(ListDataEvent e) {
        // not used;
      }
    });

    while (true) {
      // wait for signal that a job may be available
      LockSupport.park();

      if (isPaused) {
        // to pause, simply restart at top of loop.
        // Use pauseConsumer() to restart.
        continue;
      }

      // get a ScanSettings run job
      ScanSettings scanSettings = BEViewer.scanSettingsListModel.remove();
      if (scanSettings == null) {
        // the producer is not ready, so restart at top of loop.
        continue;
      }

      // log the scan command that is about to be run
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
                             new InputStreamReader(process.getErrorStream()));

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

      // tell thread try another run if one is available
      LockSupport.unpark(ScanSettingsConsumer.scanSettingsConsumer);
    }
  }
}


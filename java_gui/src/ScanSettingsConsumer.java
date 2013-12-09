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

  /**
   * Just loading the constructor starts the conumer.
   */
  public ScanSettingsConsumer() {
    scanSettingsConsumer = this;
    start();
  }

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

      // get a ScanSettings run job from the producer
      ScanSettings scanSettings = BEViewer.scanSettingsListModel.remove();
      if (scanSettings == null) {
        // the producer is not ready, so restart at top of loop.
        continue;
      }

      // log the scan command of the job that is about to be run
      WLog.log("ScanSettingsConsumer starting bulk_extractor run: '"
               + scanSettings.getCommandString() + "'");

      // start the bulk_extractor process
      Process process;
      try {
        // NOTE: It would be nice to use commandString instead, but Runtime
        // internally uses array and its string tokenizer doesn't manage
        // quotes or spaces properly, so we use array.
        process = Runtime.getRuntime().exec(scanSettings.getCommandArray());

      } catch (IOException e) {
        // something went wrong starting bulk_extractor so alert and abort
        WError.showErrorLater("bulk_extractor Scanner failed to start command\n'"
                         + scanSettings.getCommandString() + "'",
                         "bulk_extractor failure", e);

        // despite the failure to start, continue to consume the queue
        LockSupport.unpark(ScanSettingsConsumer.scanSettingsConsumer);
        continue;
      }

      // open a dedicated instance of WScanProgress for showing progress and
      // status of the bulk_extractor process
      WScanProgress.openWindow(scanSettings, process);

      // wait for the bulk_extractor scan process to finish
      // The process terminates by itself or by calling process.destroy().
      try {
        process.waitFor();
      } catch (InterruptedException ie) {
        throw new RuntimeException("unexpected event");
      }

      // unpark to try another run if the producer has one available
      LockSupport.unpark(ScanSettingsConsumer.scanSettingsConsumer);
    }
  }
}


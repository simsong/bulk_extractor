zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
import java.util.Vector;
import java.util.Iterator;
import java.util.concurrent.Semaphore;
import javax.swing.DefaultListModel;

/**
 * The <code>ScanSettingsRunQueue</code> class manages the bulk_extractor
 * run queue.
 *
 * It offers a model changed listener for other modules
 * and a java.swing.DefaultListModel for use with JList.
 *
 * Locked interfaces add, remove, swap, and modify queued ScanSettings objects.
 */

public class ScanSettingsRunQueue {

/*
zz disable consumer while testing
  // start the scan settings consumer so that it can consume directly
  // from this queue
  private static final ScanSettingsConsumer scanSettingsConsumer
                                                = new ScanSettingsConsumer();
*/

  // the jobs queue is kept in the DefaultListModel
//  private static DefaultListModel<ScanSettings> jobs // sorry, needs Java7
//                              = new defaultListModel<ScanSettings>();
  private static DefaultListModel jobs = new DefaultListModel();

  // use this as a singleton resource, not as a class
  private ScanSettingsRunQueue() {
  }

  /**
   * Add ScanSettings to tail (bottom) of LIFO job queue
   * and increment the sempahore.
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public static synchronized void add(ScanSettings scanSettings) {
WLog.log("ScanSettingsRunQueue.add " + scanSettings.getCommandString());
    jobs.addElement(scanSettings);
    scanSettingsSemaphore.release();
  }

  /**
   * Remove and return ScanSettings from head (top) of LIFO job queue
   * and decrement the sempahore.
   */
  public synchronized static boolean remove() {
    if (jobs.size() >= 1) {
      return remove((ScanSettings)jobs.getElementAt(0));
    } else {
      return remove(new ScanSettings());
    }
  }

  /**
   * Remove specified ScanSettings object from within the job queue
   * and decrement the sempahore.
   */
  public synchronized static boolean remove(ScanSettings scanSettings) {
    if (jobs.contains(scanSettings)) {
      // good, it is available to be removed

      // dequeue it
// NOTE: InterruptedException not thrown until Java 7
//      try {
        boolean isAcquired = scanSettingsSemaphore.tryAcquire();
        if (!isAcquired) {
          WLog.log("ScanSettingsRunQueue.remove: failure to reacquire semaphore.");
        }
//      } catch (InterruptedException e) {
//        throw new RuntimeException("interrupted thread");
//      }

      // remove it from jobs
      // NOTE: Dequeue before removal because removal fires a changed event.
      boolean success = jobs.removeElement(scanSettings);
      if (success != true) {
        throw new RuntimeException("internal error");
      }
      return true;

    } else {
      // the requested scanSettings object was not present in the job queue
      WLog.log("ScanSettingsRunQueue.remove: no element");
      return false;
    }
  }

  /**
   * Move ScanSettings up toward the top of the queue.
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public synchronized static boolean moveUp(ScanSettings scanSettings) {
    int n = jobs.indexOf(scanSettings);
    if (n < 1) {
      WLog.log("ScanSettingsRunQueue.moveUp: failure at index " + n);
      return false;
    } else {
      ScanSettings scanSettings2 = (ScanSettings)jobs.get(n-1);
      jobs.setElementAt(scanSettings, n-1);
      jobs.setElementAt(scanSettings2, n);
      return true;
    }
  }

  /**
   * Move ScanSettings down toward the bottom of the queue.
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public synchronized static boolean moveDown(ScanSettings scanSettings) {
    int n = jobs.indexOf(scanSettings);
    if (n < 0 || n > jobs.size()-1) {
      WLog.log("ScanSettingsRunQueue.moveDown: failure at index " + n);
      return false;
    } else {
      ScanSettings scanSettings2 = (ScanSettings)jobs.get(n+1);
      jobs.setElementAt(scanSettings, n-1);
      jobs.setElementAt(scanSettings2, n);
      return true;
    }
  }

// zzz No, to edit, dequeue and requeue when done.
// zzz This approach is simple and threadsafe.
//  /**
//   * replace settings with another
//   */
//@SuppressWarnings("unchecked") // hacked until we don't require javac6
//  public synchronized static void replace(ScanSettings oldScanSettings,
//                                          ScanSettings newScanSettings) {
//    int n = jobs.indexOf(oldScanSettings);
//    jobs.setElementAt(newScanSettings, n);
//  }

  /**
   * Number of scan settings enqueued.
   */
  public synchronized static int size() {

    // this is a good place to check internal run state consistency
    if (jobs.size() != scanSettingsSemaphore.availablePermits()) {
      WLog.log("ScanSettingsRunQueue.size internal error, jobs: "
           + jobs.size() + ", semaphore: "
           + scanSettingsSemaphore.availablePermits());
      // fatal if list and semaphore don't match.
      throw new RuntimeException("internal error");
    }

    // return size
    return jobs.size();
  }

  /**
   * Returns the <code>DefaultListModel</code> associated with this model
   * @return the run queue list model
   */
//  public DefaultListModel<FeatureLine> getListModel() 
  public static DefaultListModel getListModel() {
    return jobs;
  }
}


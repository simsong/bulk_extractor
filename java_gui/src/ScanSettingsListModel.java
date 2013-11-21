import java.util.Vector;
import java.util.concurrent.Semaphore;
import javax.swing.AbstractListModel;

/**
 * Provides list capability and also manages the producer semaphore.
 *
 * This should be a static singleton class, but abstract class
 * AbstractListModel requires that it not be static.
 */

public class ScanSettingsListModel extends AbstractListModel {

  // the jobs being managed by this list model
  private final Vector<ScanSettings> jobs = new Vector<ScanSettings>();

  // start the scan settings semaphore with nothing to consume
  private final Semaphore scanSettingsSemaphore = new Semaphore(0);

  // use this list model as a singleton resource, not as a class
  public ScanSettingsListModel() {
  }

  /**
   * Acquire a permit from the semaphore.
   */
  public void acquire() {
    try {
      scanSettingsSemaphore.acquire();
    } catch (InterruptedException e) {
      // something went wrong acquiring the semaphore
      WLog.log("ScanSettingsListModel.acquire failure");
    }
  }

  /**
   * Add ScanSettings to tail (bottom) of LIFO job queue
   * and increment the sempahore.
   */
  public synchronized void add(ScanSettings scanSettings) {
WLog.log("ScanSettingsRunQueue.add " + scanSettings.getCommandString());
    jobs.add(scanSettings);
    scanSettingsSemaphore.release();
    fireIntervalAdded(this, jobs.size()-1, jobs.size()-1);
  }

  /**
   * Remove and return ScanSettings from head (top) of LIFO job queue
   * and decrement the sempahore else return null.
   */
  public synchronized ScanSettings remove() {
    if (jobs.size() >= 1) {
      return remove(jobs.get(0));
    } else {
      return null;
    }
  }

  /**
   * Remove specified ScanSettings object from within the job queue
   * and decrement the sempahore else return null.
   */
  public synchronized ScanSettings remove(ScanSettings scanSettings) {
    int index = jobs.indexOf(scanSettings);
    if (index >= 0) {
      // good, it is available to be removed

      // remove it from jobs
      jobs.remove(index);

      // also dequeue it
// NOTE: InterruptedException not thrown until Java 7
//      try {
        boolean isAcquired = scanSettingsSemaphore.tryAcquire();
        if (!isAcquired) {
          WLog.log("ScanSettingsRunQueue.remove: failure to reacquire semaphore.");
        }
//      } catch (InterruptedException e) {
//        throw new RuntimeException("interrupted thread");
//      }

      // if removed, fire
      fireIntervalRemoved(this, index, index);
      return scanSettings;

    } else {
      // the requested scanSettings object was not present in the job queue
      WLog.log("ScanSettingsRunQueue.remove: no element");
      return null;
    }
  }

  /**
   * Move ScanSettings up toward the top of the queue.
   */
//@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public synchronized boolean moveUp(ScanSettings scanSettings) {
    int n = jobs.indexOf(scanSettings);
    if (n < 1) {
      WLog.log("ScanSettingsRunQueue.moveUp: failure at index " + n);
      return false;
    } else {
      ScanSettings scanSettings2 = jobs.get(n-1);
      jobs.setElementAt(scanSettings, n-1);
      jobs.setElementAt(scanSettings2, n);
      fireContentsChanged(this, n-1, n);
      return true;
    }
  }

  /**
   * Move ScanSettings down toward the bottom of the queue.
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public synchronized boolean moveDown(ScanSettings scanSettings) {
    int n = jobs.indexOf(scanSettings);
    if (n < 0 || n > jobs.size()-1) {
      WLog.log("ScanSettingsRunQueue.moveDown: failure at index " + n);
      return false;
    } else {
      ScanSettings scanSettings2 = jobs.get(n+1);
      jobs.setElementAt(scanSettings, n+1);
      jobs.setElementAt(scanSettings2, n);
      fireContentsChanged(this, n, n+1);
      return true;
    }
  }

  /**
   * Number of scan settings enqueued.
   * Required by interface ListModel.
   */
  public synchronized int getSize() {

    // validate consistency of the job count and the semaphore count
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
   * Element at index.
   * Required by interface ListModel.
   */
  public synchronized ScanSettings getElementAt(int index) {
    return jobs.get(index);
  }
}


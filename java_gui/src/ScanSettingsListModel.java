import java.util.Vector;
import java.util.concurrent.locks.ReentrantLock;
import javax.swing.AbstractListModel;

/**
 * Manages the jobs list and also provides list capability for a JList
 * in a threadsafe way.
 *
 * This should be a static singleton class, but abstract class
 * AbstractListModel requires that it not be static.
 *
 * Consumer: use ListDataListener to listen for intervalAdded event,
 * then try to consume a ScanSettings job object, if available.
 * Consumer must loop to consume queued jobs.
 */

public class ScanSettingsListModel extends AbstractListModel {

  // the jobs being managed by this list model
  private final Vector<ScanSettings> jobs = new Vector<ScanSettings>();

  // a lock is used to prevent corruption of the jobs queue
  private final ReentrantLock lock = new ReentrantLock();

  // AbstractModel requires that this singleton resource be non-static
  public ScanSettingsListModel() {
  }

  /**
   * Add ScanSettings to tail (bottom) of LIFO job queue
   * and increment the sempahore.
   */
  public void add(ScanSettings scanSettings) {
    WLog.log("ScanSettingsListModel.add job '"
             + scanSettings.getCommandString() + "'");
    lock.lock();
    jobs.add(scanSettings);
    lock.unlock();
    fireIntervalAdded(this, jobs.size()-1, jobs.size()-1);
  }

  /**
   * Remove and return ScanSettings from head (top) of LIFO job queue.
   * and decrement the sempahore else return null.
   */
  public ScanSettings remove() {
    lock.lock();
    boolean removed = false;
    ScanSettings scanSettings = null;
    if (jobs.size() >= 1) {
      WLog.log("ScanSettingsListModel.remove top job '"
               + jobs.get(0).getCommandString() + "'");
      scanSettings = jobs.remove(0);
      removed = true;
    } else {
      // there are no jobs to remove
      WLog.log("ScanSettingsRunQueue.remove top: no top element to remove");
    }
    lock.unlock();
    if (removed) {
      fireIntervalRemoved(this, 0, 0);
    }
    return scanSettings;
  }

  /**
   * Remove specified ScanSettings object from within the job queue
   * and decrement the sempahore else return null.
   */
  public ScanSettings remove(ScanSettings scanSettings) {
    lock.lock();
    boolean removed = false;
    int index = jobs.indexOf(scanSettings);
    if (index >= 0) {
      // good, it is available to be removed
      WLog.log("ScanSettingsListModel.remove job '"
               + scanSettings.getCommandString() + "'");

      // remove it from jobs
      jobs.remove(index);
      removed = true;
    } else {
      // the requested job was not there
      WLog.log("ScanSettingsRunQueue.remove scanSettings: no element to remove");
    }
    lock.unlock();
    if (removed) {
      fireIntervalRemoved(this, index, index);
    }
    return scanSettings;
  }

  /**
   * Move ScanSettings up toward the top of the queue.
   */
  public void moveUp(ScanSettings scanSettings) {
    lock.lock();
    boolean moved = false;
    int n = jobs.indexOf(scanSettings);
    if (n > 0) {
      ScanSettings oldUpper = jobs.set(n-1, scanSettings);
      ScanSettings newLower = jobs.set(n, oldUpper);
      moved = true;

      // validate
      if (scanSettings != newLower) {
        throw new RuntimeException("program error");
      }
    } else {
      WLog.log("ScanSettingsRunQueue.moveUp: failure at index " + n);
    }
    lock.unlock();
    if (moved) {
      fireContentsChanged(this, n-1, n);
    }
  }

  /**
   * Move ScanSettings down toward the bottom of the queue.
   */
  public void moveDown(ScanSettings scanSettings) {
    lock.lock();
    boolean moved = false;
    int n = jobs.indexOf(scanSettings);
    if (n != -1 && n < jobs.size() - 1) {
      ScanSettings oldLower = jobs.set(n+1, scanSettings);
      ScanSettings newUpper = jobs.set(n, oldLower);
      moved = true;

      // validate
      if (scanSettings != newUpper) {
        throw new RuntimeException("program error");
      }
    } else {
      WLog.log("ScanSettingsRunQueue.moveDown: failure at index " + n);
    }
    lock.unlock();
    if (moved) {
      fireContentsChanged(this, n, n+1);
    }
  }

  /**
   * Number of scan settings enqueued.
   * Required by interface ListModel.
   */
  public int getSize() {
    lock.lock();
    int size = jobs.size();
    lock.unlock();
    return size;
  }

  /**
   * Element at index.
   * Required by interface ListModel.
   */
  public ScanSettings getElementAt(int index) {
    lock.lock();
    ScanSettings scanSettings = jobs.get(index);
    lock.unlock();
    return scanSettings;
  }
}


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

  // start the scan settings semaphore with nothing to consume
  public static final Semaphore scanSettingsSemaphore = new Semaphore(0);

  // start the scan settings consumer so that it can consume directly
  // from this queue
  private static final ScanSettingsConsumer scanSettingsConsumer
                                                = new ScanSettingsConsumer();

  // the jobs queue is kept in the DefaultListModel
//  private static DefaultListModel<ScanSettings> jobs // sorry, needs Java7
//                              = new defaultListModel<ScanSettings>();
  private static DefaultListModel jobs = new DefaultListModel();

  // use this as a singleton resource, not as a class
  private ScanSettingsRunQueue() {
  }

  /**
   * Add ScanSettings to tail (bottom) of LIFO job queue.
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public static synchronized void add(ScanSettings scanSettings) {
WLog.log("ScanSettingsRunQueue.add " + scanSettings.getCommandString());
    jobs.addElement(scanSettings);
    scanSettingsSemaphore.release();
  }

  /**
   * Remove and return ScanSettings from head (top) of LIFO job queue.
   */
  public synchronized static ScanSettings remove() {
    if (jobs.size() < 1) {
      WLog.log("ScanSettingsRunQueue.remove: no first element");
    }
//    ScanSettings scanSettings = jobs.remove(0);
    ScanSettings scanSettings = (ScanSettings)jobs.remove(0);
    return scanSettings;
  }

  /**
   * Remove specified ScanSettings object from within the job queue.
   */
  public synchronized static void remove(ScanSettings scanSettings) {
//@SuppressWarnings("unchecked") // hacked until we don't require javac6
    boolean success = jobs.removeElement(scanSettings);
    if (success == false) {
      WLog.log("ScanSettingsRunQueue.remove: no element");
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

  /**
   * replace settings with another
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public synchronized static void replace(ScanSettings oldScanSettings,
                                          ScanSettings newScanSettings) {
    int n = jobs.indexOf(oldScanSettings);
    jobs.setElementAt(newScanSettings, n);
  }

  /**
   * Number of scan settings enqueued.
   */
  public synchronized static int size() {
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


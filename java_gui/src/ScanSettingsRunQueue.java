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
      throw new RuntimeException("invalid usage");
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
      throw new RuntimeException("invalid usage");
    }
  }

  /**
   * Swap the order of two elements
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public synchronized static void swap(ScanSettings s1, ScanSettings s2) {
    // java.util.Vector does not have a swap command, so use this
    int n1 = jobs.indexOf(s1);
    int n2 = jobs.indexOf(s2);
    ScanSettings j1 = (ScanSettings)jobs.get(n1);
    ScanSettings j2 = (ScanSettings)jobs.get(n2);
    jobs.setElementAt(j2, n1);
    jobs.setElementAt(j1, n2);

    // note: it is not necessary to fire run queue change for this.
  }

  /**
   * replace settings with another
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public synchronized static void replace(ScanSettings oldScanSettings,
                                          ScanSettings newScanSettings) {
    int n = jobs.indexOf(oldScanSettings);
    jobs.setElementAt(newScanSettings, n);

    // note: it is not necessary to fire run queue change for this.
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
//  public DefaultListModel<FeatureLine> getListModel() {
  public DefaultListModel getListModel() {
    return jobs;
  }
}


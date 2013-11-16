import java.util.Vector;
import java.util.Iterator;

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

  // the jobs queue is kept in the DefaultListModel
//  private static DefaultListModel<ScanSettings> jobs // sorry, needs Java7
//                              = new defaultListModel<ScanSettings>();
  private static DefaultListModel jobs = new defaultListModel();

//  // listeners use the run queue changed notifier
//  private static final ModelChangedNotifier<Object> runQueueChangedNotifier
//                = new ModelChangedNotifier<Object>();

  // use this as a singleton, not as a class
  private ScanSettingsRunQueue() {
  }

  /**
   * Add ScanSettings to tail (bottom) of LIFO job queue.
   */
//@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public static synchronized static void add(ScanSettings scanSettings) {
    jobs.addElement(scanSettings);
//    runQueueChangedNotifier.fireModelChanged(null);
  }

  /**
   * Remove and return ScanSettings from head (top) of LIFO job queue.
   */
  public synchronized static ScanSettings remove() {
    if (jobs.size() < 1) {
      throw new RuntimeException("invalid usage");
    }
    ScanSettings scanSettings = jobs.remove(0);
//    runQueueChangedNotifier.fireModelChanged(null);
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
//    runQueueChangedNotifier.fireModelChanged(null);
  }

  /**
   * Swap the order of two elements
   */
  public synchronized static void swap(ScanSettings s1, ScanSettings s2) {
    // java.util.Vector does not have a swap command, so use this
    int n1 = job.indexOf(s1);
    int n2 = job.indexOf(s2);
    ScanSettings j1 = jobs.get(n1);
    ScanSettings j2 = jobs.get(n2);
    jobs.setElementAt(j2, n1);
    jobs.setElementAt(j1, n2);

    // note: it is not necessary to fire run queue change for this.
  }

  /**
   * replace settings with another
   */
  public synchronized static void replace(ScanSettings oldScanSettings,
                                          ScanSettings newScanSettings) {
    int n = job.indexOf(oldScanSettnigs);
    jobs.setElementAt(newScanSettings, n);

    // note: it is not necessary to fire run queue change for this.
  }

  /**
   * Number of scan settings enqueued.
   */
  public synchronized static void size() {
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

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param bookmarksModelChangedListener the <code>Observer</code> to be added
   */
  public static void addRunQueueModelChangedListener(Observer runQueueModelChangedListener) {
    bookmarksModelChangedNotifier.addObserver(bookmarksModelChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param bookmarksModelChangedListener the <code>Observer</code> to be removed
   */
  public static void removeRunQueueModelChangedListener(Observer runQueueModelChangedListener) {
    runQueueModelChangedNotifier.deleteObserver(runQueueModelChangedListener);
  }

  static class Consumer {
    private static boolean isBusy = false;

    public synchronized static consume() {
      if (!isBusy && ScanSettingsRunQueue.size() > 0) {
        ScanSettings scanSettings = ScanSettingsRunQueue.

}


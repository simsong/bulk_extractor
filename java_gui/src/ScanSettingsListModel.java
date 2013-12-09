import java.util.Vector;
import javax.swing.AbstractListModel;
import javax.swing.SwingUtilities;
import javax.swing.DefaultListSelectionModel;
import javax.swing.ListSelectionModel;

/**
 * Manages the jobs list, provides list capability for a JList,
 * and provides a list selection model suitable for this use case,
 * all in a threadsafe way.
 *
 * This should be a static singleton class, but abstract class
 * AbstractListModel requires that it not be static.
 *
 * Consumer: use ListDataListener to listen for intervalAdded event,
 * then try to consume a ScanSettings job object, if available.
 * Consumer must loop to consume queued jobs.
 *
 * To avoid broken synchronization state issues between Swing and consumer
 * threads, all interfaces run on the Swing thread.
 * When interfaces are called from other threads, they block
 * until the Swing thread completes the actions on its behalf.
 */

public class ScanSettingsListModel extends AbstractListModel {

  public final DefaultListSelectionModel selectionModel;

  // the jobs being managed by this list model
  private final Vector<ScanSettings> jobs = new Vector<ScanSettings>();

  // AbstractModel requires that this singleton resource be non-static
  public ScanSettingsListModel() {
    selectionModel = new DefaultListSelectionModel();
    selectionModel.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

  }

  // Add ScanSettings to tail (bottom) of LIFO job queue
  // and increment the sempahore.
  private class RunnableAdd implements Runnable {
    private final ScanSettings scanSettings;
    public RunnableAdd(ScanSettings scanSettings) {
      this.scanSettings = scanSettings;
    }
    public void run() {
      WLog.log("ScanSettingsListModel.add job '"
               + scanSettings.getCommandString() + "'");
      jobs.add(scanSettings);
      fireIntervalAdded(this, jobs.size()-1, jobs.size()-1);
      selectionModel.setSelectionInterval(jobs.size() - 1, jobs.size() - 1);
    }
  }

  // Remove and return ScanSettings from head (top) of LIFO job queue.
  // and decrement the sempahore else return null.
  private class RunnableRemoveTop implements Runnable {
    private ScanSettings scanSettings = null;
    public void run() {
      boolean removed = false;
      if (jobs.size() >= 1) {
        WLog.log("ScanSettingsListModel.remove top job '"
                 + jobs.get(0).getCommandString() + "'");
        scanSettings = jobs.remove(0);
        removed = true;
      } else {
        // there are no jobs to remove
        WLog.log("ScanSettingsRunQueue.remove top: no top element to remove");
      }
      if (removed) {
        fireIntervalRemoved(this, 0, 0);
      }
    }
  }

  // Remove specified ScanSettings object from within the job queue
  // and decrement the sempahore else return null.
  private class RunnableRemoveJob implements Runnable {
    private final ScanSettings scanSettings;
    private boolean removed = false;
    RunnableRemoveJob(ScanSettings scanSettings) {
      this.scanSettings = scanSettings;
    }
    public void run() {
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
      if (removed) {
        fireIntervalRemoved(this, index, index);
      }
    }
  }

  // Move ScanSettings up toward the top of the queue.
  private class RunnableMoveUp implements Runnable {
    private final ScanSettings scanSettings;
    private boolean moved = false;
    RunnableMoveUp(ScanSettings scanSettings) {
      this.scanSettings = scanSettings;
    }

    public void run() {
      int n = jobs.indexOf(scanSettings);
      if (n > 0) {
        ScanSettings oldUpper = jobs.set(n-1, scanSettings);
        ScanSettings newLower = jobs.set(n, oldUpper);
        moved = true;

      } else {
        WLog.log("ScanSettingsRunQueue.moveUp: failure at index " + n);
      }
      if (moved) {
        fireContentsChanged(this, n-1, n);
        selectionModel.setSelectionInterval(n-1, n-1);
      }
    }
  }

  // Move ScanSettings down toward the bottom of the queue.
  private class RunnableMoveDown implements Runnable {
    private final ScanSettings scanSettings;
    private boolean moved = false;
    RunnableMoveDown(ScanSettings scanSettings) {
      this.scanSettings = scanSettings;
    }

    public void run() {
      int n = jobs.indexOf(scanSettings);
      if (n != -1 && n < jobs.size() - 1) {
        ScanSettings oldLower = jobs.set(n+1, scanSettings);
        ScanSettings newUpper = jobs.set(n, oldLower);
        moved = true;

      } else {
        WLog.log("ScanSettingsRunQueue.moveDown: failure at index " + n);
      }
      if (moved) {
        fireContentsChanged(this, n, n+1);
        selectionModel.setSelectionInterval(n+1, n+1);
      }
    }
  }

  // Number of scan settings enqueued.  Required by interface ListModel.
  private class RunnableGetSize implements Runnable {
    private int size;

    public void run() {
      this.size = jobs.size();
    }
  }

  // Element at index.  Required by interface ListModel.
  private class RunnableGetElementAt implements Runnable {
    private final int index;
    private ScanSettings scanSettings;
    RunnableGetElementAt(int index) {
      this.index = index;
    }
    public void run() {
      scanSettings = jobs.get(index);
    }
  }

  // run the runnable on the Swing thread and return when done
  private void doRun(Runnable runnable) {
    if (SwingUtilities.isEventDispatchThread()) {
      // run using this Swing thread
      runnable.run();
    } else {
      try {
        // run using the Swing thread
        SwingUtilities.invokeAndWait(runnable);
      } catch (Exception e) {
        // catch rather than letting the callee's thread die
        WLog.log("ScanSettingsListModel.doRun error");
        WLog.logThrowable(e);
      }
    }
  }

  /**
   * Add ScanSettings to tail (bottom) of LIFO job queue
   * and increment the sempahore using the Swing thread.
   */
  public void add(ScanSettings scanSettings) {
    RunnableAdd runnableAdd = new RunnableAdd(scanSettings);
    doRun(runnableAdd);
  }

  /**
   * Remove and return ScanSettings from head (top) of LIFO job queue
   * and decrement the sempahore else return null.
   */
  public ScanSettings remove() {
    RunnableRemoveTop runnableRemoveTop = new RunnableRemoveTop();
    doRun(runnableRemoveTop);
    return runnableRemoveTop.scanSettings;
  }

  /**
   * Remove specified ScanSettings job from within the job queue
   * and decrement the sempahore else return null.
   */
  public boolean remove(ScanSettings scanSettings) {
    RunnableRemoveJob runnableRemoveJob = new RunnableRemoveJob(scanSettings);
    doRun(runnableRemoveJob);
    return runnableRemoveJob.removed;
  }

  /**
   * Move ScanSettings up toward the top of the queue.
   */
  public boolean moveUp(ScanSettings scanSettings) {
    RunnableMoveUp runnableMoveUp = new RunnableMoveUp(scanSettings);
    doRun(runnableMoveUp);
    return runnableMoveUp.moved;
  }

  /**
   * Move ScanSettings down toward the bottom of the queue.
   */
  public boolean moveDown(ScanSettings scanSettings) {
    RunnableMoveDown runnableMoveDown = new RunnableMoveDown(scanSettings);
    doRun(runnableMoveDown);
    return runnableMoveDown.moved;
  }

  /**
   * Number of scan settings enqueued.
   * Required by interface ListModel.
   */
  public int getSize() {
    RunnableGetSize runnableGetSize = new RunnableGetSize();
    doRun(runnableGetSize);
    return runnableGetSize.size;
  }

  /**
   * Element at index.
   * Required by interface ListModel.
   */
  public ScanSettings getElementAt(int index) {
    RunnableGetElementAt runnableGetElementAt = new RunnableGetElementAt(index);
    doRun(runnableGetElementAt);
    return runnableGetElementAt.scanSettings;
  }
}


import java.util.Observable;

/**
 * The <code>ModelChangedNotifier</code> class notifies observers of the given model change.
 * It enforces that it not be called in a nested manner.
 */
public class ModelChangedNotifier<E> extends Observable {
  boolean busy = false;

  // enable this switch to trace event flow
  private static final boolean DEBUG = false;
//  private static final boolean DEBUG = true;

  // synchronized
  private synchronized void setBusy(boolean busy) {
    this.busy = busy;
  }
  private synchronized boolean isBusy() {
    return busy;
  }

  /**
   * call this to notify Observer listeners that the model changed.
   */
  public void fireModelChanged(E obj) {
    if (isBusy()) {
      throw new RuntimeException("busy: " + this);
    } else {
      setBusy(true);
      if (DEBUG) {
        WLog.log("fireModelChanged.start>: " + this);
      }
      // notify observers
      setChanged();
      try {
        notifyObservers(obj);
      } finally {
        if (DEBUG) {
          WLog.log("fireModelChanged.done<: " + this);
        }
        setBusy(false);
      }
    }
  }
}


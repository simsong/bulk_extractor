import java.util.Observer;
import java.io.BufferedReader;
import java.io.IOException;

/**
 * The <code>ThreadReaderModel</code> monitors a stream and fires events when data arrives.
 */
public class ThreadReaderModel extends Thread {

  private final BufferedReader bufferedReader;

  private final ModelChangedNotifier<String> readerModelChangedNotifier
                = new ModelChangedNotifier<String>();

  public ThreadReaderModel(BufferedReader bufferedReader) {
    this.bufferedReader = bufferedReader;
    start();
  }

  // entry point
  public void run() {
    // notify lines of input until buffered reader closes
    while(true) {

      try {
        // blocking wait until EOF
        String input = bufferedReader.readLine();
        if (input == null) {
          break;
        } else {
          readerModelChangedNotifier.fireModelChanged(input);
        }
      } catch (IOException e) {
        WLog.log("ThreadReaderModel.run aborting.");
        break;
      }
    }
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param readerModelChangedListener the <code>Observer</code> to be added
   */
  public void addReaderModelChangedListener(Observer readerModelChangedListener) {
    readerModelChangedNotifier.addObserver(readerModelChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param readerModelChangedListener the <code>Observer</code> to be removed
   */
  public void removeReaderModelChangedListener(Observer readerModelChangedListener) {
    readerModelChangedNotifier.deleteObserver(readerModelChangedListener);
  }
}


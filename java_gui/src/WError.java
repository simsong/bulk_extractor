import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;
import javax.swing.JDialog;
import java.lang.InterruptedException;
import java.lang.reflect.InvocationTargetException;

/**
 * A modal pop-up dialog for reporting error conditions.
 * Error conditions are also logged, along with any exception stack chain.
 * This class should be available for all threads.
 */
public class WError {

  private static String composeAndLogError(String text, Throwable e) {
    final String message;

    // compose message
    if (e == null) {
      message = text;
    } else {
      message = text + "\n" + e.toString();
    }

    // log the error
    WLog.log("Error: " + text);
    if (e != null) {
      WLog.logThrowable(e);
    }

    return message;
  }

  private static void showDialog(String text, String title, int type) {
    JOptionPane pane = new JOptionPane(text, type);
    JDialog dialog = pane.createDialog(BEViewer.getBEWindow(), title);
    dialog.setAlwaysOnTop(true);
    dialog.setVisible(true);
  }

/**
 * Pops up the error dialog with the given text and title.
 * If a <code>Throwable</code> event is provided, the event title is shown
 * and the event and its call stack are logged.
 * @param text the message to display
 * @param title the title for the dialog window
 * @param e the <code>Throwable</code> object, if available, associated with the error
 */
  public static void showError(String text, final String title, Throwable e) {
    String message = composeAndLogError(text, e);

    // now show the error in a dialog component
    // NOTE: this fancy mess keeps Swing from breaking when showError is called
    // from a thread that is not the Swing dispatch thread.
    if (SwingUtilities.isEventDispatchThread()) {
      // in general usage, the thread is the event dispatch thread
      showDialog(message, title, JOptionPane.ERROR_MESSAGE);
//      JOptionPane.showMessageDialog(BEViewer.getBEWindow(),
//                                    message, title, JOptionPane.ERROR_MESSAGE);
    } else {
      // schedule this immediately on the Swing dispatch thread
      try {
        SwingUtilities.invokeAndWait(new RunnableMessage(message, title, JOptionPane.ERROR_MESSAGE));
      } catch (Exception ie) {
        WLog.log("Error: Unable to provide error message: '" + message
               + "' Title: '" + title + "'");
        WLog.logThrowable(ie);
      }
    }
  }

/**
 * Pops up the error dialog with the given text and title on the Swing queue, later,
 * allowing the current thread to keep progressing.
 * The event is logged immediately.
 * @param text the message to display
 * @param title the title for the dialog window
 * @param e the <code>Throwable</code> object, if available, associated with the error
 */
  public static void showErrorLater(String text, final String title, Throwable e) {
    String message = composeAndLogError(text, e);
    SwingUtilities.invokeLater(new RunnableMessage(message, title, JOptionPane.ERROR_MESSAGE));
  }

/**
 * Pops up the message dialog with the given text and title.
 * @param message the message to display
 * @param title the title for the dialog window
 */
  public static void showMessage(final String message, final String title) {

    // log the message
    WLog.log("Note: " + message);

    // now show the message in a dialog component
    // NOTE: this fancy mess keeps Swing from breaking when showMessage is called
    // from a thread that is not the Swing dispatch thread.
    if (SwingUtilities.isEventDispatchThread()) {
      // in general usage, the thread is the event dispatch thread
      showDialog(message, title, JOptionPane.INFORMATION_MESSAGE);
//      JOptionPane.showMessageDialog(BEViewer.getBEWindow(),
//                                    message, title, JOptionPane.INFORMATION_MESSAGE);
    } else {
      // schedule this immediately on the Swing dispatch thread
      try {
        SwingUtilities.invokeAndWait(new RunnableMessage(message, title, JOptionPane.INFORMATION_MESSAGE));
      } catch (Exception ie) {
        WLog.log("Error: Unable to provide message: '" + message
               + "' Title: '" + title + "'");
        WLog.logThrowable(ie);
      }
    }
  }

/**
 * Pops up the message dialog with the given text and title on the Swing queue, later,
 * allowing the current thread to keep progressing.
 * The event is logged immediately.
 * @param message the message to display
 * @param title the title for the dialog window
 */
  public static void showMessageLater(String message, final String title) {

    // log the message
    WLog.log("Note: " + message);

    SwingUtilities.invokeLater(new RunnableMessage(message, title, JOptionPane.INFORMATION_MESSAGE));
  }

  // class for allowing the message to be scheduled on the Swing event dispatch queue
  private static final class RunnableMessage implements Runnable {
    private final String message;
    private final String title;
    private final int type;
    public RunnableMessage(String message, String title, int type) {
      this.message = message;
      this.title = title;
      this.type = type;
    }
    public void run() {
      showDialog(message, title, type);
//      JOptionPane.showMessageDialog(BEViewer.getBEWindow(), message, title, type);
    }
  }
}


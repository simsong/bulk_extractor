import java.awt.*;
import javax.swing.*;

/**
 * A pop-up window for displaying diagnostics log information.
 * This class should be available for all threads and should be thread-safe.
 */
public class WLog implements Thread.UncaughtExceptionHandler {
  private static final WLog wLog = new WLog();
  private static final StringBuffer textBuffer = new StringBuffer();
  private static JDialog window;
  private static JTextArea textArea;

  private WLog() {
  }

  /**
   * Sets this object to intercept uncaught exceptions so that they may be reported.
   */
  public static void setExceptionHandling() {
    Thread.setDefaultUncaughtExceptionHandler(wLog);
  }

  /**
   * Appends the text to the log record.
   * If the log window is open, the view is updated to include the appended text.
   * @param text the text to be appended to the log record
   */
  public static void log(String text) {
    textBuffer.append(text);
    textBuffer.append("\n");
    System.out.println(text);

    // if the window is showing then update the view
    if (window != null && window.isVisible()) {
      setLogText();
    }
  }

  private static void setLogText() {
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        textArea.setText(textBuffer.toString());
      }
    });
  }

  /**
   * Appends the array as diagnostics to the log record.
   * @param text the text to be appended to the log record
   * @param bytes the bytes to be logged
   */
  public static void logBytes(String text, byte[] bytes) {
    StringBuffer buffer = new StringBuffer();

    // generate text
    buffer.append("\n");
    buffer.append(text);
    buffer.append(", size: " + bytes.length);
    buffer.append("\n");

    int i;

    // add bytes as text
    for (i=0; i<bytes.length; i++) {
      byte b = bytes[i];
      if (b > 31 && b < 127) {
        buffer.append((char)b);	// printable
      } else {
        buffer.append(".");	// not printable
      }
    }
    buffer.append("\n");

    // add bytes as hex
    for (i=0; i<bytes.length; i++) {
      buffer.append(String.format("%1$02x", bytes[i]));
      buffer.append(" ");
    }
    buffer.append("\n");

    // generate log string
    String logString = buffer.toString();

    // send to textBuffer
    textBuffer.append(logString);

    // send to stdout
    System.out.println(logString);

    // if the window is showing then update the view
    if (window != null && window.isVisible()) {
      setLogText();
    }
  }

  /**
   * Appends the Throwable exception stack trace chain to the log record.
   * @param e the exception object to log
   */
  public static void logThrowable(Throwable e) {
    // append Throwable text
    textBuffer.append(e.toString());
    textBuffer.append("\n");

    // append Throwable stack trace
    StackTraceElement[] stackTraceElement = e.getStackTrace();
    for (int i=0; i<stackTraceElement.length; i++) {
      textBuffer.append(stackTraceElement[i]);
      textBuffer.append("\n");
    }

    // print the stack trace to stderr
    e.printStackTrace();	// to stderr

    // if the window is showing then update the view
    if (window != null && window.isVisible()) {
      setLogText();
    }
  }

  /**
   * Clears the current log text
   */
  public static void clearLog() {
    // append Throwable text
    textBuffer.delete(0, textBuffer.length());

    // if the window is showing then update the view
    if (window != null && window.isVisible()) {
      setLogText();
    }
  }

  /**
   * Returns the accumulated log text.
   * @return the accumulated log text
   */
  public static String getLog() {
    return textBuffer.toString();
  }

  // Thread.UncaughtExceptionHandler
  /**
   * Intercepts uncaught exceptions and logs the exception information that is provided.
   * @param t the Thread that cast the exception
   * @param e the exception that was thrown
   */
  public void uncaughtException(Thread t, Throwable e) {
    // append Thread information
    textBuffer.append("Error:\n");
    textBuffer.append(t.toString());
    textBuffer.append("\n");

    // append Throwable stack trace
    logThrowable(e);

    // make the window visible whether it was already visible or not
    setVisible();
  }

  /**
   * Shows the Log in the Log window.
   * The Log window is created if it has not been shown yet.
   */
  public static void setVisible() {
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        // create the JDialog window if it does not exist yet
        if (window == null) {
          // create the window component
          window = new JDialog((Frame)null, "Bulk Extractor Viewer Log", false);
          Container pane = window.getContentPane();

          // use GridBagLayout with GridBagConstraints
          pane.setLayout(new GridBagLayout());

          // (0,0) JTextArea
          GridBagConstraints c = new GridBagConstraints();
          c.insets = new Insets(0, 0, 0, 0);
          c.gridx = 0;
          c.gridy = 0;
          c.weightx = 1;
          c.weighty = 1;
          c.fill = GridBagConstraints.BOTH;

          // create the text area that holds the Log information
          textArea = new JTextArea();
          textArea.setEditable(false);

          // put the text area in a scroll pane
          JScrollPane scrollPane= new JScrollPane(textArea,
                       ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
                       ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
          scrollPane.setPreferredSize(new Dimension(700,700));

          // add the scroll pane containing the text area that holds the Log information
          pane.add(scrollPane, c);

          // pack the window
          window.pack();
        }

        // update the text area to contain the current log content
        setLogText();

        // position and show the window
        window.setLocationRelativeTo(BEViewer.getBEWindow());
        window.setVisible(true);
      }
    });
  }
}


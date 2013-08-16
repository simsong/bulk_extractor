import java.awt.*;
import javax.swing.*;

/**
 * A pop-up window for showing indeterminate progress.
 * This window has no user controls.
 * To start, call startProgress once, or more if overridable starts are issued.
 * To stop, call stopProgress.
 * The progress window will become visible after a delay.
 * The two Interfaces are thread-safe and visibility actions are scheduled
 * on the event dispatch queue.
 */
public class WIndeterminateProgress {

  private DelayerThread delayerThread;

  private final JDialog window;
  private final JProgressBar bar;
  private final JTextArea textArea = new JTextArea();

  /**
   * Creates a pop-up window for showing image reader progress.
   * @param title the window title
   */
  public WIndeterminateProgress(String title) {
    // initialize the progress window
    window = new JDialog(BEViewer.getBEWindow(), title, false);
// NOTE: Removed setAlwaysOnTop because Error windows get covered.
// It would be nice to have a way to keep this progress window near the top.
//    window.setAlwaysOnTop(true);
    window.setFocusableWindowState(false);	// don't take focus when visible
    window.setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);

    Container pane = window.getContentPane();
    GridBagConstraints c;

    // use GridBagLayout with GridBagConstraints
    pane.setLayout(new GridBagLayout());

    // (0,0) feature line text area
    c = new GridBagConstraints();
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;
    textArea.setMinimumSize(new Dimension(400, 100));
    textArea.setPreferredSize(new Dimension(400, 100));
    textArea.setEditable(false);
    pane.add(textArea, c);

    // (0,1) JProgressBar
    c = new GridBagConstraints();
    c.gridx = 0;
    c.gridy = 1;
//    c.anchor = GridBagConstraints.FIRST_LINE_START;

    // create the progress bar component
    bar = new JProgressBar();
    bar.setStringPainted(true);
    bar.setString("Reading");
    bar.setIndeterminate(true);  // animated
    bar.setMinimumSize(new Dimension(100, 16));
    bar.setPreferredSize(new Dimension(100, 16));

    // add the progress bar component
    pane.add(bar, c);

    // pack the window
    window.pack();
  }

  /**
   * Initiate the process of opening a progress window after a delay.
   * @param text text to show in the progress window
   */
  public synchronized void startProgress(String text) {
    // set the new text
    SwingUtilities.invokeLater(new SwingSetText(text));

    if (delayerThread != null) {
      // terminate old delayer thread
      delayerThread.interrupt();
      delayerThread = null;
    }

    // start new delayer thread
    delayerThread = new DelayerThread();
    delayerThread.start();
  }

  /**
   * Closes the progress window whether it is open or not.
   */
  public synchronized void stopProgress() {
    if (delayerThread == null) {
      // program error: stop issued without start
      WLog.log("WIndeterminateProgress stopProgress error: stop without start.");
    } else {
      delayerThread.interrupt();
      delayerThread = null;
    }

    // visible or not, set to not visible
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        window.setVisible(false);
      }
    });
  }

  // run this on the Swing thread to set text
  private class SwingSetText extends Thread {
    String text;
    SwingSetText(String text) {
      this.text = text;
    }
    public void run() {
      textArea.setText(text);
    }
  }

  // delay and then schedule to open the window
  // unless the thread is interrupted by interrupt()
  private class DelayerThread extends Thread {
    private static final int DELAY = 300; // ms
    public void run() {
      try {
        // to avoid potential flicker, delay a bit
        sleep(DELAY);

        // if still active, open the progress window
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            window.toFront();
            window.setLocationRelativeTo(BEViewer.getBEWindow());
            window.setVisible(true);
          }
        });
      } catch (InterruptedException e) {
        // great, we didn't open the progress window
      }
    }
  }
}


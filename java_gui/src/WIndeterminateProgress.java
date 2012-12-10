import java.awt.*;
import javax.swing.*;

/**
 * A pop-up window for showing indeterminate progress.
 * This window has no user controls.
 * Visibility is enabled after a delay.
 * Interfaces are thread-safe.
 */
public class WIndeterminateProgress {

  private static final int DELAY = 300; // ms
  private DelayerThread delayerThread;
  private boolean active = false;	// synchronized

  private final JDialog window;
  private final JProgressBar bar;
  private final JLabel label = new JLabel();

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

    // (0,0) feature line label
    c = new GridBagConstraints();
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    label.setMinimumSize(new Dimension(400, 16));
    label.setPreferredSize(new Dimension(400, 16));
    pane.add(label, c);

    // (0,1) JProgressBar
    c = new GridBagConstraints();
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.FIRST_LINE_START;

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
   * Starts the progress window.
   * The window appears after delay
   * @param text text to show in the progress window
   */
  public synchronized void startProgress(String text) {
    // set label text to indicate current progress action
    label.setText(text);

    // either update existing progress or start new progress
    if (active) {
      window.toFront();
//      throw new RuntimeException("invalid state");
    } else {

      // set active and set thread to show progress after delay
      active = true;
      delayerThread = new DelayerThread();
      delayerThread.start();
    }
  }

  /**
   * Closes the progress window.
   */
  public synchronized void stopProgress() {
    // validate state
    if (!active) {
      throw new RuntimeException("invalid state");
    }

    // signal this thread to leave this window alone
    delayerThread.cancel();

    // hide the window
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        window.setVisible(false);
      }
    });

    // release the lock
    active = false;
  }

  // set visibility after DELAY
  private class DelayerThread extends Thread {
    private boolean isCancelled = false;

    public synchronized void cancel() {
      isCancelled = true;
    }

    public synchronized void maybeShowWindow() {
      if (!isCancelled) {
        // show the window
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            window.setLocationRelativeTo(BEViewer.getBEWindow());
            window.setVisible(true);
          }
        });
      }
    }

    // run the delayer thread
    public void run() {
      // perform the delay
      try {
        sleep(DELAY);
      } catch (InterruptedException e) {
        WLog.log("WIndeterminateProgress DelayerThread interrupt");
      }

      // show the window
      maybeShowWindow();
    }
  }
}


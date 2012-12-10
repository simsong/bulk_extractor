import java.awt.*;
import java.util.Date;
import javax.swing.*;

/**
 * A pop-up window containing a progress bar.
 * This window has no user controls and is programatically closed.
 * Interfaces are synchronized and UI actions occur on the Swing Dispatch Thread.
 */
public class WProgress {
  /**
   * The delay before making the window visible in order to reduce visual flicker.
   */
  public static final long DELAY = 300;	// ms

  private JDialog window;
  private JProgressBar bar;
  private boolean active = false;
  private boolean visible = false;
  private long startTime = 0;
  private int percent = 0;

  private final Runnable showWindow = new Runnable() {
    public void run() {
      // position and show the window
      window.setLocationRelativeTo(BEViewer.getBEWindow());
      window.setVisible(true);
    }
  };
  private final Runnable hideWindow = new Runnable() {
    public void run() {
      window.setVisible(false);
    }
  };

  private final Runnable setProgressIndicator = new Runnable() {
    public void run() {
      // set the percent value
      bar.setValue(percent);
    }
  };

  /**
   * Creates a window containing a progress bar.
   * @param title the title of the window
   */
  public WProgress(String title) {
    // create the progress window
    window = new JDialog(BEViewer.getBEWindow(), title, false);
    window.setFocusableWindowState(false);	// don't take focus when visible
    window.setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
    Container pane = window.getContentPane();

    // use GridBagLayout with GridBagConstraints
    pane.setLayout(new GridBagLayout());

    // (0,0) JProgressBar
    GridBagConstraints c = new GridBagConstraints();
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;

    // create the progress bar component
    bar = new JProgressBar(0, 100);
    bar.setStringPainted(true);
    bar.setMinimumSize(bar.getPreferredSize());

    // add the progress bar component
    pane.add(bar, c);

    // pack the window
    window.pack();
  }

  /**
   * Controls visibility of the progress window.
   * Actual visibility is delayed by <code>DELAY</code> in order to avoid visual flicker.
   * @param active whether progress is active or not
   */
  public synchronized void setActive(boolean active) {
    // make sure this is not already set
    if (this.active == active) {
      throw new RuntimeException("invalid usage: active: " + active);
    }
    this.active = active;
    if (active) {	// to become visible
      startTime = new Date().getTime();
    } else  {
      // if visibility is beinig turned off, hide the window
      active = false;
      visible = false;
      SwingUtilities.invokeLater(hideWindow);

      // log diagnostics timing between start and stop
      long delta = new Date().getTime() - startTime;
      WLog.log("WProgress time in ms: " + delta);
    }
  }

  /**
   * Sets the percent progress value.
   * If the progress window is not visible
   * and the time since <code>setVisible()</code> is more than <code>DELAY</code>,
   * the progress window is made visible.
   * If the time since <code>setVisible()</code> is less than <code>DELAY</code>
   * or if the percent progress value has not changed, the progress indicator is not updated.
   * @param percent the progress value in percent of completion
   */
  public synchronized void setPercent(final int percent) {
    if (!active) {
      throw new RuntimeException("invalid usage");
    }
    if (!visible) {
      // if DELAY passed then set visible
      long presentTime = new Date().getTime();
      if (presentTime > startTime + DELAY) {
        // the DELAY has just passed
        visible = true;
        this.percent = percent;
        SwingUtilities.invokeLater(setProgressIndicator);
        SwingUtilities.invokeLater(showWindow);
      }
    } else {
      // the indicator is visible
      // optimize by not setting percent unnecessarily
      if (percent != this.percent) {
        this.percent = percent;
        SwingUtilities.invokeLater(setProgressIndicator);
      }
    }
  }
}


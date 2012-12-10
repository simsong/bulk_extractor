import java.lang.Process;
import java.io.BufferedReader;
import java.io.IOException;

/**
 * Places a timer on a process and kills the process if the timer times out.
 */
public class ThreadAborterTimer extends Thread {
  private final Process process;
  private final int msDelay;
  private boolean isAborted = false;

  /**
   * Cancel the timeout thred.
   */
  public synchronized void cancel() {
    isAborted = true;
  }

  /**
   * Creates a timer that will kill the process after the delay unless aborted.
   */
  public ThreadAborterTimer(Process process, int msDelay) {
    this.process = process;
    this.msDelay = msDelay;
    this.start();
  }

  // this should be private.
  public void run() {
    try {
      sleep (msDelay);
    } catch (InterruptedException e) {
      throw new RuntimeException(e);
    }

    // if cancel() was not called, force kill
    if (isAborted == false) {

      // timeout so kill process
      WLog.log("ThreadAborterTimer process timeout on process " + process + ", this: " + this);
      process.destroy();

      // verify that the process dies
      try {
        process.waitFor();
      } catch (InterruptedException e) {
        WLog.log("ThreadAborterTimer timeout failure");
        WLog.logThrowable(e);
      }
    }
  }
}


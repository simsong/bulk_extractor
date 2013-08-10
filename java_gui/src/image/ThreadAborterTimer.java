import java.lang.Process;
import java.io.BufferedReader;
import java.io.IOException;

/**
 * Imposes a timer on a process and kills the process if the timer times out.
 */
public class ThreadAborterTimer extends Thread {
  private final Process process;
  private final int msDelay;

  /**
   * Cancel the timeout thred.
   */
  public synchronized void cancel() {
    interrupt();
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
      // bad, the timeout happened, so destroy the process
      process.destroy();
      try {
        process.waitFor();
      } catch (InterruptedException e) {
        WLog.log("ThreadAborterTimer timeout failure");
        WLog.logThrowable(e);
      }
    } catch (InterruptedException e) {
      // good, the timer was aborted before timeout
    }
  }
}


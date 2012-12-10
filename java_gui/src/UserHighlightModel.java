import java.util.Vector;
import java.util.Observer;
import java.util.Observable;
import java.io.ByteArrayOutputStream;

/**
 * The <code>UserHighlightModel</code> manages user highlight settings.
 */

public class UserHighlightModel {

  // input settings
  private byte[] highlightBytes = new byte[0];
  private boolean highlightMatchCase = true;

  // calculated output
  private Vector<byte[]> byteVector = new Vector<byte[]>();

  // resources
  private final ModelChangedNotifier<Object> userHighlightChangedNotifier = new ModelChangedNotifier<Object>();

  /**
   * Sets the text to be highlighted
   * by filling byte[] and char[] Vectors
   * suitable for Image and Feature views.
   */
  public void setHighlightBytes(byte[] highlightBytes) {
    if (UTF8Tools.bytesMatch(highlightBytes, this.highlightBytes)) {
      // no change
      return;
    }

    // accept highlight bytes and respond to change
    this.highlightBytes = highlightBytes;

    // clear existing values
    byteVector.clear();

    if (highlightBytes.length > 0) {
      // there are user highlights to parse
      ByteArrayOutputStream s = new ByteArrayOutputStream();
      for (int i=0; i<highlightBytes.length; i++) {
        if (highlightBytes[i] == '|' && s.size() > 0) {
          // add highlight to vector and clear stream
          byteVector.add(s.toByteArray());
          s.reset();
        } else {
          // add byte to stream
          s.write(highlightBytes[i]);
        }
      }

      // add highlight
      if (s.size() > 0) {
        byteVector.add(s.toByteArray());
        s.reset();
      }
    }

    // signal model changed
    userHighlightChangedNotifier.fireModelChanged(null);
  }

  /**
   * Returns highlight string that the user has set.
   */
  public byte[] getHighlightBytes() {
    return highlightBytes;
  }

  /**
   * Returns Vector<byte[]> from user's highlightBytes which may include escape codes.
   */
  public Vector<byte[]> getUserHighlightByteVector() {
    return byteVector;
  }

  /**
   * Sets highlight match case.
   */
  public void setHighlightMatchCase(boolean highlightMatchCase) {
    // ignore if no change
    if (this.highlightMatchCase == highlightMatchCase) {
      return;
    }

    this.highlightMatchCase = highlightMatchCase;
    // signal model changed
    userHighlightChangedNotifier.fireModelChanged(null);
  }

  /**
   * returns highlight match case.
   */
  public boolean isHighlightMatchCase() {
    return highlightMatchCase;
  }

  // ************************************************************
  // listener registry
  // ************************************************************
  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param highlightChangedListener  the <code>Observer</code> to be added
   */
  public void addUserHighlightModelChangedListener(Observer highlightChangedListener) {
    userHighlightChangedNotifier.addObserver(highlightChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param highlightChangedListener  the <code>Observer</code> to be removed
   */
  public void removeUserHighlightModelChangedListener(Observer highlightChangedListener) {
    userHighlightChangedNotifier.deleteObserver(highlightChangedListener);
  }
}


import java.util.HashMap;

/**
 * The <code>ImageLine</code> class provides accessors for rendering an image line.
 */
public class ImageLine {

  /**
   * The forensic path of the image line
   */
  public final String lineForensicPath;
  /**
   * The text of the image line
   */
  public final String text;
  /**
   * The array of text index characters to be highlighted
   */
  public int[] highlightIndexes;	// the array of highlight indexes
  /**
   * The number of text index characters to be highlighted in the image line
   */
  public int numHighlights = 0;			// the number of highlights

  /**
   * Creates an <code>ImageLine</code> object.
   * @param lineForensicPath The forensic path of the image line
   * @param text The text of the image line
   * @param highlightIndexes The array of text index characters to be highlighted
   * @param numHighlights The number of text index characters to be highlighted in the image line
   */
  public ImageLine(String lineForensicPath, String text, int[] highlightIndexes, int numHighlights) {
    this.lineForensicPath = lineForensicPath;
    this.text = text;
    this.highlightIndexes = highlightIndexes;
    this.numHighlights = numHighlights;
  }

  /**
   * The <code>LineFormat</code> class identifies view format styles.
   */
  public static final class LineFormat {
    private static final HashMap<String, LineFormat> map = new HashMap<String, LineFormat>();
    /**
     * The Hex view format
     */
    public static final LineFormat HEX_FORMAT = new LineFormat("Hex");
    /**
     * The Text view format
     */
    public static final LineFormat TEXT_FORMAT = new LineFormat("Text");
    /**
     * The name of the view format
     */
    private final String name;
    private LineFormat(String name) {
      this.name = name;
      map.put(name, this);
    }
    /**
     * returns a string representation of this object.
     * @return the name of this object
     */
    public String toString() {
      return name;
    }
    /**
     * Returns the object associated with the name.
     */
    public static LineFormat lookup(String name) {
      if (map.containsKey(name)) {
        return map.get(name);
      } else {
        WLog.log("line format '" + name + "' not recognized.  Using '" + TEXT_FORMAT.toString() + "'");
        return TEXT_FORMAT;
      }
    }
  }
}


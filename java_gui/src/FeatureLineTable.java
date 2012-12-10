import java.io.File;
import java.util.HashMap;

//import java.awt.font.TextAttribute;
//import javax.swing.SwingUtilities;

/** class <code>FeatureLineTable</code> allows filtered feature file access by line number.
 * This class is optimized for memory by providing dynamic line indexing
 * without resorting to managing one Object per line.
 */
public class FeatureLineTable {
  private static final int LINES_PER_GROUP = 0x10000;
  private final HashMap<Integer, LineGroup> lineGroups = new HashMap<Integer, LineGroup>();
  private LineGroup activeLineGroup = null;
  private int totalLines = 0;
  private int widestLineLength = 0;

  private File imageFile;
  private File featureFile;

  /**
   * Constructs an object for holding feature line references and providing access to feature lines.
   * @param imageFile records the image file associated with the feature line table
   * @param featureFile records the feature file associated with the feature line table
   */
  public FeatureLineTable(File imageFile, File featureFile) {
    // use this information for creating FeatureLine
    this.imageFile = imageFile;
    this.featureFile = featureFile;
  }

  /**
   * Provides a group of lines as a native array, in support of speed and memory optimization.
   * It is faster and smaller to have native arrays, but allocation must be dynamic,
   * so we manage arrays in line groups.
   */
  private class LineGroup {
    private final long[] lineBeginIndex = new long[LINES_PER_GROUP];
    private final int[] lineLength = new int[LINES_PER_GROUP];
  }

  /**
   * Adds the line index values to their respective line group.
   * @param startByte the starting byte of the feature in the feature file
   * @param lineLength the length of the feature in the feature file
   */
  public void put(long startByte, int lineLength) {
    // this should be fairly fast and should not take up undue memory
    int lineGroupNumber = totalLines / LINES_PER_GROUP;
    int lineIndexInGroup = totalLines % LINES_PER_GROUP;

    // if this line starts a new line group, create the new group object
    if (lineIndexInGroup == 0) {
      activeLineGroup = new LineGroup();
      lineGroups.put(new Integer(lineGroupNumber), activeLineGroup);
    }

    // add the line startByte and lineLength at its indexed position
    activeLineGroup.lineBeginIndex[lineIndexInGroup] = startByte;
    activeLineGroup.lineLength[lineIndexInGroup] = lineLength;
    totalLines++;

    // track longest line
    if (lineLength > widestLineLength) {
      widestLineLength = lineLength;
    }
  }

  /**
   * Returns the feature line indexed by the given line number.
   * @param lineIndex the index of the line in the line table
   * @return the feature line at the associated index
   */
  public FeatureLine get(int lineIndex) {
    // it is an error if the requested line is out of range
    if (lineIndex < 0 || lineIndex >= totalLines) {
      throw new IndexOutOfBoundsException("Index " + lineIndex + " out of bounds.");
    }

    // determine which line group the line is in and which position the line is in its group
    int lineGroupNumber = lineIndex / LINES_PER_GROUP;
    int lineIndexInGroup = lineIndex % LINES_PER_GROUP;
    LineGroup lineGroup = lineGroups.get(new Integer(lineGroupNumber));

    // determine the start byte
    long startByte = lineGroup.lineBeginIndex[lineIndexInGroup];

    // determine the line length
    int lineLength = lineGroup.lineLength[lineIndexInGroup];

    // return the FeatureLine
    return new FeatureLine(imageFile, featureFile, startByte, lineLength);
  }

  /**
   * Returns the unprocessed byte width of the longest line.
   * The widest line dictates the window width.
   * @return the total unprocessed width in bytes of the longest line
   */
  // returns the byte width of the longest line.
  public int getWidestLineLength() {
    return widestLineLength;
  }

  /**
   * Returns the number of indexed features.
   * @return the number of indexed features
   */
  public int size() {
    return totalLines;
  }
}


import java.util.Vector;

/**
 * The <code>ImageReaderType</code> class identifies available readers
 * which may be specifically requested.
 */
public final class ImageReaderType {
  /**
   * The names of the available readers.
   */
  private static final Vector<ImageReaderType> imageReaderTypes = new Vector<ImageReaderType>();

  // image reader types
  public static final ImageReaderType OPTIMAL = new ImageReaderType("Optimal");
  public static final ImageReaderType JAVA_ONLY = new ImageReaderType("Java Only");
  public static final ImageReaderType BULK_EXTRACTOR = new ImageReaderType("bulk_extractor Only");
  public static final ImageReaderType LIBEWFCS_TESTER = new ImageReaderType("libewfcs Test Only");

  private final String name;
  private ImageReaderType(String name) {
    this.name = name;
    imageReaderTypes.add(this);
  }

  /**
   * returns the array of image reader types
   */
  public static Vector<ImageReaderType> getImageReaderTypes() {
    return imageReaderTypes;
  }

  public String toString() {
    return name;
  }
}



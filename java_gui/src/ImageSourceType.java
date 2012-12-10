/**
 * The <code>ImageSourceType</code> class identifies where images are expected to come from.
 */
public final class ImageSourceType{
  public static final ImageSourceType IMAGE_FILE = new ImageSourceType("Image file");
  public static final ImageSourceType RAW_DEVICE = new ImageSourceType("Raw device");
  public static final ImageSourceType DIRECTORY_OF_FILES = new ImageSourceType("Directory of files");
  private final String name;
  private ImageSourceType(String name) {
    this.name = name;
  }
  public String toString() {
    return name;
  }
}


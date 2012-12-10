import java.io.File;

/**
 * The <code>FileTools</code> class provides static null-safe file operators.
 */
public final class FileTools {

  /**
   * Returns whether files are equvalent, accounting for null.
   */
  public static boolean filesAreEqual(File file1, File file2) {
    if (file1 == null) {
      // true if both null
      return (file2 == null);
    } else {
      // true if both equal
      return (file1.equals(file2));
    }
  }

  /**
   * Returns value, accounting for null.
   */
  public static String getAbsolutePath(File file) {
    if (file == null) {
      return "";
    } else {
      return file.getAbsolutePath();
    }
  }
}


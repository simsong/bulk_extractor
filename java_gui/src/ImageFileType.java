import java.io.File;
import java.io.IOException;
import javax.swing.filechooser.FileFilter;

/**
 * Provides image file type constants and code for determining image file types.
 */

public class ImageFileType {
  // the suffix arrays
  private static final String[] RAW_SUFFIX_ARRAY = {".raw", ".img", ".dd"};
  private static final String[] MULTIPART_SUFFIX_ARRAY = {".000", ".001", "001.vmdk"};
  private static final String[] E01_SUFFIX_ARRAY = {".E01"};
  private static final String[] AFF_SUFFIX_ARRAY = {".aff"};

  // the image file types
  public static final ImageFileType RAW = new ImageFileType("Raw image");
  public static final ImageFileType MULTIPART = new ImageFileType("Multipart image");
  public static final ImageFileType E01 = new ImageFileType("E01 image");
  public static final ImageFileType AFF  = new ImageFileType("AFF image");

  /**
   * The image file filter for image file types.
   */
  public static final ImageFileFilter imageFileFilter = new ImageFileFilter();

  private final String name;
  private ImageFileType(String name) {
    this.name = name;
  }

  /**
   * Returns the string name of the image file type
   */
  public String toString() {
    return name;
  }

  /**
   * see if file suffix is in the given suffix array
   */
  private static boolean hasSuffixType(String[] suffixArray, File file) {
    String filename = file.getName();
    int length = suffixArray.length;
    for (int i=0; i< length; i++) {
      if (filename.toLowerCase().endsWith(suffixArray[i].toLowerCase())) {
        return true;
      }
    }
    return false;
  }

  /**
   * Returns the image file type
   */
  public static ImageFileType getImageFileType(File file) {

    if (hasSuffixType(MULTIPART_SUFFIX_ARRAY, file)) {
      return MULTIPART;
    } else if (hasSuffixType(E01_SUFFIX_ARRAY, file)) {
      return E01;
    } else if (hasSuffixType(AFF_SUFFIX_ARRAY, file)) {
      return AFF;
    } else {
      // all unrecognized extensions are RAW
      return RAW;
    }
  }

  private static class ImageFileFilter extends FileFilter {
    // accept filenames for valid image files
    /**
     * Whether the given file is accepted by this filter.
     * @param file The file to check
     * @return true if accepted, false if not
     */
    public boolean accept(File file) {
      // directory
      if (file.isDirectory()) {
        return true;
      }

      // valid suffix
      if (hasSuffixType(RAW_SUFFIX_ARRAY, file)
       || hasSuffixType(E01_SUFFIX_ARRAY, file)
       || hasSuffixType(AFF_SUFFIX_ARRAY, file)
       || file.getAbsolutePath().startsWith("/dev/")) {	// allow raw devices for Linux
        return true;
      }

      // valid multipart file suffix
      if (isValidFirstMultipartFile(file)) {
        return true;
      }
      return false;
    }

    /**
     * Indicates whether the specified file is a valid first multipart file.
     * Note that .001 is not valid if .000 exists in the same directory.
     * @param firstFile the first file in the multipart file sequence
     */
    private static boolean isValidFirstMultipartFile(File firstFile) {
      String firstFileString = firstFile.getAbsolutePath();
      // .000
      if (firstFileString.endsWith(".000")) {
        return true;
      }
      // .001
      if (firstFileString.endsWith(".001")) {
        String possible000FileString = firstFileString.substring(0, firstFileString.length() - 1) + "0";
        File possible000File = new File(possible000FileString);
        if (possible000File.isFile()) {
          // a .000 file exists, so the .001 file is not the first
          return false;
        } else {
          return true;
        }
      }
      // 001.vmdk
      if (firstFileString.endsWith("001.vmdk")) {
        return true;
      }

      return false;
    }

    /**
     * The description of this filter.
     * @return the description of this filter
     */
    public String getDescription() {
      return "Image Files (" + getSupportedImageFileTypes() + ")";
    }

    /**
     * Returns all supported file types, separated by a space
     */
    private static String getSupportedImageFileTypes() {
      StringBuffer buffer = new StringBuffer();
      int i;
      for (i = 0; i<RAW_SUFFIX_ARRAY.length; i++) {
        buffer.append(RAW_SUFFIX_ARRAY[i] + " ");
      }
      for (i = 0; i<MULTIPART_SUFFIX_ARRAY.length; i++) {
        buffer.append(MULTIPART_SUFFIX_ARRAY[i] + " ");
      }
      for (i = 0; i<E01_SUFFIX_ARRAY.length; i++) {
        buffer.append(E01_SUFFIX_ARRAY[i] + " ");
      }
      for (i = 0; i<AFF_SUFFIX_ARRAY.length; i++) {
        buffer.append(AFF_SUFFIX_ARRAY[i] + " ");
      }

      // non-Windows also allows "/dev"
      if (!System.getProperty("os.name").startsWith("Windows")) {
        buffer.append("/dev");
      }

      return buffer.toString().trim();
    }
  }
}


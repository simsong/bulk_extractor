import java.io.File;
import java.io.IOException;

/**
 * The <code>BulkExtractorPathReader</code> class provides path reading services
 * using the bulk_extractor utility, which unpacks compressed paths
 */
public class JavaReader implements ImageReaderInterface {

  private ImageFileReaderInterface reader;

  public JavaReader(ImageFileReaderInterface reader) {
    this.reader = reader;
  }

  /**
   * Returns the requested bytes.
   * @param featureLine the feature line defining the image to read from
   * @param imageAddress the address to start reading from
   * @param numBytes the number of bytes to read
   * @throws IOException if the read fails
   */
  public byte[] readPathBytes(final FeatureLine featureLine,
                          final long imageAddress, final int numBytes)
                          throws IOException {

    // prohibit unless address read request
    if (featureLine.getType() != FeatureLine.FeatureLineType.ADDRESS_LINE) {
      throw new IOException("Only address features are allowed for Java readers");
    }

    // forward the read request
    byte[] bytes = reader.readImageBytes(imageAddress, numBytes);
    return bytes;
  }

  /**
   * Returns the size of the image identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @throws IOException if the file access fails
   */
  public long getPathSize(final FeatureLine featureLine) throws IOException {
    // get the size
    return reader.getImageSize();
  }

  /**
   * Returns the image metadata identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @throws IOException if the file access fails
   */
  public String getPathMetadata(final FeatureLine featureLine) throws IOException {

    // get the metadata
    return reader.getImageMetadata();
  }

  /**
   * Removes the reader associated with the file.
   */
  public void close() throws IOException {
    reader.close();
  }
}


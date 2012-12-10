import java.io.File;
import java.io.IOException;

/**
 * The <code>BulkExtractorPathReader</code> class provides path reading services
 * using the bulk_extractor utility, which unpacks compressed paths
 */
public class BulkExtractorReader implements ImageReaderInterface {

  private final File file;
  private BulkExtractorFileReader reader;

  public BulkExtractorReader(File file) throws IOException {
    this.file = file;
    reader = new BulkExtractorFileReader(file);
  }

  // restart a faied reader
  private void validateReader() throws IOException {
    // see if the reader's bulk_extractor process is still alive
    if (!reader.isAlive()) {
      // restart the reader
      reader.close();
      reader = new BulkExtractorFileReader(file);
    }
  }

  /**
   * Returns the requested bytes.
   * @param featureLine the feature line defining the image to read from
   * @param startAddress the address to start reading from
   * @param numBytes the number of bytes to read
   * @throws IOException if the read fails
   */
  public byte[] readPathBytes(final FeatureLine featureLine,
                          final long startAddress, final int numBytes)
                          throws IOException {

    // NOTE: do not check for startAddress > EOF

    // read the page bytes
    validateReader();
    byte[] bytes = reader.readBulkExtractorPathBytes(featureLine, startAddress, numBytes);
    return bytes;
  }

  /**
   * Returns the size of the image identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @throws IOException if the file access fails
   */
  public long getPathSize(final FeatureLine featureLine) throws IOException {
    // get the size
    validateReader();
    return reader.getBulkExtractorPathSize(featureLine);
  }

  /**
   * Returns the image metadata identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @throws IOException if the file access fails
   */
  public String getPathMetadata(final FeatureLine featureLine) throws IOException {

    // get the metadata
    validateReader();
    return reader.getBulkExtractorPathMetadata(featureLine);
  }

  /**
   * Removes the reader associated with the file.
   */
  public void close() throws IOException {
    reader.close();
  }
}


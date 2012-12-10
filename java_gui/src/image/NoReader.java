import java.io.File;
import java.io.IOException;

/**
 * Provides functionality for when no reader is selected.
 */
public class NoReader implements ImageReaderInterface {

  /**
   * Returns the requested bytes.
   * @param featureLine the feature line defining the image to read from
   * @param startAddress the address to start reading from
   * @param numBytes the number of bytes to read
   * @return the bytes read
   * @throws IOException if the read fails
   */
  public byte[] readPathBytes(final FeatureLine featureLine,
                              final long startAddress, final int numBytes)
                              throws IOException {
     return new byte[0];
  }

  /**
   * Returns the size of the image identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the size of the image identified by the feature line.
   * @throws IOException if the file access fails
   */
  public long getPathSize(final FeatureLine featureLine) throws IOException {
    return 0;
  }

  /**
   * Returns the metadata associated with the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the metadata associated with the feature line
   * @throws IOException if the file access fails
   */
  public String getPathMetadata(final FeatureLine featureLine) throws IOException {
    return "No metadata available.  Please select a Feature to obtain metadata.";
  }

  /**
   * close the reader associated with the file.
   */
  public void close() throws IOException {
    // no action
  }
}


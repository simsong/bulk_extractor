import java.io.File;
import java.io.IOException;

/**
 * Provides a reader to use when the requested reader fails.
 */
public class FailReader implements ImageReaderInterface {

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
      WError.showError("Unable to read Image data."
                       + "\nPlease verify that the Image file is valid.",
                       "BEViewer Image File Read error", null);
     return new byte[0];
  }

  /**
   * Returns the size of the image identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the size of the image identified by the feature line.
   * @throws IOException if the file access fails
   */
  public long getPathSize(final FeatureLine featureLine) throws IOException {
    return Long.MAX_VALUE;
  }

  /**
   * Returns the metadata associated with the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the metadata associated with the feature line
   * @throws IOException if the file access fails
   */
  public String getPathMetadata(final FeatureLine featureLine) throws IOException {
    return "No metadata available.";
  }

  /**
   * close the reader associated with the file.
   */
  public void close() throws IOException {
    // no action
  }
}


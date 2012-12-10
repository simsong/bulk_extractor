import java.io.File;
import java.io.IOException;

/**
 * An interface for reading path information specified by <code>FeatureLine</code>s.
 * Specific image readers should implement this interface.
 */
public interface ImageReaderInterface {

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
                              throws IOException;

  /**
   * Returns the size of the image identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the size of the image identified by the feature line.
   * @throws IOException if the file access fails
   */
  public long getPathSize(final FeatureLine featureLine) throws IOException;

  /**
   * Returns the metadata associated with the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the metadata associated with the feature line
   * @throws IOException if the file access fails
   */
  public String getPathMetadata(final FeatureLine featureLine) throws IOException;

  /**
   * close the reader associated with the file.
   */
  public void close() throws IOException;
}


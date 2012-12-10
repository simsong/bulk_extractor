import java.io.IOException;

/**
 * An interface for reading from an image.
 * Media image readers should implement this interface.
 */
public interface ImageFileReaderInterface {

  /**
   * Reads media image bytes into the byte array.
   * @param imageAddress the address within the image to read
   * @param numBytes the number of bytes to read
   * @return the byte array containing the bytes read
   * @throws IOException if the file cannot be read
   * or if the requested number of bytes cannot be read
   */
  public byte[] readImageBytes(long imageAddress, int numBytes) throws IOException;

  /**
   * Returns the size in bytes of the image.
   * @return the size in bytes of the image
   * @throws IOException if the size cannot be determined
   */
  public long getImageSize() throws IOException;

  /**
   * Returns metadata specific to the image.
   * @return metadata specific to the image
   * @throws IOException if image metadata cannot be obtained
   */
  public String getImageMetadata() throws IOException;

  /**
   * Closes the reader, releasing resources.
   */
  public void close() throws IOException;
}


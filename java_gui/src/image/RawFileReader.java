import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import java.nio.ByteBuffer;

/**
 * The <code>RawFileReader</code> class provides image reading services for raw files.
 */
public class RawFileReader implements ImageFileReaderInterface {

  private final File file;
  private final FileInputStream fileInputStream;
  private final FileChannel fileChannel;
  private final long imageSize;

  /**
   * Creates an image reader for reading a raw file.
   * If the reader fails then an error dialog is displayed
   * and an empty reader is returned in its place.
   * @param newFile the raw file
   * @throws IOException if the reader cannot be created
   */
  public RawFileReader(File newFile) throws IOException {
    file = newFile;

    try {
      // set the raw file and file channel to use for reading media image pages
      fileInputStream = new FileInputStream(file);
      fileChannel = fileInputStream.getChannel();

      // set the raw file image size
      if (file.isFile()) {
        // this is a normal file so size is available
        imageSize = fileChannel.size();
      } else {
        // this is not a normal file so get the size by setting position and attempting to read

        long raw_offset = 0;
        int bits;
        int status;
        byte[] bytes = new byte[1];

        // phase 1: find MSB where read still works
        for (bits=0; bits<60; bits++) {
          raw_offset = (long)1 << bits;
          ByteBuffer byteBuffer = ByteBuffer.wrap(bytes);
          status = fileChannel.read(byteBuffer, raw_offset);
          if (status == -1) {
            // reached EOF
            break;
          }
        }
        if (bits == 60) {
          throw new IOException("Partition detection failed.");
        }

        // phase 2: going toward LSB, turn on bits when read still works
        int i;
        for (i=bits; i>=0; i--) {
          long test = (long)1<<i;
          long test_filesize = raw_offset | test;
          ByteBuffer byteBuffer = ByteBuffer.wrap(bytes);
          status = fileChannel.read(byteBuffer, test_filesize);
          if (status == 1) {
            raw_offset |= test;
          } else {
            raw_offset &= ~test;
          }
        }

        imageSize = raw_offset + 1; // size is last valid byte + 1
      }
    } catch (Exception e) {
      // rethrow whatever as an IOException
      // note: gjc doesn't support IOException(Throwable) so use this:
      Throwable cause = e.getCause();
      IOException ioe = new IOException(cause == null ? null : cause.toString());
      ioe.initCause(cause);
      throw ioe;
    }
  }

  /**
   * Returns the size in bytes of the image in the raw file.
   * @return the size in bytes of the image in the raw file
   */
  public long getImageSize() {
    return imageSize;
  }

  /**
   * Reads the requested number of bytes from the raw file at the specified start address.
   * @param imageAddress the address within the image to read
   * @param numBytes the number of bytes to read
   * @return the byte array read
   * @throws IOException if the number of bytes cannot be read
   * or if the requested number of bytes cannot be read
   */
  public byte[] readImageBytes(long imageAddress, int numBytes) throws IOException {

    // past EOF
    if (imageAddress >= imageSize) {
      return new byte[0];
    }

    // near EOF: truncate actual read if read request passes end of image
    if (imageAddress + numBytes > imageSize) {
      numBytes = (int)(imageSize - imageAddress);
    }

    // read from file channel int byte buffer
    // NOTE: don't use fileChannel.map because this approach fails when rading raw devices on linux
    byte[] bytes = new byte[numBytes];
    ByteBuffer byteBuffer = ByteBuffer.wrap(bytes);
    int numBytesRead = fileChannel.read(byteBuffer, imageAddress);
    if (numBytes != numBytesRead) {
      throw new IOException("File read error: Requested: " + numBytes + ", read: " + numBytesRead);
    }

// NOTE: don't use fileChannel.map because this approach fails when rading raw devices on linux
//    // read page into bytes
//    MappedByteBuffer rawMappedByteBuffer;
//
//    // read into MappedByteBuffer
//WLog.log("RawFileReader.readImageBytes imageAddress: " + imageAddress + ", numBytes: " + numBytes);
//    rawMappedByteBuffer = fileChannel.map(MapMode.READ_ONLY, imageAddress, numBytes);
//WLog.log("RawFileReader.readImageBytes.b");
//    rawMappedByteBuffer.load();
//WLog.log("RawFileReader.readImageBytes.c");
//    int bytesRead = rawMappedByteBuffer.limit();
//WLog.log("RawFileReader.readImageBytes.d");
//
//    // make sure the full number of bytes were read
//    if (numBytes != bytesRead) {
//      throw new IOException("File read error: Requested: " + numBytes + ", read: " + bytesRead);
//    }
//
//    // create the new byte array
//    byte[] bytes = new byte[numBytes];
//
//    // copy read into bytes[]
//    rawMappedByteBuffer.get(bytes, 0, numBytes);

    // return the byte array from the raw mapped byte buffer
    return bytes;
  }

  /**
   * Returns image metadata specific to raw files.
   * @return image metadata specific to raw files
   */
  public String getImageMetadata() {
    StringBuffer buffer = new StringBuffer();

    // filename
    buffer.append("Raw file filename: ");
    buffer.append(file.getAbsolutePath());

    // file size
    buffer.append("\n");
    buffer.append("Image size: ");
    String imageSizeString = String.format(BEViewer.LONG_FORMAT, imageSize);
    buffer.append(imageSizeString);

    // return the properties
    return buffer.toString();
  }

  /**
   * Closes the reader, releasing resources.
   */
  public void close() throws IOException {
    fileInputStream.close();
    fileChannel.close();
  }
}


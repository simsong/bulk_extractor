import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;

/**
 * The <code>MultipartFileReader</code> class provides image reading services for serialized
 * raw files.
 */
public class MultipartFileReader implements ImageFileReaderInterface {

  private final File firstFile;
  private final long firstFileSize;
  private final long imageSize;

  /**
   * Creates an image reader for reading a serialized raw file.
   * If the reader fails then an error dialog is displayed
   * and an empty reader is returned in its place.
   * @param firstFile the first raw file in the serial sequence
   * @throws IOException if the reader cannot be created
   */
  public MultipartFileReader(File firstFile) throws IOException {

    // validate the file as the first serial file
    if (!isValidFirstFile(firstFile)) {
      throw new IOException("Invalid first serial file: " + firstFile.toString());
    }

    // set first file
    this.firstFile = firstFile;

    // get first file size
    firstFileSize = firstFile.length();

    // find the media image size, verifying equal file sizes along the way
    int fileIndex = 1;
    while (true) {
      File currentFile = getSerialFile(firstFile, fileIndex);
      File nextFile = getSerialFile(firstFile, fileIndex + 1);

      if (nextFile.isFile()) {
        // there are more files
        fileIndex++;

        // ensure that the current file is of equal size
        if (currentFile.length() != firstFileSize) {
          throw new IOException("Serialized file " + currentFile.toString()
                              + " is not equal to the expected file size.");
        }

      } else {
        // this is the last file

        // ensure that the current file size is not larger than the established file size
        if (currentFile.length() > firstFileSize) {
          throw new IOException("Serialized file " + currentFile.toString()
                              + " is larger than the expected file size.");
        }

        // calculate total media image size
        imageSize = firstFileSize * fileIndex + currentFile.length();
        break;
      }
    }
  }

  /**
   * Indicates whether the specified file is a valid first file.
   * Note that .001 is not valid if .000 exists in the same directory.
   * @param firstFile the first file in the multipart file sequence
   */
  public static boolean isValidFirstFile(File firstFile) {
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
   * Returns the serial file.
   * @param firstFile the first file in the multipart file sequence
   * @param fileIndex the index of the serial file, starting at 0.
   * @return the serial file
   */
  private File getSerialFile(File firstFile, int fileIndex) {
    String firstFileString = firstFile.getAbsolutePath();
    String filePrefix;
    String serialFilename;
    // .000
    if (firstFileString.endsWith(".000")) {
      filePrefix = firstFileString.substring(0, firstFileString.length() - 3);
      serialFilename = filePrefix + String.format("%1$03d", fileIndex + 0);
      return new File(serialFilename);
    }

    // .001
    if (firstFileString.endsWith(".001")) {
      filePrefix = firstFileString.substring(0, firstFileString.length() - 3);
      serialFilename = filePrefix + String.format("%1$03d", fileIndex + 1);
      return new File(serialFilename);
    }

    // 001.vmdk
    if (firstFileString.endsWith("001.vmdk")) {
      filePrefix = firstFileString.substring(0, firstFileString.length() - 8);
      serialFilename = filePrefix + String.format("%1$03d", fileIndex + 1) + ".vmdk";
      return new File(serialFilename);
    }

    throw new RuntimeException("invalid usage");
  }

  /**
   * Returns the total size in bytes of the media image.
   * @return the total size in bytes of the media image
   */
  public long getImageSize() {
    return imageSize;
  }

  /**
   * Reads the image bytes at the specified start address.
   * @param imageAddress the address within the image to read
   * @param numBytes the number of bytes to read
   * @throws IOException on read failure
   * @return the byte array read
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

    long currentStartAddress = imageAddress;
    int currentNumBytes = numBytes;
    ByteArrayOutputStream outputStream = new ByteArrayOutputStream(numBytes);

    // build the return bytes out of smaller aligned byte reads
    while (currentNumBytes > 0) {
      // identify the current start address and current number of bytes to read
      int fileNumBytes;
      long fileStartAddress = currentStartAddress % firstFileSize;
      if (fileStartAddress + currentNumBytes > firstFileSize) {
        // the read crosses the file boundary, so read up to the file boundary
        fileNumBytes = (int)(firstFileSize - fileStartAddress);	// guaranteed < Integer.MAX_VALUE
      } else {
        // the read does not cross the file boundary and this concludes the read request
        fileNumBytes = currentNumBytes;
      }

      // read the current start address and current number of bytes into the output stream
      byte[] sectionBytes = readAlignedBytes(currentStartAddress, fileNumBytes);
      outputStream.write(sectionBytes, 0, fileNumBytes);

      // calculate the start address and number of bytes for the next read
      currentStartAddress += fileNumBytes;
      currentNumBytes -= fileNumBytes;
    }

    // convert the output stream to a byte array
    byte[] bytes = outputStream.toByteArray();

    // make sure the full number of bytes were read
    int bytesRead = bytes.length;
    if (numBytes != bytesRead) {
      throw new IOException("File read error: Requested: " + numBytes + ", read: " + bytesRead);
    }


    return outputStream.toByteArray();
    // return the output stream as a byte array
  }

  /**
   * Reads the page at the specified start address.
   * The number of bytes read is equal to the size of the byte array.
   * @param pageStartAddress the start address of the currently read page
   * @param numBytes the number of bytes to read
   * @throws IOException if the requested number of bytes cannot be read
   * @return the byte array read
   */
  private byte[] readAlignedBytes(long pageStartAddress, int numBytes) throws IOException {
    // force runtime failure if pageStartAddress is out of range
    // force runtime failure on any exception

    // calculate file index
    long longFileIndex = pageStartAddress / firstFileSize;

    // validate file index
    if (longFileIndex > 1000) {
      throw new IOException("Invalid serial index required for this read: " + longFileIndex);
    }
    int fileIndex = (int)longFileIndex;

    // calculate file and file start address to read
    File file = getSerialFile(firstFile, fileIndex);
    long fileStartAddress = pageStartAddress % firstFileSize;

    // open file channel for serial file
    final FileInputStream fileInputStream = new FileInputStream(file);
    final FileChannel fileChannel = fileInputStream.getChannel();

    // read from file channel into MappedByteBuffer
    MappedByteBuffer rawMappedByteBuffer;
    rawMappedByteBuffer = fileChannel.map(MapMode.READ_ONLY, fileStartAddress, numBytes);
    rawMappedByteBuffer.load();
    int bytesRead = rawMappedByteBuffer.limit();

    // make sure the full number of bytes were read
    if (numBytes != bytesRead) {
      throw new RuntimeException("Serial file read error: Requested: "
                                 + numBytes + ", read: " + bytesRead);
    }

    // create the new byte array
    byte[] bytes = new byte[numBytes];

    // copy read into the new byte array
    rawMappedByteBuffer.get(bytes, 0, numBytes);

    // close file resources
    fileChannel.close();
    fileInputStream.close();

    // return the byte array from the raw mapped byte buffer
    return bytes;
  }

  /**
   * Returns metadata specific to serial raw files.
   * @return metadata specific to serial raw files
   */
  public String getImageMetadata() {
    StringBuffer buffer = new StringBuffer();

    // filename
    buffer.append("Serial raw file filename: ");
    buffer.append(getSerialFile(firstFile, 0));

    // individual file size
    buffer.append("\n");
    buffer.append("Individual serialized file size: ");
    String fileSizeString = String.format(BEViewer.LONG_FORMAT, firstFileSize);
    buffer.append(fileSizeString);

    // total media image size
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
  public void close() {
    // no action, this reader does not leave file resources open between reads.
  }

}


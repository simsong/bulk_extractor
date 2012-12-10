/*
 * The software provided here is released by the Naval Postgraduate
 * School, an agency of the U.S. Department of Navy.  The software
 * bears no warranty, either expressed or implied. NPS does not assume
 * legal liability nor responsibility for a User's use of the software
 * or the results of such use.
 *
 * Please note that within the United States, copyright protection,
 * under Section 105 of the United States Code, Title 17, is not
 * available for any work of the United States Government and/or for
 * any works created by United States Government employees. User
 * acknowledges that this software contains work which was created by
 * NPS government employees and is therefore in the public domain and
 * not subject to copyright.
 *
 * Released into the public domain on December 17, 2010 by Bruce Allen.
 */

package edu.nps.jlibewf;

import java.io.File;
import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.Iterator;
import org.apache.log4j.Logger;

/**
 * The <code>EWFFileReader</code> class reads EWF files formatted in the .E01 format.
 */
public class EWFFileReader {

  /**
   * The build date of this version, {@value}.
   */
  public static final String VERSION_DATE = "20110721";

  /**
   * The default logger name, {@value}.
   */
  public static final String DEFAULT_LOGGER_NAME = "edu.nps.jlibewf";

  /**
   * The logger to which log information in the <code>jlibewf</code> package is sent,
   * see org.apache.log4j.Logger.
   * The logger is initially set to <code>DEFAULT_LOGGER_NAME</code>.
   */
  protected static Logger logger = Logger.getLogger(DEFAULT_LOGGER_NAME);
  static {
    logger.info("EWFFileReader build date " + VERSION_DATE);
  }

  /**
   * Sets the logger to the logger identified by the new logger name.
   * @param loggerName the name of the new logger to use instead
   */
  public void setLogger(String loggerName) {
    logger = Logger.getLogger(loggerName);
  }

  private File firstFile;
  private final ArrayList<EWFSection.SectionPrefix> sectionPrefixArray = new ArrayList<EWFSection.SectionPrefix>();
  private int chunkSize;
  private long imageSize = 0;
  private EWFSegmentFileReader reader;

  /**
   * Constructs the EWF file reader for reading EWF files formatted in the .E01 format.
   * @param file the first EWF file in the serial sequence
   * @throws IOException if the reader cannot be created
   */
  public EWFFileReader(File file) throws IOException {

    // set the segment file reader to use
    reader = new EWFSegmentFileReader();

    // set the default chunk size
    chunkSize = reader.DEFAULT_CHUNK_SIZE;

    // validate the file as the first EWF file
    if (!reader.isValidFirstEWFFilename(file)) {
      throw new IOException("Invalid first EWF filename file " + file.toString());
    }

    // set file as first file
    firstFile = file;

    // cache all section prefix entries
    loadSectionPrefixArray();

    // cache the chunk size since this dictates data size
    loadChunkSize();

    // cache the media size
    loadMediaSize();
  }

  /**
   * Reads the media image bytes into the byte array, where the media image bytes
   * are read from EWF files formatted in the .E01 format.
   * @param pageStartByte the start address of the page to read
   * @param numBytes the number of bytes to read
   * @throws IOException if the requested number of bytes cannot be read
   * @return the byte array read
   */
  private byte[] readAlignedBytes(long pageStartByte, int numBytes) throws IOException {
    // calculate chunk start byte
    long chunkIndex = pageStartByte / chunkSize;
    long chunkStartByte = chunkIndex * chunkSize;

    // verify that chunk index fits in int
    if (!EWFSection.isPositiveInt(chunkIndex)) {
      throw new IOException("Invalid chunk index: " + chunkIndex);
    }

    // read the chunk
    byte[] chunkBytes = readMediaChunk((int)chunkIndex);

    // find the offset of the requested data within the chunk
    long longOffset = pageStartByte - chunkStartByte;

    // verify that the offset fits within int before typecasting to int
    if (!EWFSection.isPositiveInt(longOffset)) {
      throw new IOException("Invalid chunk offset: " + longOffset);
    }

    // set the offset of the requested data within the chunk
    int offset = (int)longOffset;

    // fail if the requested bytes cannot be read
    if (offset + numBytes > chunkBytes.length) {
      throw new IOException("Insufficient bytes read: offset: " + offset
               + ", number of bytes: " + numBytes + ", length: " + chunkBytes.length);
    }

    // return the requested aligned bytes read
    ByteArrayOutputStream outStream = new ByteArrayOutputStream(numBytes);
    outStream.write(chunkBytes, offset, numBytes);
    byte[] alignedBytes = outStream.toByteArray();
    return alignedBytes;
  }

  // interface ImageReader

  /**
   * Reads the image bytes at the specified start address.
   * The number of bytes read is equal to the size of the byte array.
   * @param imageAddress the address within the image to read
   * @param numBytes the number of bytes to read
   * @throws IOException if the requested number of bytes cannot be read
   * @return the byte array read
   */
  public byte[] readImageBytes(long imageAddress, int numBytes) throws IOException {

    // past EOF
    if (imageAddress >= imageSize) {
      return new byte[0];
    }

    // truncate actual read if read request passes end of image
    if (imageAddress + numBytes > imageSize) {
      numBytes = (int)(imageSize - imageAddress);
    }

    long currentStartAddress = imageAddress;
    int currentNumBytes = numBytes;
    ByteArrayOutputStream outputStream = new ByteArrayOutputStream(numBytes);

    // build the return bytes out of smaller aligned byte reads
    while (currentNumBytes > 0) {
      // identify the current start address and current number of bytes to read
      int chunkNumBytes;
      int chunkStartAddress = (int)(currentStartAddress % chunkSize);
      if (chunkStartAddress + currentNumBytes > chunkSize) {
        // the read crosses the chunk boundary, so read up to the chunk boundary
        chunkNumBytes = chunkSize - chunkStartAddress;
      } else {
        // the read does not cross the file boundary and this concludes the read request
        chunkNumBytes = currentNumBytes;
      }

      // read the current start address and current number of bytes into the output stream
      byte[] sectionBytes = readAlignedBytes(currentStartAddress, chunkNumBytes);
      outputStream.write(sectionBytes, 0, chunkNumBytes);

      // calculate the start address and number of bytes for the next read
      currentStartAddress += chunkNumBytes;
      currentNumBytes -= chunkNumBytes;
    }

    // return the output stream as a byte array
    return outputStream.toByteArray();
  }

  /**
   * Returns the size in bytes of the media image within the EWF files formatted in the .E01 format.
   * @return the size in bytes of the image
   */
  public long getImageSize() {
    return imageSize;
  }

  // loads the section prefix array during initialization
  private void loadSectionPrefixArray() throws IOException {
    File nextFile = firstFile;
    long nextSectionStartAddress = EWFSegmentFileReader.FILE_FIRST_SECTION_START_ADDRESS;
    int nextChunkIndex = 0;

    // process all sections within all files
    while(true) {

      // get the next section prefix
      EWFSection.SectionPrefix sectionPrefix = new EWFSection.SectionPrefix(
                      reader, nextFile, nextSectionStartAddress, nextChunkIndex);

      // add the next section prefix
      sectionPrefixArray.add(sectionPrefix);

      // update the section start address
      nextSectionStartAddress = sectionPrefix.nextOffset;

      // update the next chunk index
      nextChunkIndex = sectionPrefix.nextChunkIndex;

      // move to next file
      if (sectionPrefix.sectionType == EWFSection.SectionType.NEXT_TYPE) {
        nextFile = reader.getNextFile(nextFile);
        nextSectionStartAddress = EWFSegmentFileReader.FILE_FIRST_SECTION_START_ADDRESS;
      }
  
      // stop after last file
      if (sectionPrefix.sectionType == EWFSection.SectionType.DONE_TYPE) {
        break;
      }
    }

    // log the number of sections used
    logger.info("Total section count: " + sectionPrefixArray.size());
  }

  // loads the chunk size during initialization
  private void loadChunkSize() throws IOException {
    // look for the Volume Section prefix because it contains chunk size information
    for (int i=0; i<sectionPrefixArray.size(); i++) {
      EWFSection.SectionPrefix sectionPrefix = sectionPrefixArray.get(i);
      if (sectionPrefix.sectionType == EWFSection.SectionType.VOLUME_TYPE) {

        // set the chunk size from bytes per sector * sectors per chunk
        EWFSection.VolumeSection volumeSection = new EWFSection.VolumeSection(reader, sectionPrefix);
        chunkSize = volumeSection.bytesPerSector * volumeSection.sectorsPerChunk;

        // log the chunk size used
        logger.info("EWFFileReader.loadChunkSize Chunk size: " + chunkSize);
        return;
      }
    }

    // note that the Volume Section could not be found
    logger.info("EWFFileReader.loadChunkSize: This media has no Volume Section.");
  }

  // reads the Header information
  private String readHeaderInformation() throws IOException {
    // look for the Header Section prefix because it contains the header information
    for (int i=0; i<sectionPrefixArray.size(); i++) {
      EWFSection.SectionPrefix sectionPrefix = sectionPrefixArray.get(i);
      if (sectionPrefix.sectionType == EWFSection.SectionType.HEADER_TYPE) {
        // get the header section as an object
        EWFSection.HeaderSection headerSection = new EWFSection.HeaderSection(reader, sectionPrefix);

        // return the header text from the header section
        return headerSection.getHeaderText();
      }
    }

    // note that the Header Section could not be found
    logger.info("EWFFileReader.readHeaderInformation: This media has no Header Section.");

    return "This media has no Header Section.";
  }

  // loads the media size during initialization
  private void loadMediaSize() throws IOException {
    // get last chunk index from the chunk index of the last Section prefix
    EWFSection.SectionPrefix sectionPrefix = sectionPrefixArray.get(sectionPrefixArray.size() - 1);
    int lastChunkIndex = sectionPrefix.nextChunkIndex - 1;

    // ensure that there are chunks
    if (lastChunkIndex == -1) {
      throw new IOException("No media chunks.");
    }

    // read last chunk
    byte[] bytes = readMediaChunk(lastChunkIndex);

    // set media size
    imageSize = (long)lastChunkIndex * chunkSize + bytes.length;

    // note load results
    logger.trace("EWFFileReader.loadMediaSize: lastChunkIndex: " + lastChunkIndex
                 + ", chunkSize: " + chunkSize + ", last chunk length: " + bytes.length
                 + ", final size: " +  String.format(EWFSegmentFileReader.longFormat, imageSize));
  }

  // reads the requested media chunk
  private byte[] readMediaChunk(int chunkIndex) throws IOException {

    // find the section prefix containing the chunk index
    Iterator<EWFSection.SectionPrefix> iterator = sectionPrefixArray.iterator();
    EWFSection.SectionPrefix sectionPrefix;
    while (true) {

      // bad data state if the section prefix containing the chunk index cannot be found
      if (!iterator.hasNext()) {
        throw new IOException("Section for chunk index " + chunkIndex + " cannot be found.");
      }

      // look for the section prefix containing the chunk index
      sectionPrefix = iterator.next();
      if (chunkIndex >= sectionPrefix.chunkIndex && chunkIndex < sectionPrefix.nextChunkIndex) {
        // the requested chunk index is within this range.
        break;
      }
    }

    // determine the table base offset from the table section, used by EnCase v.6+
    EWFSection.TableSection tableSection = new EWFSection.TableSection(reader, sectionPrefix);
    long tableBaseOffset = tableSection.tableBaseOffset;

    // log media offset value used
    if (tableBaseOffset != 0) {
      logger.info("EWFFileReader.readMediaChunk non-zero tableBaseOffset: "
                  + String.format(EWFSegmentFileReader.longFormat, tableBaseOffset));
    }

    // get the table section chunk table
    EWFSection.ChunkTable chunkTable = new EWFSection.ChunkTable(reader, sectionPrefix);

    // get the chunk table index with respect to the Table Section
    int chunkTableIndex = chunkIndex - sectionPrefix.chunkIndex;

    // get the file offset to the media chunk base
    long chunkStartOffset = chunkTable.getChunkStartOffset(chunkTableIndex);

    // determine if compression is used for the chunk
    boolean isCompressed = chunkTable.isCompressedChunk(chunkTableIndex);

    // log that compression was not used
    if (!isCompressed) {
      logger.info("EWFFileReader.readMediaChunk: No compression");
    }

    // set the media chunk start address
    long mediaChunkBeginAddress = chunkStartOffset + tableBaseOffset;

    // set the media chunk end address
    long mediaChunkEndedAddress;	// points to byte after end
    if (chunkIndex + 1 < sectionPrefix.nextChunkIndex) {
      // the end address of the chunk is just before the start address of the next chunk

      // get the file offset to the next media chunk base
      long nextChunkStartOffset = chunkTable.getChunkStartOffset(chunkTableIndex + 1);

      // set the media chunk end address
      mediaChunkEndedAddress = nextChunkStartOffset + tableBaseOffset;

    } else {

      // the end address is just before the start of another Section

      // loop through Section Prefixes to find the one surrounding the chunk's start address
      Iterator<EWFSection.SectionPrefix> endpointIterator = sectionPrefixArray.iterator();
      EWFSection.SectionPrefix addressedSectionPrefix;
      while (true) {

        // bad data state if the section prefix containing the media chunk address cannot be found
        if (!endpointIterator.hasNext()) {
          throw new IOException(
                        "Section surrounding address "
                        + String.format(EWFSegmentFileReader.longFormat, mediaChunkBeginAddress)
                        + " cannot be found.");
        }

        // get the next section prefix to check for encapsulating addresses
        addressedSectionPrefix = endpointIterator.next();

        // check that the addressed section prefix encapsulates the media chunk address
        if (addressedSectionPrefix.file == sectionPrefix.file
         && addressedSectionPrefix.fileOffset < mediaChunkBeginAddress
         && addressedSectionPrefix.nextOffset > mediaChunkBeginAddress) {
          // the section encapsulates the data so the chunk end address is just before
          // the start address of the next section
          mediaChunkEndedAddress = addressedSectionPrefix.nextOffset;
          break;
        }
      }
    }

    // verify the chunk size
    int mediaReadSize;
    if (!EWFSection.isPositiveInt(mediaChunkEndedAddress - mediaChunkBeginAddress)) {
      throw new IOException("Invalid media chunk size at section " + sectionPrefix.toString());
    } else {
      mediaReadSize = (int)(mediaChunkEndedAddress - mediaChunkBeginAddress);
    }

    // read the chunk
    byte[] bytes;
    if (isCompressed) {

      // read using decompression, which inherently verifies the checksum
      bytes = reader.readZlib(sectionPrefix.file, mediaChunkBeginAddress, mediaReadSize, chunkSize);
    } else {
      // extract the bytes using Adler32
      byte[] tempBytes = reader.readAdler32(sectionPrefix.file, mediaChunkBeginAddress, mediaReadSize);

      // remove the four checksum bytes
      ByteArrayOutputStream outStream = new ByteArrayOutputStream(tempBytes.length - 4);
      outStream.write(tempBytes, 0, tempBytes.length - 4);
      bytes = outStream.toByteArray();
    }

    // return the media from the chunk
    return bytes;
  }

  /**
   * Returns properties specific to EWF files formatted in the .E01 format.
   * @return properties specific to EWF files formatted in the .E01 format.
   */
  public String getImageProperties() throws IOException {
    StringBuffer buffer = new StringBuffer();

    // filename
    buffer.append("EWF file filename: ");
    buffer.append(firstFile.getAbsolutePath());

    // file size
    buffer.append("\n");
    buffer.append("Image size: ");
    String imageSizeString = String.format(EWFSegmentFileReader.longFormat, imageSize);
    buffer.append(imageSizeString);

    // chunk size
    buffer.append("\n");
    buffer.append("Chunk size: " + chunkSize);

    // header information
    buffer.append("\n");
    buffer.append("Volume header information:\n");
    buffer.append(readHeaderInformation());

    // return the properties
    return buffer.toString();

  }
  /**
   * Closes the reader, releasing resources.
   */
  public void close() throws IOException {
    if (reader != null) {
      reader.closeFileChannel();
    }
  }
}


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
import java.util.Map;
import java.util.HashMap;
//import org.apache.log4j.Logger;

/**
 * The <code>EWFSection</code> class provides objects that map to section structures
 * in EWF files formatted in the .E01 format.
 */
public class EWFSection {

  // ************************************************************
  // general helper function
  // ************************************************************
  /**
   * Indicates whether the specified long is a positive integer.
   * Use this before typecasting from <code>long</code> to <code>int</code>.
   * @param longInt the long that should be a positive integer
   * @return true if the value is a positive integer
   */
  public static boolean isPositiveInt(long longInt) {
    if (longInt > Integer.MAX_VALUE || longInt < 0) {
      return false;
    } else {
      return true;
    }
  }

  // ************************************************************
  // segment classes
  // ************************************************************

  /**
   * The <code>SectionType</code> class manages valid section types.
   */
  public static final class SectionType {
    private static final Map<String, SectionType> MAP = new HashMap<String, SectionType>();
    // the name of the Section type.
    private String sectionTypeString;
    private SectionType(String sectionTypeString) {
      // make sure the type is not added twice
      if (MAP.containsKey(sectionTypeString)) {
        throw new RuntimeException("Duplicate SectionType constant");
      }

      // add the section type
      this.sectionTypeString = sectionTypeString;
      MAP.put(sectionTypeString, this);
    }

    /**
     * Provides a visual representation of this object.
     * @return the string name of this section type object.
     */
    public String toString() {
      return sectionTypeString;
    }

    /**
     * Returns the SectionType object associated with this section type.
     * @return the SectionType object associated with this section type.
     * @throws IOException if the requested section type is not a valid section type
     */
    private static SectionType getSectionType(String sectionTypeString) throws IOException{
      SectionType sectionType = MAP.get(sectionTypeString);

      // It is an IO exception if the section type is not recognized
      if (sectionType == null) {
        throw new IOException("Invalid Section type: '" + sectionTypeString + "'");
      }
      return sectionType;
    }

    /**
     * The Header Section type.
     */
    public static final SectionType HEADER_TYPE = new SectionType("header");
    /**
     * The Volume Section type.
     */
    public static final SectionType VOLUME_TYPE = new SectionType("volume");
    /**
     * The Table Section type.
     */
    public static final SectionType TABLE_TYPE = new SectionType("table");
    /**
     * The Next Section type.
     */
    public static final SectionType NEXT_TYPE = new SectionType("next");
    /**
     * The Done Section type.
     */
    public static final SectionType DONE_TYPE = new SectionType("done");

    /**
     * header2 Section type.
     */
    public static final SectionType HEADER2_TYPE = new SectionType("header2");
    /**
     * Disk Section type.
     */
    public static final SectionType DISK_TYPE = new SectionType("disk");
    /**
     * Data Section type.
     */
    public static final SectionType DATA_TYPE = new SectionType("data");
    /**
     * Sectors Section type.
     */
    public static final SectionType SECTORS_TYPE = new SectionType("sectors");
    /**
     * Table2 Section type.
     */
    public static final SectionType TABLE2_TYPE = new SectionType("table2");
    /**
     * LTree Section type.
     */
    public static final SectionType LTREE_TYPE = new SectionType("ltree");
    /**
     * Digest Section type.
     */
    public static final SectionType DIGEST_TYPE = new SectionType("digest");
    /**
     * Hash Section type.
     */
    public static final SectionType HASH_TYPE = new SectionType("hash");
  }

  /**
   * An implementation of the prefix portion of a Section.
   * EWF Segment files contain sections such as "header section", "volume section",
   * or "table section".
   * Each section has a common section prefix.
   * Each section prefix defines the type of section it represents,
   * contains a pointer to the start of the next section prefix,
   * and contains an Adler32 checksum in order to verify that the byte fields
   * within the section prefix are not corrupted.
   */
  public static class SectionPrefix {
    private static final int SECTION_TYPE_OFFSET = 0;
    private static final int NEXT_SECTION_OFFSET = 16;
    private static final int SECTION_SIZE_OFFSET = 24;
//    private static final int SECTION_ADLER32_OFFSET = 72;
    /**
     * The size of the section prefix
     */
    public static final int SECTION_PREFIX_SIZE = 76;

    /**
     * The section type.
     */
    public SectionType sectionType;
    /**
     * The file that this section prefix is associated with
     */
    public File file;
    /**
     * The offset address into the file where this section starts.
     */
    public long fileOffset;
    /**
     * The offset address into the file where the next section starts.
     */
    public long nextOffset;
    /**
     * The size of this section.
     */
    public long sectionSize;
    /**
     * The running chunk index.
     */
    public int chunkIndex;
    /**
     * The next chunk index.  If larger than chunkIndex, then this section contains a chunk table.
     * This is a convenience variable because <code>nextChunkIndex = chunkIndex + chunkCount</code>.
     */
    public int nextChunkIndex;
    /**
     * The chunk count.  If non-zero, then this section contains a chunk table of count chunks.
     */
    public int chunkCount;

    /**
     * Constructs a Section Prefix based on bytes from the given EWF file and address.
     * @param reader the segment file reader to use for reading EWF files
     * @param file the file to read from
     * @param fileOffset the byte offset address in the file to read from
     * @param previousChunkCount the running count of media chunks defined so far
     * @throws IOException If an I/O error occurs, which is possible if the read fails
     * or if the Adler32 checksum validation fails
     */
    public SectionPrefix(EWFSegmentFileReader reader, File file, long fileOffset,
                         int previousChunkCount) throws EWFIOException {
      final int SECTION_TYPE_STRING_LENGTH = 16;

      // read the section prefix bytes
      byte[] bytes = reader.readAdler32(file, fileOffset, SectionPrefix.SECTION_PREFIX_SIZE);

      // set the section type
      String sectionTypeString = EWFSegmentFileReader.bytesToString(
                   bytes, SECTION_TYPE_OFFSET, SECTION_TYPE_STRING_LENGTH);
      try {
        sectionType = SectionType.getSectionType(sectionTypeString);
      } catch (IOException e) {
        throw new EWFIOException(e.getMessage(), file, fileOffset);
      }

      // set the file
      this.file = file;

      // set the file offset
      this.fileOffset = fileOffset;

      // set the address of the next section
      nextOffset = EWFSegmentFileReader.bytesToLong(bytes, NEXT_SECTION_OFFSET);

      // set the section size
      sectionSize = EWFSegmentFileReader.bytesToLong(bytes, SECTION_SIZE_OFFSET);

      // set the chunk offset values
      chunkIndex = previousChunkCount;
      if (sectionType == SectionType.TABLE_TYPE) {
        // read the chunk count for this table section
        TableSection tableSection = new TableSection(reader, this);
        nextChunkIndex = chunkIndex + tableSection.tableChunkCount;
        chunkCount = tableSection.tableChunkCount;
      } else {
        // leave the chunk count alone
        nextChunkIndex = chunkIndex;
        chunkCount = 0;
      }
    }

    /**
     * Returns a string representation of this object.
     * @return a string representation of this object
     */
    public String toString() {
      return "Section Prefix type '" + sectionType.toString()
           + "' of file " + file.toString()
           + "\noffset " + String.format(EWFSegmentFileReader.longFormat, fileOffset)
           + " size " + String.format(EWFSegmentFileReader.longFormat, sectionSize)
           + " chunk index " + chunkIndex + " chunk count " + chunkCount;
    }
  }

  // ************************************************************
  // header section
  // ************************************************************
  /**
   * An implementation of the header section portion of a section.
   * The header section contains metadata about the series of EWF Segment files
   * such as case number and examiner name.
   * The header section portion does not include the section prefix.
   * <p>This class currently simply provides header text as a single string.
   * It may be improved to provide individual header fields.
   */
  public static class HeaderSection {
    private static final int HEADER_SECTION_OFFSET = 76;

    /**
     * The header text.
     * This class may be enhanced to provide formatted header fields.
     */
    private String headerText;

    /**
     * Reads the header section associated with the given section prefix
     * The header section bytes are decompressed and validated using zlib.
     * @param sectionPrefix the section prefix from which this header section is composed
     * @throws IOException If an I/O error occurs, which is possible if the requested read fails
     */
    public HeaderSection(EWFSegmentFileReader reader, SectionPrefix sectionPrefix)
                         throws EWFIOException {
      File file = sectionPrefix.file;
      long fileOffset = sectionPrefix.fileOffset;
      long sectionSize = sectionPrefix.sectionSize;

      // make sure the section prefix is correct
      if (sectionPrefix.sectionType != SectionType.HEADER_TYPE) {
        throw new RuntimeException("Invalid section type");
      }

      // make sure the header section size is small enough to cast to int
      if (!isPositiveInt(sectionSize)) {
        throw new EWFIOException("Invalid large Header Section size", file, fileOffset);
      }

      // read decompressed header section into bytes[]
      long address = fileOffset + HEADER_SECTION_OFFSET;
      int numBytes = (int)sectionSize - HEADER_SECTION_OFFSET;
      byte[] bytes = reader.readZlib(file, address, numBytes, reader.DEFAULT_CHUNK_SIZE);

      // convert bytes to String
      headerText = new String(bytes);
    }

    /**
     * Returns all the header section text as a single string.
     * @return the header section text as a single string
     */
    public String getHeaderText() {
      return headerText;
    }
  }

  // ************************************************************
  // volume section
  // ************************************************************
  /**
   * An implementation of the volume section portion of a section.
   * The <code>VolumeSection</code> class contains volume section values
   * such as chunk count and sectors per chunk.
   */
  public static class VolumeSection {
    private static final int CHUNK_COUNT_OFFSET = 80;
    private static final int SECTORS_PER_CHUNK_OFFSET = 84;
    private static final int BYTES_PER_SECTOR_OFFSET = 88;
    private static final int SECTOR_COUNT_OFFSET = 92;
    private static final int VOLUME_SECTION_SIZE = 1128;
    /**
     * The chunk count.
     */
    public int volumeChunkCount;
    /**
     * Sectors per chunk.
     */
    public int sectorsPerChunk;
    /**
     * Bytes per Sector.
     */
    public int bytesPerSector;
    /**
     * Sector count.
     */
    public int sectorCount;

    /**
     * Constructs a volume section based on bytes from the given EWF file and address.
     * @param sectionPrefix the section prefix from which this header section is composed
     * @throws IOException If an I/O error occurs, which is possible if the requested read fails
     * or if the Adler32 checksum validation fails
     */
    public VolumeSection(EWFSegmentFileReader reader, SectionPrefix sectionPrefix)
                         throws EWFIOException {
      File file = sectionPrefix.file;
      long fileOffset = sectionPrefix.fileOffset;
      long sectionSize = sectionPrefix.sectionSize;

      // make sure the section prefix is correct
      if (sectionPrefix.sectionType != SectionType.VOLUME_TYPE) {
        throw new RuntimeException("Invalid section type");
      }

      // make sure the section size is not smaller than the volume section data structure
      if (sectionSize < VOLUME_SECTION_SIZE) {
        throw new EWFIOException("Invalid small Volume Section size", file, fileOffset);
      }

      // read volume section into bytes[]
      long address = fileOffset + SectionPrefix.SECTION_PREFIX_SIZE;
      int numBytes = VOLUME_SECTION_SIZE - SectionPrefix.SECTION_PREFIX_SIZE;
      byte[] bytes = reader.readAdler32(file, address, numBytes);

      // now read the Volume Section's values

      long longChunkCount = EWFSegmentFileReader.bytesToUInt(bytes, CHUNK_COUNT_OFFSET - SectionPrefix.SECTION_PREFIX_SIZE);
      // make sure the chunk count is valid before typecasting it
      if (!isPositiveInt(longChunkCount)) {
        throw new EWFIOException("Invalid chunk count", file, fileOffset);
      }
      volumeChunkCount = (int)longChunkCount;

      // int sectorsPerChunk
      long longSectorsPerChunk = EWFSegmentFileReader.bytesToUInt(bytes, SECTORS_PER_CHUNK_OFFSET - SectionPrefix.SECTION_PREFIX_SIZE);
      // make sure the value is valid before typecasting it
      if (!isPositiveInt(longSectorsPerChunk)) {
        throw new EWFIOException("Invalid sectors per chunk", file, fileOffset);
      }
      sectorsPerChunk = (int)longSectorsPerChunk;

      // int bytesPerSector
      long longBytesPerSector = EWFSegmentFileReader.bytesToUInt(bytes, BYTES_PER_SECTOR_OFFSET - SectionPrefix.SECTION_PREFIX_SIZE);
      // make sure the value is valid before typecasting it
      if (!isPositiveInt(longBytesPerSector)) {
        throw new EWFIOException("Invalid bytes per sector", file, fileOffset);
      }
      bytesPerSector = (int)longBytesPerSector;

      // int sectorCount
      long longSectorCount = EWFSegmentFileReader.bytesToUInt(bytes, SECTOR_COUNT_OFFSET - SectionPrefix.SECTION_PREFIX_SIZE);
      // make sure the value is valid before typecasting it
      if (!isPositiveInt(longSectorCount)) {
        throw new EWFIOException("Invalid sector count", file, fileOffset);
      }
      sectorCount = (int)longSectorCount;
    }
  }

  // ************************************************************
  // table section
  // ************************************************************
  /**
   * An implementation of the table section portion of a section.
   * The table section contains the chunk count and table base offset.
   */
  public static class TableSection {
    private static final int CHUNK_COUNT_OFFSET = 76;
    private static final int TABLE_BASE_OFFSET = 84;
    /**
     * the offset into the chunk table offset array.
     */
    public static final int OFFSET_ARRAY_OFFSET = 100;

    /**
     * the number of chunks in this table section.
     */
    public int tableChunkCount;
    /**
     * the base offset of the integer chunk offsets with respect to the beginning of the file.
     */
    public long tableBaseOffset;

    /**
     * Constructs a table section based on bytes from the given EWF file and address.
     * @param reader the EWF reader instance to use for reading
     * @param sectionPrefix the section prefix from which this header section is composed
     * @throws IOException If an I/O error occurs, which is possible if the requested read fails
     */
    public TableSection(EWFSegmentFileReader reader, SectionPrefix sectionPrefix)
                        throws EWFIOException {

      File file = sectionPrefix.file;
      long fileOffset = sectionPrefix.fileOffset;
      long sectionSize = sectionPrefix.sectionSize;

      // make sure the section prefix is correct
      if (sectionPrefix.sectionType != SectionType.TABLE_TYPE) {
        throw new RuntimeException("Invalid section type");
      }

      // validate section size
      if (sectionSize < OFFSET_ARRAY_OFFSET) {
        throw new EWFIOException("table section chunk count size is too small", file, fileOffset);
      }

      // read the table section
      long address = fileOffset + SectionPrefix.SECTION_PREFIX_SIZE;
      int numBytes = OFFSET_ARRAY_OFFSET - SectionPrefix.SECTION_PREFIX_SIZE;
      byte[] bytes = reader.readAdler32(file, address, numBytes);

      // set table section values

      // tableChunkCount 
      long longChunkCount = EWFSegmentFileReader.bytesToUInt(bytes, CHUNK_COUNT_OFFSET - SectionPrefix.SECTION_PREFIX_SIZE);
      // make sure the value is valid before typecasting it
      if (!isPositiveInt(longChunkCount)) {
        throw new EWFIOException("Invalid chunk count", file, fileOffset);
      }
      tableChunkCount = (int)longChunkCount;

      // tableBaseOffset
      tableBaseOffset = EWFSegmentFileReader.bytesToLong(bytes, TABLE_BASE_OFFSET - SectionPrefix.SECTION_PREFIX_SIZE);
    }

    /**
     * Provides a visual representation of this object.
     */
    public String toString() {
      return "TableSection: tableChunkCount: " + tableChunkCount + " tableBaseOffset: " + tableBaseOffset;
    }
  }

  // ************************************************************
  // table section chunk table
  // ************************************************************
  /**
   * An implementation of the chunk table portion of a table section.
   * The table section contains accessors for reading a chunk entry.
   */
  public static class ChunkTable {
    private byte[] bytes;
    private int chunkCount;
    private File file;
    private long fileOffset;

    /**
     * Constructs a chunk table from the given section prefix.
     * @param sectionPrefix the section prefix from which this header section is composed
     */
    public ChunkTable(EWFSegmentFileReader reader, SectionPrefix sectionPrefix)
                      throws EWFIOException {

      file = sectionPrefix.file;
      fileOffset = sectionPrefix.fileOffset;
      long sectionSize = sectionPrefix.sectionSize;
      chunkCount = sectionPrefix.chunkCount;

      // make sure the section prefix is correct
      if (sectionPrefix.sectionType != SectionType.TABLE_TYPE) {
        throw new RuntimeException("Invalid section type");
      }

      // validate section size
      if (sectionSize < TableSection.OFFSET_ARRAY_OFFSET + chunkCount * 4) {
        throw new EWFIOException("table section chunk table size is too small", file, fileOffset);
      }

      // read the chunk table into bytes[]
      long address = fileOffset + TableSection.OFFSET_ARRAY_OFFSET;
      int numBytes;
      if (sectionSize >= TableSection.OFFSET_ARRAY_OFFSET + chunkCount * 4 + 4) {

        // add 4 bytes for the Adler32 checksum for the chunk table
        numBytes = chunkCount * 4 + 4;

        // read table with Adler32 checksum bytes
        bytes = reader.readAdler32(file, address, numBytes);

        // Although not required by the spec, note if there are extra bytes in the table section
        if (sectionSize != TableSection.OFFSET_ARRAY_OFFSET + chunkCount * 4 + 4) {
          EWFFileReader.logger.info("EWFSection.ChunkTable: Note: File " + file.toString()
                + " chunk table contains extra bytes at section " + sectionPrefix.toString());
        }

      } else if (sectionSize == TableSection.OFFSET_ARRAY_OFFSET + chunkCount * 4) {

        // old way has no Adler32 checksum for the chunk table
        numBytes = chunkCount * 4;

        // read table without Adler32 checksum bytes
        bytes = reader.readRaw(file, address, numBytes);

        // note that the old format with no checksum was encountered
        EWFFileReader.logger.info("EWFSection.ChunkTable: Note: File " + file.toString()
                + " does not use an offset array Adler32 checksum at section "
                + sectionPrefix.toString());

      } else {
        // too large for no Adler32 but too small for Adler32
        throw new EWFIOException("invalid table section size for chunk table", file, fileOffset);
      }
    }

    /**
     * Returns the chunk start offset at the given index
     * @param chunkTableIndex the index within this chunk table
     * @return the chunk start offset from the table
     */
    public long getChunkStartOffset(int chunkTableIndex) throws EWFIOException {
      // make sure the chunk index is valid
      if (chunkTableIndex >= chunkCount) {
        throw new EWFIOException("Invalid chunk index: " + chunkTableIndex, file, fileOffset);
      }

      // get the file offset to the media chunk base
      long chunkStartOffset = EWFSegmentFileReader.bytesToUInt(bytes, chunkTableIndex * 4);

      // strip out any MSB used to indicate compressed zlib encoding
      chunkStartOffset &= 0x7fffffff;

      return chunkStartOffset;
    }

    /**
     * Returns whether the given chunk is compressed
     * @param chunkTableIndex the index within this chunk table
     * @return true if the given chunk is compressed, false if not
     */
    public boolean isCompressedChunk(int chunkTableIndex) throws EWFIOException {
      // make sure the chunk index is valid
      if (chunkTableIndex >= chunkCount) {
        throw new EWFIOException("Invalid chunk index: " + chunkTableIndex, file, fileOffset);
      }
      
      // get the file offset to the media chunk base
      long chunkStartOffset = EWFSegmentFileReader.bytesToUInt(bytes, chunkTableIndex * 4);

      // determine if compression is used for the chunk
      boolean isCompressedChunk = (chunkStartOffset & 0x80000000) != 0;
      return isCompressedChunk;
    }
  }
}


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

/**
 * The <code>EWFSegmentFileReader</code> class provides accessors for reading EWF files formatted
 * in the .E01 format.
 */
public final class EWFIOException extends IOException {
  private static final long serialVersionUID = 1;

  // the file associated with the exceptioin
  private File file;

  // the address associated with the exception
  private long address;

  /**
   * Constructs an Exception event containing the file and address that sourced the exception.
   * @param file the file from which the exception originated
   * @param address the address from which the exception originated
   */
  protected EWFIOException(String message, File file, long address) {
    super(message);
    this.file = file;
    this.address = address;
  }

  /**
   * Returns the file associated with this exception.
   * @return the file associated with this exception
   */
  public File getFile() {
    return file;
  }

  /**
   * Returns the address associated with this exception.
   * @return the address associated with this exception
   */
  public long getAddress() {
    return address;
  }

  /**
   * Returns the exception message with the file and address appended
   * @return the exception message with the file and address appended
   */
  public String getMessage() {
    return super.getMessage() + " at file " + file.toString() + " file address "
              + String.format(EWFSegmentFileReader.longFormat, address);
  }
}


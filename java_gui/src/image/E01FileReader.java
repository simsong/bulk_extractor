import edu.nps.jlibewf.EWFFileReader;
import java.io.File;
import java.io.IOException;

/**
 * This <code>E01FileReader</code> glue class extends edu.nps.jlibewf.EWFFileReader
 * in order to implement ImageReader.
 */
public class E01FileReader extends EWFFileReader implements ImageFileReaderInterface {

  /**
   * Creates an image reader for reading a raw file.
   * If the reader fails then an error dialog is displayed
   * and an empty reader is returned in its place.
   * @param newFile the raw file
   * @throws IOException if the reader cannot be created
   */
  public E01FileReader(File newFile) throws IOException {
    super(newFile);
  }

  /**
   * Returns image properties specific to E01 files.
   * @return image properties specific to E01 files
   */
  public String getImageMetadata() throws IOException {
    return super.getImageProperties();
  }
}


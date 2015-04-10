import java.io.File;
import java.io.IOException;
import java.io.ByteArrayOutputStream;

public class ImageReaderThread extends Thread {
  // input values
  private File imageFile;
  private String forensicPath;
  private int numBytes;

  // output values
  public ImageReader.ImageReaderResponse response;
  public boolean isDone = false;
 
  // resources
  private final ImageModel imageModel;

  /**
   * The <code>ImageReaderThread</code> class performs reading from an image
   * into <code>ImageReaderResponse</code>.
   * imageModel.manageModelChanges is called when done, which consumes the response.
   */
  public ImageReaderThread(ImageModel imageModel,
                           File imageFile,
                           String forensicPath,
                           int numBytes) {

    this.imageModel = imageModel;
    this.imageFile = imageFile;
    this.forensicPath = forensicPath;
    this.numBytes = numBytes;
  }

  // run the image reader
  public void run() {

    // handle the read request
    try {
      // issue the read
      ImageReader reader = new ImageReader(imageFile);
      response = reader.read(forensicPath, numBytes);
      reader.close();

      // note if no bytes were returned
      if (response.bytes.length == 0) {
        WError.showMessageLater("No bytes were read from the image path, likely because the image file is not aviailable.", "No Data");
      }

    } catch (Exception e) {
      // on any failure: warn and clear values
      WError.showErrorLater("Unable to read the Image.\n"
                            + "file: '" + imageFile + "' forensic path: '" + forensicPath + "'",
                            "Error reading Image", e);

      response = new ImageReader.ImageReaderResponse(new byte[0], 0);
    }
    isDone = true;

    // update the image model
    imageModel.manageModelChanges();
  }
}


import java.io.File;
import java.io.IOException;
import java.io.ByteArrayOutputStream;

public class ImageReaderThread extends Thread {
  // input values
  private File imageFile;
  private String forensicPath;
  private int numBytes;

  // output values
  public byte[] bytes;
  public long imageSize;
  public boolean isDone = false;
 
  // resources
  private final ImageModel imageModel;
  private final ImageReaderManager imageReaderManager;

  /**
   * The <code>ImageReaderThread</code> class performs reading from an image.
   * During the read, the bytes, image size, and image metadata are obtained.
   * Output values are produced.  Up to numBytes bytes are read, see bytes.length.
   * imageModel.manageModelChanges is called when done, which consumes output values.
   */
  public ImageReaderThread(ImageModel imageModel,
                           ImageReaderManager imageReaderManager,
                           File imageFile,
                           String forensicPath,
                           int numBytes) {

    this.imageModel = imageModel;
    this.imageReaderManager = imageReaderManager;
    this.imageFile = imageFile;
    this.forensicPath = forensicPath;
    this.numBytes = numBytes;
  }

  // run the image reader
  public void run() {

    // handle the read request
    try {
      // read the requested bytes
      bytes = imageReaderManager.readImageBytes(
                           imageFile, forensicPath, numBytes);

      // read the image size
      imageSize = imageReaderManager.getImageSize(imageFile, forensicPath);

      long offset = ForensicPath.getOffset(forensicPath);

      // warn if an image file is not used
      if (imageFile == null) {
        WError.showMessageLater("An Image file is not available for this Feature.\n"
                  + "Please open a Report with an associated Image File to view Image content.",
                  "Image File not available");
      // warn if an image file is used but the start address is beyond the image
      } else if (offset > imageSize) {
        WError.showErrorLater("An attempt was made to read past the end of the Image.\n"
                  + "Please verify that the Image is correct for the selected Report File.\n"
                  + "Image offset: " + offset + ", Image size: " + imageSize + "\n"
                  + "Feature: " + FeatureLine.getSummaryString(featureLine),
                  "Error reading Image", null);
      }

    } catch (Exception e) {
      // on any failure: warn and clear values
      WError.showErrorLater("Unable to read the Image.\n"
                            + "file: " + imageFile + " forensic path: " + forensicPath,
                            "Error reading Image", e);

      bytes = new byte[0];
      imageSize = 0;
    }
    isDone = true;

    // update the image model
    imageModel.manageModelChanges();
  }
}


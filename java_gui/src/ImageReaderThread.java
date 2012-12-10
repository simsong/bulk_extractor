import java.io.IOException;
import java.io.ByteArrayOutputStream;

public class ImageReaderThread extends Thread {
  // input values
  private FeatureLine featureLine;
  private long startAddress;
  private int numBytes;

  // output values
  public byte[] bytes;
  public long imageSize;
  public String imageMetadata;
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
                           FeatureLine featureLine,
                           long startAddress,
                           int numBytes) {

    this.imageModel = imageModel;
    this.imageReaderManager = imageReaderManager;
    this.featureLine = featureLine;
    this.startAddress = startAddress;
    this.numBytes = numBytes;
  }

  // run the image reader
  public void run() {

    // handle a null FeatureLine or a feature line that is not an address or path
    if (featureLine == null
        || (featureLine.getType() != FeatureLine.FeatureLineType.ADDRESS_LINE
         && featureLine.getType() != FeatureLine.FeatureLineType.PATH_LINE)) {
      bytes = new byte[0];
      imageSize = 0;
      imageMetadata = "Image metatada is not loaded";
      isDone = true;
      // update the image model
      imageModel.manageModelChanges();
      return;
    }

    // handle featureLine
    try {
      // read the requested bytes
      bytes = imageReaderManager.readImageBytes(
                           featureLine, startAddress, numBytes);

      // read the image size
      imageSize = imageReaderManager.getImageSize(featureLine);

      // warn if an image file is not used
      if (featureLine.getImageFile() == null) {
        WError.showMessageLater("An Image file is not available for this Feature.\n"
                  + "Please open a Report with an associated Image File to view Image content.",
                  "Image File not available");
      // warn if an image file is used but the start address is beyond the image
      } else if (startAddress > imageSize) {
        WError.showErrorLater("An attempt was made to read past the end of the Image.\n"
                  + "Please verify that the Image is correct for the selected Report File.\n"
                  + "Image offset: " + startAddress + ", Image size: " + imageSize + "\n"
                  + "Feature: " + FeatureLine.getSummaryString(featureLine),
                  "Error reading Image", null);
      }

      // read the image metadata
      imageMetadata = imageReaderManager.getImageMetadata(featureLine);

    } catch (Exception e) {
      // on any failure: warn and clear values
      WError.showErrorLater("Unable to read the Image.\n"
                            + "Feature: " + FeatureLine.getSummaryString(featureLine),
                            "Error reading Image", e);

      bytes = new byte[0];
      imageSize = 0;
      imageMetadata = "Image metatada is not loaded";
    }
    isDone = true;

    // update the image model
    imageModel.manageModelChanges();
  }
}


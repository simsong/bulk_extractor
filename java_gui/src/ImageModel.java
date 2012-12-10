import java.util.Vector;
import java.util.Enumeration;
import java.io.File;
import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.util.Observer;
import java.util.Observable;
import javax.swing.SwingUtilities;

/**
 * The <code>ImageModel</code> class tracks image configuration parameters and fires
 * an image changed event when the model input changes.
 * Use <code>isBusy()</code> or a change listener to identify when the image read is completed.
 */

public class ImageModel {

  // feature attributes that define this model
  private FeatureLine featureLine = null;
  private long pageStartAddress = 0;

  // derived attributes that define this model
  private byte[] pageBytes = new byte[0];
  private long paddedPageStartAddress = 0;
  private int paddingPrefixSize = 0;
  private byte[] paddedPageBytes = new byte[0];
  private boolean[] pageHighlightFlags = new boolean[0];

  // values returned by image reader thread
  private long imageSize = 0;
  private String imageMetadata = "Please select a Feature to view the metadata associated with it.";

  // model state
  private boolean busy = false;
  private boolean featureAttributeChanged = false;

  // resources
  public final ImageReaderManager imageReaderManager = new ImageReaderManager();
  private final FeatureLineSelectionManager featureLineSelectionManager;
  private final WIndeterminateProgress busyIndicator = new WIndeterminateProgress("Reading Image");
  private final ModelChangedNotifier<Object> imageChangedNotifier = new ModelChangedNotifier<Object>();
  private ImageReaderThread imageReaderThread;

  /**
   * The default size of the page to be read, {@value}.
   */
  public static final int PAGE_SIZE = 4096;

  /**
   * Returns the aligned page start address for the given FeatureLine.
   * @param featureLine the feature line to obtain the aligned page start address for
   * @return the aligned page start address for the given FeatureLine
   */
  public static long getAlignedAddress(final FeatureLine featureLine) {
    if (featureLine == null
        || (featureLine.getType() != FeatureLine.FeatureLineType.ADDRESS_LINE
         && featureLine.getType() != FeatureLine.FeatureLineType.PATH_LINE)) {
      return 0;
    } else {
      // back up to modulo PAGE_SIZE in front of the feature line's address
      long address = featureLine.getAddress();
      return address - (address % ImageModel.PAGE_SIZE);
    }
  }

  /**
   * Returns the aligned page start address for the given address.
   * @param address the address to obtain the aligned page start address for
   * @return the aligned page start address for the given address
   */
  public static long getAlignedAddress(final long address) {
    // bad if negative input
    if (address < 0) {
      throw new IllegalArgumentException("Invalid address: " + address);
    }
    // back up to modulo PAGE_SIZE in front of the feature line's address
    return address - (address % ImageModel.PAGE_SIZE);
  }

  private final Runnable fireChanged = new Runnable() {
    public void run() {
      // fire signal that the model changed
      imageChangedNotifier.fireModelChanged(null);
    }
  };

  /**
   * Constructs an Image model and registers it to listen to feature line selection manager changes.
   */
  public ImageModel(FeatureLineSelectionManager _featureLineSelectionManager) {
    // resources
    this.featureLineSelectionManager = _featureLineSelectionManager;

    // feature line selection manager changed listener
    featureLineSelectionManager.addFeatureLineSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        setFeatureLine(featureLineSelectionManager.getFeatureLineSelection());
      }
    });
  }

  /**
   * Constructs an Image model that is not wired to other models, useful for generating Reports.
   */
  public ImageModel() {
    featureLineSelectionManager = null;
  }

  // ************************************************************
  // synchronized reader state control
  // ************************************************************
  /**
   * Sets the feature line and its default page start address and starts reading.
   */
  public synchronized void setFeatureLine(FeatureLine featureLine) {
    featureAttributeChanged = true;  // synchronized
    this.featureLine = featureLine;
    pageStartAddress = getAlignedAddress(featureLine);
    manageModelChanges();
  }

  /**
   * Sets the page start address and starts reading.
   */
  public synchronized void setPageStartAddress(long pageStartAddress) {
    featureAttributeChanged = true;  // synchronized
    // validate state
    if (featureLine == null || pageStartAddress < 0 || pageStartAddress > imageSize) {
      throw new RuntimeException("ImageModel.setPageStartAddress error");
//      WLog.log("ImageModel.setPageStartAddress error");
    }
    this.pageStartAddress = pageStartAddress;
    manageModelChanges();
  }

  /**
   * Force refresh of the Image Model.
   * Use this after <code>ImageReaderManager.setReaderTypeAllowed</code>
   * in order to force an ImageModel reload.
   */
  public synchronized void refresh() {
    // force an image reload, useful when the image reader changes
    featureAttributeChanged = true;  // synchronized
    manageModelChanges();
  }

  /**
   * Close the image reader associated with the image file.
   */
  public void closeImageReader(File imageFile) {
    imageReaderManager.closeReader(imageFile);
  }

  /**
   * Close all image readers.
   */
  public void closeAllImageReaders() {
    imageReaderManager.closeAllReaders();
  }

//  /**
//   * Set the reader type allowed in ImageModel's ImageReaderManager
//   */
//  public void setReaderTypeAllowed(ImageReaderType readerType) {
//    imageReaderManager.setReaderTypeAllowed(readerType);
//  }

  // ************************************************************
  // reader state polling
  // ************************************************************
  /**
   * Returns the feature line associated with the currently requested image.
   */
  public FeatureLine getFeatureLine() {
    return featureLine;
  }

  /**
   * Returns the page start address associated with the currently requested image.
   */
  public long getPageStartAddress() {
    return pageStartAddress;
  }

  // ************************************************************
  // polling and returned data
  // ************************************************************
  /**
   * Indicates whether the model is busy reading an image.
   * @return true if busy, false if not
   */
  public synchronized boolean isBusy() {
    return busy;
  }

  public static class ImagePage {
    public final FeatureLine featureLine;
    public final long pageStartAddress;
    public final byte[] pageBytes;
    public final byte[] paddedPageBytes;
    public final int paddingPrefixSize;
    public final int defaultPageSize;
    public final long imageSize;

    private ImagePage(FeatureLine featureLine,
                      long pageStartAddress, byte[] pageBytes, byte[] paddedPageBytes,
                      int paddingPrefixSize, int defaultPageSize, long imageSize) {
      this.featureLine = featureLine;
      this.pageStartAddress = pageStartAddress;
      this.pageBytes = pageBytes;;
      this.paddedPageBytes = paddedPageBytes;
      this.paddingPrefixSize = paddingPrefixSize;
      this.defaultPageSize = defaultPageSize;
      this.imageSize = imageSize;
    }
  }

  /**
   * Returns the Image Page structure associated with the currently active image.
   */
  public synchronized ImagePage getImagePage() {
    if (busy) {
      WLog.log("ImageModel.getImagePage: note: blank image page provided while busy.");
      return null;
    }
    return new ImagePage(featureLine, pageStartAddress, pageBytes, paddedPageBytes,
                         paddingPrefixSize, PAGE_SIZE, imageSize);
  }

  /**
   * Returns the image size associated with the currently active image.
   */
  public synchronized long getImageSize() {
    if (busy) {
      throw new RuntimeException("sync error");
    }
    return imageSize;
  }
  /**
   * Returns the metadata associated with the currently active image.
   */
  public synchronized String getImageMetadata() {
    if (busy) {
      throw new RuntimeException("sync error");
    }
    return imageMetadata;
  }

  // ************************************************************
  // set model inputs and start the image reader thread
  // ************************************************************
  /**
   * Called internally and by ImageReaderThread to reschedule or resolve a read request.
   */
  public synchronized void manageModelChanges() {
    // set busy and busy indicator
    busy = true;
    if (featureLine == null) {
      busyIndicator.startProgress("No Feature");
    } else {
      busyIndicator.startProgress(featureLine.getSummaryString());
    }
 
//WLog.log("ImageModel.manageModelChanges.a");
    if (imageReaderThread == null || imageReaderThread.isDone) {
//WLog.log("ImageModel.manageModelChanges.b");
      // no active thread
      if (featureAttributeChanged) {
//WLog.log("ImageModel.manageModelChanges.c");
        // thread processing is required to read feature attributes

        // signal model changed on the Swing thread in order to clear the image view until done
        SwingUtilities.invokeLater(fireChanged);

        // establish the padded page start address and the padding prefix size,
        // expecting up to PAGE_SIZE of padding
        paddedPageStartAddress = pageStartAddress - PAGE_SIZE;
        if (paddedPageStartAddress < 0) {
          // don't read before byte zero
          paddedPageStartAddress = 0;
        }
        if (paddedPageStartAddress > imageSize) {
          // even the padding is out of range
          paddedPageStartAddress = pageStartAddress;
        }
        paddingPrefixSize = (int)(pageStartAddress - paddedPageStartAddress);
          
        // begin thread processing
        imageReaderThread = new ImageReaderThread( this, imageReaderManager,
               featureLine, paddedPageStartAddress, paddingPrefixSize + PAGE_SIZE + PAGE_SIZE);
        imageReaderThread.start();
        featureAttributeChanged = false;
      } else {
//WLog.log("ImageModel.manageModelChanges.d");
        // thread processingintegrate the changes into the model
        if (imageReaderThread != null) {
          // bring in values from thread
          paddedPageBytes = imageReaderThread.bytes;
//WLog.log("ImageModel.manageModelChanges.e: " + imageReaderThread.bytes.length);
          imageSize = imageReaderThread.imageSize;
          imageMetadata = imageReaderThread.imageMetadata;
          imageReaderThread = null;
        }
        setPageBytes();
        busyIndicator.stopProgress();
        busy = false;

        // signal model changed on the Swing thread
        SwingUtilities.invokeLater(fireChanged);
      }

    } else {
      // A previously requested thread read is already active, so do nothing here.
      // imageReaderThread will call manageModelChanges to continue the flow.
      // No action.
    }
  }

  private byte[] getUTF16Bytes(byte[] utf8Bytes) {
    int utf8Length = utf8Bytes.length;
    if (utf8Length == 0) {
      return utf8Bytes;
    }
    byte[] utf16Bytes = new byte[utf8Length * 2 - 1];
    for (int i=0; i<utf8Length; i++) {
      utf16Bytes[i*2] = utf8Bytes[i];
    }
    return utf16Bytes;
  }

  private void setPageBytes() {
    // not all bytes may have been read, so determine what is available based on paddedPageBytes
    int availablePrefixSize = paddingPrefixSize;
    if (availablePrefixSize > paddedPageBytes.length) {
      // the prefix could not fully be read
      availablePrefixSize = paddedPageBytes.length;
    }
    int availablePageSize = paddedPageBytes.length - availablePrefixSize;
    if (availablePageSize > PAGE_SIZE) {
      // remove postfix padding
      availablePageSize = PAGE_SIZE;
    }

    // set pageBytes[] from paddedPageBytes[]
    ByteArrayOutputStream outStream = new ByteArrayOutputStream(availablePageSize);
    outStream.write(paddedPageBytes, availablePrefixSize, availablePageSize);
    pageBytes = outStream.toByteArray();
  }

  // ************************************************************
  // listener registry
  // ************************************************************
  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param imageChangedListener  the <code>Observer</code> to be added
   */
  public void addImageModelChangedListener(Observer imageChangedListener) {
    imageChangedNotifier.addObserver(imageChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param imageChangedListener  the <code>Observer</code> to be removed
   */
  public void removeImageModelChangedListener(Observer imageChangedListener) {
    imageChangedNotifier.deleteObserver(imageChangedListener);
  }
}


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
  private FeatureLine featureLine = new FeatureLine();
  private String pageForensicPath = "";

  // derived attributes that define this model
  private byte[] pageBytes = new byte[0];
  private long paddedPageOffset = 0;
  private int paddingPrefixSize = 0;
  private ImageReader.ImageReaderResponse response
                     = new ImageReader.ImageReaderResponse(new byte[0], 0);
  private byte[] paddedPageBytes = new byte[0];
  private boolean[] pageHighlightFlags = new boolean[0];

  // values returned by image reader thread
  private long imageSize = 0;

  // model state
  private boolean busy = false;
  private boolean imageSelectionChanged = false;

  // resources
  private final FeatureLineSelectionManager featureLineSelectionManager;
  private final WIndeterminateProgress busyIndicator = new WIndeterminateProgress("Reading Image");
  private final ModelChangedNotifier<Object> imageChangedNotifier = new ModelChangedNotifier<Object>();
  private ImageReaderThread imageReaderThread;

  /**
   * The default size of the page to be read, {@value}.
   */
  public static final int PAGE_SIZE = 65536;

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
        FeatureLine featureLine = featureLineSelectionManager.getFeatureLineSelection();

        // disregard request if this is a histogram line
        if (ForensicPath.isHistogram(featureLine.firstField)) {
          return;
        }

        setImageSelection(featureLine);
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
   * Sets the image selection image file and forensic path.
   */
  public synchronized void setImageSelection(FeatureLine featureLine) {
    imageSelectionChanged = true;  // synchronized
    this.featureLine = featureLine;
    this.pageForensicPath = ForensicPath.getAlignedPath(featureLine.forensicPath);
    manageModelChanges();
  }

  /**
   * Changes the forensic path to an inclusive aligned page value.
   */
  public synchronized void setImageSelection(String forensicPath) {
    imageSelectionChanged = true;  // synchronized
    this.pageForensicPath = ForensicPath.getAlignedPath(forensicPath);
    manageModelChanges();
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
    public final String pageForensicPath; // path used by model
    public final byte[] pageBytes;
    public final byte[] paddedPageBytes;
    public final int paddingPrefixSize;
    public final int defaultPageSize;
    public final long imageSize;

    private ImagePage(FeatureLine featureLine,
                      String pageForensicPath,
                      byte[] pageBytes, byte[] paddedPageBytes,
                      int paddingPrefixSize, int defaultPageSize,
                      long imageSize) {
      this.featureLine = featureLine;
      this.pageForensicPath = pageForensicPath;
      this.pageBytes = pageBytes;
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
      return new ImagePage(new FeatureLine(), "", new byte[0], new byte[0], 0, PAGE_SIZE, 0);
    }
    return new ImagePage(featureLine, pageForensicPath,
                         pageBytes, paddedPageBytes,
                         paddingPrefixSize, PAGE_SIZE, imageSize);
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

    // generate path
    // NOTE: should use ForensicPath.getPrintablePath() but isHex
    // is not visible to ImageModel.  Also, this is what is sent to the reader.
    // Either way, isHex should be moved into a class of its own to have
    // uniform visibility everywhere.
    String text = "Image file: " + FileTools.getAbsolutePath(featureLine.actualImageFile)
                + "\nFeature path: " + featureLine.forensicPath
                + "\nRequested read path: " + pageForensicPath;
    busyIndicator.startProgress(text);
 
    if (imageReaderThread == null || imageReaderThread.isDone) {
      // no active thread

      // for blank feature line, simply clear image reader thread and use now
      if (featureLine.isBlank()) {
        imageReaderThread = null;
        imageSelectionChanged = false;
      }

      // for change, schedule on thread
      if (imageSelectionChanged) {
        // thread processing is required to read feature attributes

        // signal model changed on the Swing thread in order to clear the image view until done
        SwingUtilities.invokeLater(fireChanged);

        // establish the padded page offset and the padding prefix size,
        // expecting up to PAGE_SIZE of padding
        long pageOffset = ForensicPath.getOffset(pageForensicPath);
        long paddedPageOffset = pageOffset - PAGE_SIZE;
        if (paddedPageOffset < 0) {
          // don't read before byte zero
          paddedPageOffset = 0;
        }
        if (paddedPageOffset > imageSize) {
          // even the padding is out of range
          paddedPageOffset = pageOffset;
        }
        paddingPrefixSize = (int)(pageOffset - paddedPageOffset);
        String paddedForensicPath = ForensicPath.getAdjustedPath(pageForensicPath, paddedPageOffset);
          
        // begin thread processing
        imageReaderThread = new ImageReaderThread( this,
               featureLine.actualImageFile, paddedForensicPath,
               paddingPrefixSize + PAGE_SIZE + PAGE_SIZE);
        imageReaderThread.start();
        imageSelectionChanged = false;
      } else {
        // integrate the changes into the model
        if (imageReaderThread != null) {
          // bring in values from thread
          ImageReader.ImageReaderResponse response = imageReaderThread.response;
          paddedPageBytes = response.bytes;
          setPageBytes();
          imageSize = response.totalSizeAtPath;
          imageReaderThread = null;
        } else {
          // got here without reader thread
          paddedPageBytes = new byte[0];
          pageBytes = new byte[0];
          imageSize = 0;
        }
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

  // this acts on class-local variables
  private void setPageBytes() {
    // return page bytes from within padded page bytes
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


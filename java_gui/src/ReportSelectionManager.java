import java.util.Observer;
import java.io.File;
import java.io.InputStream;
import java.io.FileInputStream;

/**
 * The <code>ReportSelectionManager</code> class provides notification when
 * a Report is selected.
 * It also records the user's features file view preference.
 */
public class ReportSelectionManager {

  // model state
  private ReportSelectionType activeReportSelectionType = ReportSelectionType.NONE;
  private File imageFile = null;
  private File featuresFile = null;
  private File histogramFile = null;
  private File referencedFeaturesFile = null;
  private ReportSelectionType reportSelectionType = ReportSelectionType.NONE;
  private ChangeType changeType = ChangeType.SELECTION_CHANGED;
  private boolean requestHideReferencedFeatureView = false;

  private final ModelChangedNotifier<Object> reportSelectionManagerChangedNotifier = new ModelChangedNotifier<Object>();

  /**
   * The <code>ReportSelectionType</code> class identifies the currently active selection type.
   */
  public static final class ReportSelectionType {
    public static final ReportSelectionType NONE = new ReportSelectionType("None");
//    public static final ReportSelectionType DIRECTORY = new ReportSelectionType("Directory");
    public static final ReportSelectionType HISTOGRAM = new ReportSelectionType("Histogram file");
    public static final ReportSelectionType FEATURES = new ReportSelectionType("Features file");
    private final String name;
    private ReportSelectionType(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }

  /**
   * The <code>ChangeType</code> class identifies the type of the change that was last requested.
   */
  public static final class ChangeType {
    public static final ChangeType SELECTION_CHANGED = new ChangeType("Selection changed");
    public static final ChangeType PREFERENCE_CHANGED = new ChangeType("User view preference changed");
    private final String name;
    private ChangeType(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }

  /**
   * Sets the Report selection and notifies listeners.
   * @param imageFile the file to reference as the image source
   * @param featuresFile the file to be used as the feature source
    */
  public void setReportSelection(final File imageFile, final File featuresFile) {
    this.imageFile = imageFile;
    if (featuresFile == null) {
      // no selection
      this.featuresFile = null;
      histogramFile = null;
      referencedFeaturesFile = null;
      reportSelectionType = ReportSelectionType.NONE;

    } else if (isHistogramFile(featuresFile)) {
      // histogram selection
      this.featuresFile = null;
      histogramFile = featuresFile;
      referencedFeaturesFile = getReferencedFile(histogramFile);
      reportSelectionType = ReportSelectionType.HISTOGRAM;
    } else {
      // featues selection
      this.featuresFile = featuresFile;
      histogramFile = null;
      referencedFeaturesFile = null;
      reportSelectionType = ReportSelectionType.FEATURES;
    }
    WLog.log("ReportSelectionManager.setReportSelection image file: " + imageFile + ", features file: " + featuresFile + ", selection type: " + reportSelectionType);

    // notify
    fire(ChangeType.SELECTION_CHANGED);
  }

  private void fire(ChangeType changeType) {
    this.changeType = changeType;
    reportSelectionManagerChangedNotifier.fireModelChanged(null);
  }

  /**
   * Returns the change type of the last change request, report or preference.
   */
  public ChangeType getChangeType() {
    return changeType;
  }

  /**
   * Returns the report selection type of the last report selection
   */
  public ReportSelectionType getReportSelectionType() {
    return reportSelectionType;
  }

  private boolean isHistogramFile(final File file) {
    // allow null
    if ((file == null) || !file.exists()) {
      return false;
    }

    // evaluate the file contents
    try {
      // open the potential histogram file
      InputStream inputStream = new FileInputStream(file);

      // strip off the UTF-8 Byte Order Mark (BOM)
      final byte[] utf8BOM = {(byte)0xef, (byte)0xbb, (byte)0xbf};
      byte[] bom = new byte[3];
      inputStream.read(bom);
      if (bom[0] == utf8BOM[0] && bom[1] == utf8BOM[1] && bom[2] == utf8BOM[2]) {
        // good, BOM matches, so leave stream pointing to start byte 3
      } else {
        // rewind and warn
        inputStream = new FileInputStream(file);
        WLog.log("ReportSelectionManager.isHistogramFile: Note: File "
                 + file + " does not contain a UTF-8 Byte Order Mark.");
      }

      int c;

      // find first line that is not a comment
      while (true) {
        c = inputStream.read();

        // strip off bytes until \n
        if (c == '#') {
          while (true) {
            c = inputStream.read();
            // EOF
            if (c == -1) {
              return false;
            }
            // \n
            if (c == '\n') {
              break;
            }
          }
        } else {
          break;
        }
      }

      // now, look for histogram indicator "n="
      if (c == 'n') {
        c = inputStream.read();
        if (c == '=') {
          return true;
        }
      }
      return false;
    } catch (Exception e) {
      WError.showError("Unable to evaluate Feature File type for file " + file, "Read error", e);
      return false;
    }
  }

  private File getReferencedFile(File histogramFile) {

    // get the histogram filename as a string
    String histogramFileString = histogramFile.getAbsolutePath();

    int lastUnderscorePosition = histogramFileString.lastIndexOf('_');

    // a valid histogram filename requires an underscore and to end with .txt
    if (lastUnderscorePosition < 0 || !histogramFileString.endsWith(".txt")) {
      WError.showError("Unexpected Histogram filename: " + histogramFileString, "Histogram Filename Error", null);
      return null;
    }

    // remove content after last "_" and add .txt to end
    String referencedFileString = histogramFileString.substring(0, lastUnderscorePosition) + ".txt";

    File referencedFile = new File(referencedFileString);
    if (referencedFile.exists()) {
      // great, return the refernced file
      return referencedFile;
    } else {
      // for example feature file email_domain_histogram.txt needs two
      // underscores stripped, so try again with this new referenced file name
      return getReferencedFile(referencedFile);
    }
  }

  /**
   * Sets whether the referenced feature view may be hidden and fires a changed event.
   * @param requestHideReferencedFeatureView wheather the referenced feature view may be hidden
   * when not used
   */
  public void setRequestHideReferencedFeatureView(boolean requestHideReferencedFeatureView) {
    if (this.requestHideReferencedFeatureView != requestHideReferencedFeatureView) {
      this.requestHideReferencedFeatureView = requestHideReferencedFeatureView;
      fire(ChangeType.PREFERENCE_CHANGED);
    }
  }

  /**
   * Indicates whether the referenced feature view may be hidden when not used.
   * @return true if the referenced feature view may be hidden when not used.
   */
  public boolean isRequestHideReferencedFeatureView() {
    return requestHideReferencedFeatureView;
  }

  /**
   * Returns the active Image file.
   */
  public File getImageFile() {
    return imageFile;
  }

  /**
   * Returns the active Features file.
   */
  public File getFeaturesFile() {
    return featuresFile;
  }

  /**
   * Returns the active histogram file.
   */
  public File getHistogramFile() {
    return histogramFile;
  }

  /**
   * Returns the active Referenced Features file.
   */
  public File getReferencedFeaturesFile() {
    return referencedFeaturesFile;
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param reportSelectionManagerChangedListener the <code>Observer</code> to be added
   */
  public void addReportSelectionManagerChangedListener(Observer reportSelectionManagerChangedListener) {
    reportSelectionManagerChangedNotifier.addObserver(reportSelectionManagerChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param reportSelectionManagerChangedListener the <code>Observer</code> to be removed
   */
  public void removeReportSelectionManagerChangedListener(Observer reportSelectionManagerChangedListener) {
    reportSelectionManagerChangedNotifier.deleteObserver(reportSelectionManagerChangedListener);
  }

}


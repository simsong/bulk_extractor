import java.util.Observer;
import java.util.HashMap;
import java.util.Observable;
import java.util.Observer;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import javax.swing.SwingUtilities;

/**
 * The <code>FeaturesModel</code> class provides a configured listing of features.
 * The features listing content depends on the feature file selected,
 * the filter used, case sensitivity, and whether hex is used in the path format.
 * The image file is recorded because the features file is associated with it.
 * <p>For optimization, the features listing is generated on a separate dispatch thread.
 * A change signal is thrown when the listing is ready.
 * <p>There is no action when there is no change, tested by object equality.
 */
public class FeaturesModel implements CopyableLineInterface {

  // private Model properties
  private FeaturesParserThread featuresParserThread;
  private final ModelType modelType; // features and histogram or else referenced features
  private ModelRole modelRole = ModelRole.FEATURES_ROLE; // serving feature or histogram entries
  private final ReportSelectionManager reportSelectionManager;
  private final ReportSelectionManagerChangedListener reportSelectionManagerChangedListener
                      = new ReportSelectionManagerChangedListener();

  // model calculation state

  // feature values
  private File imageFile = null;
  private File featuresFile = null;
  private byte[] filterBytes = new byte[0];
  private boolean filterMatchCase = false;	// only false is allowed for Referenced Features
  private boolean useHexPath = false;
  private int fontSize = 12;

  // the feature line table is cleared during computation and set upon completion
  private FeatureLineTable featureLineTable = new FeatureLineTable(null, null);

  // resources
  public WProgress progressBar = new WProgress("Reading");
  private final ModelChangedNotifier<Object> featuresModelChangedNotifier = new ModelChangedNotifier<Object>();

  // this output state allows listeners to know the type of the last change
  private ChangeType changeType = ChangeType.FEATURES_CHANGED; // indicates fullest change

  /**
   * The <code>ChangeType</code> class identifies the type of the change that was last requested.
   */
  public static final class ChangeType {
    public static final ChangeType FEATURES_CHANGED = new ChangeType("Feature list changed");
    public static final ChangeType FORMAT_CHANGED = new ChangeType("Path format or font changed");
    private final String name;
    private ChangeType(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }

  /**
   * The <code>ModelType</code> class identifies which instance the features model is.
   */
  public static final class ModelType {
    public static final ModelType FEATURES_OR_HISTOGRAM = new ModelType("Features or histogram, upper window");
    public static final ModelType REFERENCED_FEATURES = new ModelType("Referenced featues, lower window");
    private final String name;
    private ModelType(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }

  /**
   * The <code>ModelRole</code> class identifies whether the model serves histogram or feature data
   */
  public static final class ModelRole {
    public static final ModelRole FEATURES_ROLE = new ModelRole("Features role");
    public static final ModelRole HISTOGRAM_ROLE = new ModelRole("Histogram role");
    private final String name;
    private ModelRole(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }

  /**
   * Creates FeaturesModel with a role.
   */
  FeaturesModel(ReportSelectionManager reportSelectionManager, ModelType modelType) {
    this.reportSelectionManager = reportSelectionManager;
//    reportSelectionManager.addReportSelectionManagerChangedListener(ReportSelectionManagerChangedListener);
    reportSelectionManager.addReportSelectionManagerChangedListener(reportSelectionManagerChangedListener);
    this.modelType = modelType;
  }

  private final class ReportSelectionManagerChangedListener implements Observer {
    public void update(Observable o, Object arg) {

      // do not respond to user view preference change
      if (reportSelectionManager.getChangeType() == ReportSelectionManager.ChangeType.PREFERENCE_CHANGED) {
        return;
      }

      // set the image file
      imageFile = reportSelectionManager.getImageFile();

      // set the features file and model role based on the features model type
      // and the report selection type
      if (modelType == ModelType.FEATURES_OR_HISTOGRAM) {
        // this model shows feature or histogram entries
        if (reportSelectionManager.getReportSelectionType() == ReportSelectionManager.ReportSelectionType.HISTOGRAM) {
          // set role and features file to histogram
          modelRole = ModelRole.HISTOGRAM_ROLE;
          featuresFile = reportSelectionManager.getHistogramFile();
        } else {
          // set role and features file to features
          modelRole = ModelRole.FEATURES_ROLE;
          featuresFile = reportSelectionManager.getFeaturesFile();
        }

        // set the report
        runFeaturesParserThread();

      } else if (modelType == ModelType.REFERENCED_FEATURES) {
        // this model shows referenced features
        modelRole = ModelRole.FEATURES_ROLE;

        // use features file
        featuresFile = reportSelectionManager.getReferencedFeaturesFile();

        // clear any features filtering
        filterBytes = new byte[0];

        // set the report
        runFeaturesParserThread();

      } else {
        throw new RuntimeException("Invalid type");
      }
    }
  }

  /**
   * Returns the model type: FEATURES_OR_HISTOGRAM or REFERENCED_FEATURES.
   */
  public ModelType getModelType() {
    return modelType;
  }

  /**
   * Returns the model role: FEATURES_ROLE or HISTOGRAM_ROLE.
   */
  public ModelRole getModelRole() {
    return modelRole;
  }

  /**
   * Sets the requested fields then begins recalculating the feature lines.
   * @param imageFile the file to reference as the image source
   * @param featuresFile the file to be used as the feature source
   */
  public void setReport(File imageFile, File featuresFile, ModelRole modelRole) {
  WLog.log("FeaturesModel.setReport image file: " + imageFile + ", featuresFile: " + featuresFile);
    // ignore if object equality
    if (imageFile == this.imageFile && featuresFile == this.featuresFile) {
      return;
    }

    this.modelRole = modelRole;
    this.imageFile = imageFile;
    this.featuresFile = featuresFile;
//WLog.log("FeaturesModel.setReport");
    runFeaturesParserThread();
  }

  /**
   * Sets the requested field then begins recalculating the feature lines.
   * @param filterBytes the filter text to filter feature entries
   */
  public void setFilterBytes(final byte[] filterBytes) {
    // no change
    if (UTF8Tools.bytesMatch(filterBytes, this.filterBytes)) {
      return;
    }

    // change
    this.filterBytes = filterBytes;
    runFeaturesParserThread();
  }

  /**
   * Sets the requested field then begins recalculating the feature lines.
   * @param filterMatchCase whether to ignore case sensitivity when filtering
   */
  public void setFilterMatchCase(final boolean filterMatchCase) {
    // ignore if same
    if (filterMatchCase == this.filterMatchCase) {
      return;
    }

    // only false is allowed for Referenced Features
    if (modelType == ModelType.REFERENCED_FEATURES) {
      throw new RuntimeException("Request not allowed for Referenced Features");
    }

    this.filterMatchCase = filterMatchCase;

    runFeaturesParserThread();
  }

  /**
   * Returns the filter string associated with the features model
   * used to generate this filter set.
   * @return the filter string associated with the features model
   * used to generate this filter set
   */
  public byte[] getFilterBytes() {
    return filterBytes;
  }

  /**
   * Returns the filter case sensitivity used to generate this filter set.
   * @return true if case is ignored, false if case is significant
   */
  public boolean isFilterMatchCase() {
    return filterMatchCase;
  }

  /**
   * Sets the path format associated with the view
   * @param useHexPath whether path is formatted in hex
   */
  public void setUseHexPath(boolean useHexPath) {
//no; featureline takes this as an input parameter   if (modelRole == ModelRole.HISTOGRAM_ROLE) throw new RuntimeException("Invalid Usage");

    // ignore if object equality
    if (useHexPath == this.useHexPath) {
      return;
    }

    this.useHexPath = useHexPath;
    changeType = ChangeType.FORMAT_CHANGED;
    featuresModelChangedNotifier.fireModelChanged(null);
  }

  /**
   * Returns the path format associated with the features model.
   * @return the path format associated with the features model
   */
  public boolean getUseHexPath() {
//no; featureline takes this as an input parameter    if (modelRole == ModelRole.HISTOGRAM_ROLE) throw new RuntimeException("Invalid Usage");

    return useHexPath;
  }

  /**
   * Sets font size.
   */
  public void setFontSize(int fontSize) {
    // ignore if object equality
    if (fontSize == this.fontSize) {
      return;
    }

    this.fontSize = fontSize;
//WLog.log("FeaturesModel.setFontSize");
    // signal model changed
    changeType = ChangeType.FORMAT_CHANGED;
    featuresModelChangedNotifier.fireModelChanged(null);
  }

  /**
   * Returns the font size.
   */
  public int getFontSize() {
    return fontSize;
  }

  /**
   * Returns the image file associated with the features model.
   * @return the image file associated with the features model
   */
  public File getImageFile() {
    return imageFile;
  }

  /**
   * Returns the features file associated with the features model.
   * @return the features file associated with the features model
   */
  public File getFeaturesFile() {
    return featuresFile;
  }

  /**
   * Returns the change type that resulted in the currently active state.
   */
  public FeaturesModel.ChangeType getChangeType() {
    return changeType;
  }

  /**
   * Returns the number of active feature lines.
   * The number of active feature lines may be less than the number of features
   * if a filter is active.
   * @return the total number of feature lines
   * or the total number of filtered feature lines, if filtered
   */
  public int getTotalLines() {
    return featureLineTable.size();
  }

  /**
   * Returns the number of bytes of the widest line,
   * which is typically wider than the actual feature.
   * @return the width, in bytes, of the widest line
   */
  public int getWidestLineLength() {
    return featureLineTable.getWidestLineLength();
  }

  /**
   * Returns the feature line corresponding to the given line number.
   * @param lineNumber the line number of the feature line to be returned
   * @return the feature line
   */
  public FeatureLine getFeatureLine(int lineNumber) {
    FeatureLine featureLine = featureLineTable.get(lineNumber);
    return featureLine;
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param featuresModelChangedListener the <code>Observer</code> to be added
   */
  public void addFeaturesModelChangedListener(Observer featuresModelChangedListener) {
    featuresModelChangedNotifier.addObserver(featuresModelChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param featuresModelChangedListener the <code>Observer</code> to be removed
   */
  public void removeFeaturesModelChangedListener(Observer featuresModelChangedListener) {
    featuresModelChangedNotifier.deleteObserver(featuresModelChangedListener);
  }


  /**
   * Starts the process that, upon successful completion,
   * sets the completion flag and the feature line table.
   * Features are cleared when the process starts and feature listeners are notified.
   * Upon completion, the completion flag is set and feature listeners are notified.
   *
   * <p>To avoid stalling the AWT event dispatching thread,
   * parsing is performed on a separate parser dispatch thread.
   * If this function is called while a previous run is still active,
   * the running parser dispatch thread is terminated and a new thread is started.
   * Upon completion, the new filter results are accepted
   * by code run on the AWT event dispatching thread.
   * When the parsing takes time to run, a visual progress bar is displayed.
   */
  private void runFeaturesParserThread() {

    WLog.log("FeaturesModel.runFeaturesParserThread: " + this + " imageFile: " + imageFile
              + ", featuresFile: " + featuresFile
              + ", filterBytes as string: " + new String(filterBytes)
              + ", filterMatchCase: " + filterMatchCase);

    // validate input
    if (filterBytes == null) {
      throw new NullPointerException();
    }

    // stop the existing parser thread
    if (featuresParserThread != null) {

      // NOTE: using featuresParserThread.interrupt causes
      // java.nio.channels.ClosedByInterruptExceptions.
      // Instead, we manage our own abort request flag.
      featuresParserThread.setAbortRequest();
      try {
        // wait for the thread to die
        featuresParserThread.join();
      } catch (InterruptedException e) {
          WError.showError("Parser Thread error.", "BEViewer Thread error", e);
      }
    }

    // clear old lines immediately so the view is clean while the new lines are being prepared
    featureLineTable = new FeatureLineTable(null, null);
    changeType = ChangeType.FEATURES_CHANGED;
    featuresModelChangedNotifier.fireModelChanged(null);

    // start the parser thread
    featuresParserThread = new FeaturesParserThread(this, imageFile, featuresFile,
                               filterBytes, filterMatchCase, useHexPath);
    featuresParserThread.start();
  }

  /**
   * The FeaturesParserThread invokes this on the Swing thread
   * after the FeatureLineTable has been prepared.
   */
  public void setFeatureLineTable(FeatureLineTable newFeatureLineTable) {

    // set the new feature line attributes
    featureLineTable = newFeatureLineTable;

    // signal model changed
    changeType = ChangeType.FEATURES_CHANGED;
    featuresModelChangedNotifier.fireModelChanged(null);
  }

  /**
   * Implements CopyableLineInterface to provide a copyable line as a String
   */
  public String getCopyableLine(int line) {
    FeatureLine featureLine = getFeatureLine(line);
    return ForensicPath.getPrintablePath(featureLine.forensicPath, useHexPath)
         + "\t"
         + featureLine.formattedFeature;
  }
}


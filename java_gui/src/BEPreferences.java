import java.util.prefs.Preferences;
import java.io.ByteArrayOutputStream;
import java.util.Enumeration;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

/**
 * The <code>BEViewer</code> class provides the main entry
 * for the Bulk Extractor Viewer application.
 */
public class BEPreferences {
  // root preferences node
  private static final Preferences preferences = Preferences.userNodeForPackage(BEViewer.class);

  // Defaults
  /**
   * Default show toolbar, {@value}.
   */
  public static final boolean DEFAULT_SHOW_TOOLBAR = true;
  /**
   * Default font size for Feature listing, {@value}.
   */
  public static final int DEFAULT_FEATURE_FONT_SIZE = 12;
  /**
   * Default font size for Image listing, {@value}.
   */
  public static final int DEFAULT_IMAGE_FONT_SIZE = 12;
  /**
   * Default forensic path numeric base hex, {@value}.
   */
  public static final boolean DEFAULT_FORENSIC_PATH_NUMERIC_BASE_HEX = false;
  /**
   * Default format for presenting Image lines, {@value}.
   */
  public static final String DEFAULT_IMAGE_LINE_FORMAT = "Text";
  /**
   * Default request for hiding the referenced feature view, {@value}.
   */
  public static final boolean DEFAULT_REQUEST_HIDE_REFERENCED_FEATURE_VIEW = false;
  /**
   * Default to show stoplist files, {@value}.
   */
  public static final boolean DEFAULT_SHOW_STOPLIST_FILES = false;
  /**
   * Default to show empty files, {@value}.
   */
  public static final boolean DEFAULT_SHOW_EMPTY_FILES = false;

  private BEPreferences() {
  }

  /**
   * clears all preferences
   */
  public static void clearPreferences() {
    try {
      preferences.removeNode();
//      preferences.flush();
      WLog.log("BEPreferences.clearPreferences: preferences cleared.");
    } catch (Exception e) {
      // report error but do not rethrow error
      WError.showError("Failure while clearing user preferences.", "BEViewer Preferences Error", e);
    }
  }

  /**
   * used only during validation testing.
   */
  public static String getPreferencesString() throws Exception {
    ByteArrayOutputStream preferencesStream = new ByteArrayOutputStream();
    preferences.exportSubtree(preferencesStream);
    return preferencesStream.toString();
  }

  // load user preferences saved locally from last run, if available, else load defaults
  public static void loadPreferences() {

    // load preference to show the toolbar
    boolean showToolbar = preferences.getBoolean("show_toolbar", DEFAULT_SHOW_TOOLBAR);
    BEViewer.toolbar.setVisible(showToolbar);

    // load feature font size
    int featureFontSize = preferences.getInt("feature_font_size", DEFAULT_FEATURE_FONT_SIZE);
    BEViewer.featuresModel.setFontSize(featureFontSize);
    BEViewer.referencedFeaturesModel.setFontSize(featureFontSize);

    // load image file font size
    int imageFontSize = preferences.getInt("image_font_size", DEFAULT_IMAGE_FONT_SIZE);
    BEViewer.imageView.setFontSize(imageFontSize);

    // load forensic path numeric base
    boolean forensicPathNumericBaseHex = preferences.getBoolean("forensic_path_numeric_base_hex", DEFAULT_FORENSIC_PATH_NUMERIC_BASE_HEX);
    BEViewer.featuresModel.setUseHexPath(forensicPathNumericBaseHex);
    BEViewer.imageView.setUseHexPath(forensicPathNumericBaseHex);

    // load the image line format, note that ImageLine.LineFormat establishes the default
    String imageLineFormat = preferences.get("image_line_format", DEFAULT_IMAGE_LINE_FORMAT);
    BEViewer.imageView.setLineFormat(ImageLine.LineFormat.lookup(imageLineFormat));

    // load preference to hide the referenced feature view
    boolean requestHide = preferences.getBoolean("request_hide_referenced_feature_view", DEFAULT_REQUEST_HIDE_REFERENCED_FEATURE_VIEW);
    BEViewer.reportSelectionManager.setRequestHideReferencedFeatureView(requestHide);

    // load preference to show stoplist files
    boolean requestShowStoplistFiles = preferences.getBoolean("show_stoplist_files", DEFAULT_SHOW_STOPLIST_FILES);
    BEViewer.reportsModel.setIncludeStoplistFiles(requestShowStoplistFiles);

    // load preference to show empty files
    boolean requestShowEmptyFiles = preferences.getBoolean("show_empty_files", DEFAULT_SHOW_EMPTY_FILES);
    BEViewer.reportsModel.setIncludeEmptyFiles(requestShowEmptyFiles);

    // load test settings
    WTest.testWorkSettingsFileString = preferences.get("test_work_settings_file", "");
    WTest.testOutputDirectoryString = preferences.get("test_output_directory", "");
    WTest.testScanDirective = preferences.get("test_scan_directive", "bulk_extractor,");

    // load saved reports
    loadSavedReports();

    // load saved bookmarks
    loadSavedBookmarks();
  }

  // load saved reports
  private static void loadSavedReports() {

    Preferences reportsPreferences = preferences.node("saved_reports");
    int i = 0;
    while (true) {
      // generate the preferences variable names
      String reportIndex = Integer.toString(i);
      String featuresDirectoryString = reportsPreferences.get("feature_directory_" + reportIndex, null);
      File featuresDirectory = (featuresDirectoryString == null) ? null : new File(featuresDirectoryString);
      String reportImageFileString = reportsPreferences.get("image_file_" + reportIndex, null);
      File reportImageFile = (reportImageFileString == null) ? null : new File(reportImageFileString);

      // stop loading reports when there are no more saved values
      if (featuresDirectory == null) {
        break;
      }
      if (reportImageFile == null) {
        WLog.log("BEPreferences: loadSavedReports: unexpected null reportImageFile");
        break;
      }

      // load the saved report into the model
      BEViewer.reportsModel.addReport(featuresDirectory, reportImageFile);

      // move to the next index
      i++;
    }
  }

  // load saved bookmarks
  private static void loadSavedBookmarks() {
  Preferences bookmarksPreferences = preferences.node("saved_bookmarks");
    int i = 0;
    while (true) {
      // generate the preferences variable names
      String bookmarkIndex = Integer.toString(i);
      String reportImageFileString = bookmarksPreferences.get("image_file_" + bookmarkIndex, null);
      File reportImageFile = (reportImageFileString == null) ? null : new File(reportImageFileString);
      String featuresFileString = bookmarksPreferences.get("features_file_" + bookmarkIndex, null);
      File featuresFile = (reportImageFileString == null) ? null : new File(featuresFileString);
      long startByte = bookmarksPreferences.getLong("start_byte_" + bookmarkIndex, 0);
      int numBytes = bookmarksPreferences.getInt("num_bytes_" + bookmarkIndex, 0);

      // stop loading bookmarks when there are no more saved values
//      if (reportImageFile == null) {
//        break;
//      }
      if (featuresFile == null) {
//        WLog.log("BEPreferences: loadSavedBookmarks: unexpected null featuresFile");
        break;
      }

      // warn if the image file does not exist
      if (reportImageFile != null && !reportImageFile.exists()) {
        WError.showError("The image file for the Feature being restored is not available:"
                         + "\nImage file: " + reportImageFileString
                         + "\nFeature file: " + featuresFileString
                         + "\nFeature index: " + startByte,
                         "BEViewer Preferences Error", null);
      }

      // warn and skip if the feature file does not exist
      if (!featuresFile.exists()) {
        WError.showError("Unable to restore feature from saved preferences because the Feature file is not available:"
                         + "\nImage file: " + reportImageFileString
                         + "\nFeature file: " + featuresFileString
                         + "\nFeature index: " + startByte,
                         "BEViewer Preferences Error", null);
        break;
      }

      // create the feature from the saved values
      try {
        FeatureLine featureLine = new FeatureLine(reportImageFile, featuresFile, startByte, numBytes);

        // load the saved feature line bookmark into the Feature bookmarks model
        BEViewer.bookmarksModel.addElement(featureLine);
      } catch (Exception e) {
        WError.showError("Unable to restore feature from saved preferences:"
                         + "\nImage file: " + reportImageFileString
                         + "\nFeature file: " + featuresFileString
                         + "\nFeature index: " + startByte,
                         "BEViewer Preferences Error", e);
      }
      // move to the next index
      i++;
    }
  }

  // save user preferences set during run, then quit
  public static void savePreferences() {
    try {

      // preference to show the toolbar
      preferences.putBoolean("show_toolbar", BEViewer.toolbar.isVisible());

      // feature font size
      preferences.putInt("feature_font_size", BEViewer.featuresModel.getFontSize());

      // image file font size
      preferences.putInt("image_font_size", BEViewer.imageView.getFontSize());

      // forensic path numeric base, the same for featuresModel and imageView
      preferences.putBoolean("forensic_path_numeric_base_hex", BEViewer.featuresModel.getUseHexPath());

      // image line format
      preferences.put("image_line_format", BEViewer.imageView.getLineFormat().toString());

      // hide referenced feature view
      preferences.putBoolean("request_hide_referenced_feature_view",
                      BEViewer.reportSelectionManager.isRequestHideReferencedFeatureView());

      // preference to show stoplist files
      preferences.putBoolean("show_stoplist_files", BEViewer.reportsModel.isIncludeStoplistFiles());

      // preference to show empty files
      preferences.putBoolean("show_empty_files", BEViewer.reportsModel.isIncludeEmptyFiles());

      // save test settings
      preferences.put("test_work_settings_file", WTest.testWorkSettingsFileString);
      preferences.put("test_output_directory", WTest.testOutputDirectoryString);
      preferences.put("test_scan_directive", WTest.testScanDirective);

      // save reports
      saveReports();

      // save bookmarks
      saveBookmarks();

    } catch (Exception e) {
      // report error but do not rethrow error
      WError.showError("Unable to save user preferences.", "BEViewer Preferences Error", e);
    }
  }

  // save reports
  private static void saveReports() throws Exception {

    // clear and recreate reports node
    Preferences reportsPreferences = preferences.node("saved_reports");
    reportsPreferences.clear();

    // get reports to save
    Enumeration<ReportsModel.ReportTreeNode> e = BEViewer.reportsModel.elements();
    int i=0;
    while (e.hasMoreElements()) {
      ReportsModel.ReportTreeNode reportTreeNode = e.nextElement();

      // get the File strings
      String featuresDirectoryString = FileTools.getAbsolutePath(reportTreeNode.featuresDirectory);
      String reportImageFileString = FileTools.getAbsolutePath(reportTreeNode.reportImageFile);

      // save the indexed feature directory and image file preferences
      String reportIndex = Integer.toString(i);
      reportsPreferences.put("feature_directory_" + reportIndex, featuresDirectoryString);
      reportsPreferences.put("image_file_" + reportIndex, reportImageFileString);

      i++;
    }
  }

  // save bookmarks
  private static void saveBookmarks() throws Exception {

    // clear and recreate bookmarks node
    Preferences bookmarksPreferences = preferences.node("saved_bookmarks");
    bookmarksPreferences.clear();

    // get number of bookmarks to save
    int size = BEViewer.bookmarksModel.size();

    // save the bookmarks
    for (int i=0; i<size; i++) {
      // generate the preferences variable names
      FeatureLine featureLine = BEViewer.bookmarksModel.get(i);
      String reportImageFileString = FileTools.getAbsolutePath(featureLine.reportImageFile);
      String featuresFileString = FileTools.getAbsolutePath(featureLine.featuresFile);
      long startByte = featureLine.startByte;
      int numBytes = featureLine.numBytes;

      // save the indexed feature line bookmark preferences
      String reportIndex = Integer.toString(i);
      bookmarksPreferences.put("image_file_" + reportIndex, reportImageFileString);
      bookmarksPreferences.put("features_file_" + reportIndex, featuresFileString);
      bookmarksPreferences.putLong("start_byte_" + reportIndex, startByte);
      bookmarksPreferences.putInt("num_bytes_" + reportIndex, numBytes);
    }
  }

  /**
   * Exports work settings in XML format to the given filename.
   * @param workSettingsFile the file to export preferences to
   * @return true if successful, false if failed
   */
  public static boolean exportWorkSettings(File workSettingsFile) {
    try {

      // create the DOM doc object
      DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();
      DocumentBuilder builder = builderFactory.newDocumentBuilder();
      Document doc = builder.newDocument();

      // create the root dfxml element
      Element root = doc.createElement("dfxml");
      root.setAttribute("xmloutputversion", "1.0");
      doc.appendChild(root);

      // create the work settings element
      Element workSettings = doc.createElement("work_settings");
      root.appendChild(workSettings);

      // fill the Report elements
      Enumeration<ReportsModel.ReportTreeNode> e = BEViewer.reportsModel.elements();
      while (e.hasMoreElements()) {
        ReportsModel.ReportTreeNode reportTreeNode = e.nextElement();

        // get the File strings
        String featuresDirectoryString = FileTools.getAbsolutePath(reportTreeNode.featuresDirectory);
        String reportImageFileString = FileTools.getAbsolutePath(reportTreeNode.reportImageFile);

        // create the Report element
        Element report = doc.createElement("report");

        // set the feature file and image file strings
        report.setAttribute("feature_directory", featuresDirectoryString);
        report.setAttribute("image_file", reportImageFileString);

        // add the Report element to the work settings element
        workSettings.appendChild(report);
      }

      // fill the bookmark elements
      int numBookmarks = BEViewer.bookmarksModel.size();
      for (int i=0; i<numBookmarks; i++) {

        // get the requisite bookmark attributes
        FeatureLine featureLine = BEViewer.bookmarksModel.get(i);
        String reportImageFileString = FileTools.getAbsolutePath(featureLine.reportImageFile);
        String featuresFileString = FileTools.getAbsolutePath(featureLine.featuresFile);
        String startByteString = Long.toString(featureLine.startByte);
        String numBytesString = Integer.toString(featureLine.numBytes);

        // create the Bookmark element
        Element bookmark = doc.createElement("bookmark");

        // set the bookmark attributes
        bookmark.setAttribute("image_file", reportImageFileString);
        bookmark.setAttribute("feature_file", featuresFileString);
        bookmark.setAttribute("start_byte", startByteString);
        bookmark.setAttribute("num_bytes", numBytesString);

        // add the Bookmark element to the work settings element
        workSettings.appendChild(bookmark);
      }

      // create transformer for converting DOM doc source to XML-formatted StreamResult object
      TransformerFactory transformerFactory = TransformerFactory.newInstance();
      Transformer transformer = transformerFactory.newTransformer();
      transformer.setOutputProperty(OutputKeys.INDENT, "yes");
// NOTE: Transformer disregards this request.      transformer.setOutputProperty(OutputKeys.STANDALONE, "yes");

      // create transformer source from doc
      DOMSource domSource = new DOMSource(doc);

      // create transformer destination to work settings file
      FileOutputStream fileOutputStream = new FileOutputStream(workSettingsFile);
      StreamResult streamResult = new StreamResult(fileOutputStream);

      // transform DOM doc to XML-formatted StreamResult object
      transformer.transform(domSource, streamResult);

      // close the file output stream
      fileOutputStream.flush();
      fileOutputStream.close();

      return true;

    } catch (Exception e) {
      WError.showError("Unable to export work settings to file " + workSettingsFile + ".",
                       "BEViewer file error", e);
    }
    return false;
  }

  /**
   * Imports work settings in XML format from the given filename.
   * @param workSettingsFile the file to import preferences from
   * @param keepExistingWorkSettings true to keep settings, false to clear navigation history,
   * reports, readers, and the bookmarks list
   * @return true if successful, false if failed
   */
  public static boolean importWorkSettings(File workSettingsFile, boolean keepExistingWorkSettings) {
    try {
      int i;
      // read workSettingsFile into Document
      DocumentBuilderFactory documentBuilderFactory = DocumentBuilderFactory.newInstance();
      DocumentBuilder documentBuilder = documentBuilderFactory.newDocumentBuilder();
      Document document = documentBuilder.parse(workSettingsFile);

      // drill down to work settings element
      Element root = document.getDocumentElement();
      Element workSettings = (Element)(root.getElementsByTagName("work_settings").item(0));

      if (!keepExistingWorkSettings) {
        // dump all the old work settings
        BEViewer.closeAllReports();
      }

      // load the new work settings
      // load Reports
      NodeList reportList = workSettings.getElementsByTagName("report");
      for (i=0; i<reportList.getLength(); i++) {

        // get the report files
        Element report = (Element)reportList.item(i);
        File featuresDirectory = new File(report.getAttribute("feature_directory"));
        File reportImageFile = new File(report.getAttribute("image_file"));

        // load the report into the model
        BEViewer.reportsModel.addReport(featuresDirectory, reportImageFile);
      }

      // load Bookmarks
      NodeList bookmarkList = workSettings.getElementsByTagName("bookmark");
      for (i=0; i<bookmarkList.getLength(); i++) {

        // get the bookmark attributes
        Element bookmark = (Element)bookmarkList.item(i);
        File reportImageFile = new File(bookmark.getAttribute("image_file"));
        File featuresFile = new File(bookmark.getAttribute("feature_file"));
        long startByte = Long.parseLong(bookmark.getAttribute("start_byte"));
        int numBytes = Integer.parseInt(bookmark.getAttribute("num_bytes"));

        // load the bookmark into the model
        // create the feature from the saved values
        try {
          FeatureLine featureLine = new FeatureLine(reportImageFile, featuresFile, startByte, numBytes);

          // load the saved feature line bookmark into the Feature bookmarks model
          BEViewer.bookmarksModel.addElement(featureLine);
        } catch (Exception e) {
          WError.showError("Unable to restore work settings feature:"
                           + "\nReport Image file: " + reportImageFile
                           + "\nFeature file: " + featuresFile
                           + "\nFeature index: " + startByte,
                           "BEViewer Preferences Error", e);
        }
      }

      // the import took place
      return true;

    } catch (Exception e) {
      WError.showError("Unable to load work settings from file " + workSettingsFile + ".",
                       "BEViewer file error", e);
    }
    return false;
  }
}


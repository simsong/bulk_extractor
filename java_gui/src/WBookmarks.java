import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.event.*;
import java.io.File;
import java.io.PrintWriter;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.FileOutputStream;
import java.util.Observable;
import java.util.Observer;
import java.util.Enumeration;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Text;
import org.w3c.dom.NodeList;

/**
 * The dialog window for exporting bookmarks.
 */

@SuppressWarnings("unchecked") // hacked until we don't require javac6
public class WBookmarks extends JDialog {
  private static final long serialVersionUID = 1;

  public static final String TEXT = "Text";
  public static final String DFXML = "DFXML";

  private static WBookmarks wBookmarks;
//  private static JList<FeatureLine> bookmarkL = new JList<FeatureLine>();
  private static JList bookmarkL = new JList();
  private static JTextField filenameTF = new JTextField();
  private static JButton fileChooserB = new FileChooserButton(BEViewer.getBEWindow(), "Export Bookmarks File As", FileChooserButton.WRITE_FILE, filenameTF);
//  private static JComboBox<String> fileFormatComboBox = new JComboBox<String>();
  private static JComboBox fileFormatComboBox = new JComboBox();
  private static JButton clearB = new JButton("Clear");
  private static JButton deleteB = new JButton("Delete");
  private static JButton navigateB = new JButton("Navigate");
  private static JButton exportB = new JButton("Export");
  private static JButton closeB = new JButton("Close");

  static {
    fileFormatComboBox.addItem(TEXT);
    fileFormatComboBox.addItem(DFXML);
    fileFormatComboBox.setEditable(false);
    fileFormatComboBox.setToolTipText("Select the format to use for exporting bookmarks");
  }

/**
 * Opens this window.
 */
  public static void openWindow() {
    if (wBookmarks == null) {
      // this is the first invocation
      // create the window
      wBookmarks = new WBookmarks();
    }

    // show the dialog window
    wBookmarks.setLocationRelativeTo(BEViewer.getBEWindow());
    wBookmarks.setVisible(true);
  }

  private static void closeWindow() {
    // the window is not instantiated in test mode
    if (wBookmarks != null) {
      WBookmarks.wBookmarks.setVisible(false);
    }
  }

  private WBookmarks() {
    // set parent window, title, and modality

    buildInterface();
    setUIValues();
    wireActions();
    getRootPane().setDefaultButton(exportB);
    pack();
  }

  private void buildInterface() {
    setTitle("Export Bookmarks");
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,0) "Bookmarks"
    c = new GridBagConstraints();
    c.insets = new Insets(10, 10, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.BOTH;
    pane.add(new JLabel("Bookmarks List"), c);

    // (0,1) scroll pane containing Bookmarks table
    c = new GridBagConstraints();
    c.insets = new Insets(0, 10, 10, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // put the bookmarks list in a scroll pane
    JScrollPane scrollPane = new JScrollPane(bookmarkL);
    scrollPane.setPreferredSize(new Dimension(500, 300));

     // add the scroll pane containing the bookmarks list
    pane.add(scrollPane, c);

    // (0,2) add the filename and file input
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildFilePath(), c);

    // (0,3) add the file type control
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 3;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildFileFormatControl(), c);

    // (0,4) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 4;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildControls(), c);
  }

  private Component buildFilePath() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // filename label (0,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Bookmarks Filename"), c);

    // file filenameTF (1,0)
    filenameTF.setMinimumSize(new Dimension(250, filenameTF.getPreferredSize().height));
    filenameTF.setPreferredSize(new Dimension(250, filenameTF.getPreferredSize().height));
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(filenameTF, c);

    // change Export button availability if filenameTF is empty
    filenameTF.getDocument().addDocumentListener(new DocumentListener() {
      // JTextField responds to Document change
      public void insertUpdate(DocumentEvent e) {
        setExportBEnabledState();
      }
      public void removeUpdate(DocumentEvent e) {
        setExportBEnabledState();
      }
      public void changedUpdate(DocumentEvent e) {
        setExportBEnabledState();
      }
    });

    // file chooser (2,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 2;
    c.gridy = 0;
    container.add(fileChooserB, c);

    return container;
  }

  private Component buildFileFormatControl() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // file type label (0,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 0;
    container.add(new JLabel("File Format"), c);

    // export as JComboBox (1,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 0;
    container.add(fileFormatComboBox, c);

    return container;
  }

  private Component buildControls() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,1) clearB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 1;
    container.add(clearB, c);

    // (1,1) deleteB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 1;
    c.gridy = 1;
    container.add(deleteB, c);

    // (2,1) vertical separator
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 2;
    c.gridy = 1;
    container.add(new JSeparator(SwingConstants.VERTICAL), c);

    // (3,1) navigateB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 3;
    c.gridy = 1;
    container.add(navigateB, c);

    // (4,1) exportB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 4;
    c.gridy = 1;
    container.add(exportB, c);

    // (5,1) closeB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 5;
    c.gridy = 1;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(closeB, c);

    return container;
  }

  private void setExportBEnabledState() {
    exportB.setEnabled(BEViewer.featureBookmarksModel.size() > 0 && !filenameTF.getText().equals(""));
  }

@SuppressWarnings("unchecked") // hacked until we don't require javac6
  private void setUIValues() {
    // bookmarkL selection mode and list model
    bookmarkL.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    bookmarkL.setModel(BEViewer.featureBookmarksModel.getListModel());
    bookmarkL.setCellRenderer(new FeatureListCellRenderer());

    // enabled states for buttons
    clearB.setEnabled(BEViewer.featureBookmarksModel.size() > 0);
    deleteB.setEnabled(false);
    navigateB.setEnabled(false);
    setExportBEnabledState();
  }

  private void wireActions() {
    // bookmarkL selection change changes deleteB and navigateB enabled state
    bookmarkL.addListSelectionListener(new ListSelectionListener() {
      public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting() == false) {
          deleteB.setEnabled(bookmarkL.getSelectedIndex() >= 0);
          navigateB.setEnabled(bookmarkL.getSelectedIndex() >= 0);
        }
      }
    });

    // bookmark model being empty disables clearB and exportB
    BEViewer.featureBookmarksModel.addBookmarksModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        clearB.setEnabled(BEViewer.featureBookmarksModel.size() > 0);
        setExportBEnabledState();
      }
    });

    // clicking clearB deletes all bookmark entries
    clearB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        BEViewer.featureBookmarksModel.removeAllBookmarks();
      }
    });

    // clicking deleteB deletes the bookmark entry selected in bookmarkL
    deleteB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        BEViewer.featureBookmarksModel.removeBookmark((FeatureLine)bookmarkL.getSelectedValue());
      }
    });

    // clicking navigateB navigates to the bookmark entry selected in bookmarkL
    navigateB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        FeatureLine featureLine = (FeatureLine)bookmarkL.getSelectedValue();
        if (featureLine == null) {
          throw new NullPointerException();
        }
        BEViewer.featureLineSelectionManager.setFeatureLineSelection(featureLine);
      }
    });

    // clicking exportB exports the bookmarked features
    exportB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        String fileFormat = (String)fileFormatComboBox.getSelectedItem();
        File file = new File(filenameTF.getText());
        startThread(fileFormat, file);
      }
    });

    // clicking closeB closes this window
    closeB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setVisible(false);
      }
    });
  }

  public static Thread startThread(String fileFormat, File file) {

    // verify that the export path is valid
    if (!verifyTargetPath(file)) {
      return null;
    }

    // spawn the thread that performs the export
    if (fileFormat.equals(TEXT)) {
      ExportTextThread exportTextThread = new ExportTextThread();
      exportTextThread.setBookmarkFile(file);
      exportTextThread.start();
      return exportTextThread;
    } else if (fileFormat.equals(DFXML)) {
      ExportDFXMLThread exportDFXMLThread = new ExportDFXMLThread();
      exportDFXMLThread.setBookmarkFile(file);
      exportDFXMLThread.start();
      return exportDFXMLThread;
    } else {
      throw new RuntimeException("Invalid state: " + fileFormat);
    }
  }

  public static Thread testStartThread(String fileFormat, File file, ImageReaderType imageReaderType) {

    // verify that the export path is valid
    if (!verifyTargetPath(file)) {
      throw new RuntimeException("No target path.");
    }

    // spawn the thread that performs the export
    if (fileFormat.equals(TEXT)) {
      ExportTextThread exportTextThread = new ExportTextThread();
      exportTextThread.setBookmarkFile(file);
      exportTextThread.bookmarkImageModel.imageReaderManager.setReaderTypeAllowed(imageReaderType);
      exportTextThread.start();
      return exportTextThread;
    } else if (fileFormat.equals(DFXML)) {
      ExportDFXMLThread exportDFXMLThread = new ExportDFXMLThread();
      exportDFXMLThread.setBookmarkFile(file);
      exportDFXMLThread.bookmarkImageModel.imageReaderManager.setReaderTypeAllowed(imageReaderType);
      exportDFXMLThread.start();
      return exportDFXMLThread;
    } else {
      throw new RuntimeException("Invalid state: " + fileFormat);
    }
  }

  private static boolean verifyTargetPath(File bookmarkFile) {
    // make sure the requested filename does not exist
    if (bookmarkFile.exists()) {
      WError.showError("File '" + bookmarkFile + "' already exists.", "BEViewer file error", null);
      return false;
    }

    // create the output file
    try {
      if (!bookmarkFile.createNewFile()) {
        WError.showError("File '" + bookmarkFile + "' cannot be created.", "BEViewer file error", null);
        return false;
      }
    } catch (IOException e) {
      WError.showError("File '" + bookmarkFile + "' cannot be created.", "BEViewer file error", e);
      return false;
    }

    // verified
    return true;
  }

  // ************************************************************
  // export Text bookmarks
  // ************************************************************
  private static class ExportTextThread extends Thread {
    // image model resource
    private final ImageModel bookmarkImageModel = new ImageModel();
    private final ImageView bookmarkImageView = new ImageView();

    private File bookmarkFile = null;

    public void setBookmarkFile(File file) {
      bookmarkFile = file;
    }

    // run the export thread
    public void run() {

      // open the output writer
      PrintWriter bookmarkTextWriter;
      try {
        bookmarkTextWriter = new PrintWriter(new BufferedWriter(new FileWriter(bookmarkFile)));
      } catch (IOException e) {
        WError.showError("Unable to write to new file " + bookmarkFile + ".", "BEViewer file error", e);
        return;
      }

      // configure the image view
      bookmarkImageView.setAddressFormat(BEViewer.imageView.getAddressFormat());
      bookmarkImageView.setLineFormat(BEViewer.imageView.getLineFormat());

      // go through each bookmark, writing the content of its associated FeatureLine
// NOTE: this recommended syntax is not permitted because DefaultListModel does not allow anything but Object
//    Enumeration<FeatureLine> bookmarks = BEViewer.featureBookmarksModel.getListModel().elements();
      Enumeration<?> bookmarks = BEViewer.featureBookmarksModel.getListModel().elements();
      while (bookmarks.hasMoreElements()) {
        FeatureLine featureLine = (FeatureLine)bookmarks.nextElement();
        exportText(bookmarkTextWriter, featureLine);
      }

      // close the output writer
      bookmarkTextWriter.close();

      // close all image readers
      bookmarkImageModel.closeAllImageReaders();

      // close the Export Bookmarks window
      closeWindow();

      WLog.log("WBookmarks: bookmarks to text done.");
    }

    private void exportText(PrintWriter writer, FeatureLine featureLine) {
      // print the summary using featureLine's getSummaryString
      writer.println(featureLine.getSummaryString());

      // abort if there is no image associated with this feature line
      if (featureLine.getImageFile() == null) {
        writer.println("An image file is not available for this Feature.");
        return;
      }

      // read image bytes from the image model
      bookmarkImageModel.setFeatureLine(featureLine);

      // wait until generation is done
      while (bookmarkImageModel.isBusy()) {
        Thread.yield();
      }

      // get image page from image model
      ImageModel.ImagePage imagePage = bookmarkImageModel.getImagePage();

      // generate the image view from the image page
      bookmarkImageView.setImagePage(imagePage);

      // get number of available image lines
      int size = bookmarkImageView.getNumLines();
      // print each line
      for (int i=0; i<size; i++) {
        ImageLine imageLine = bookmarkImageView.getLine(i);
        writer.println(imageLine.text);
      }
      writer.println();
    }
  }


  // ************************************************************
  // export DFXML bookmarks
  // ************************************************************
  private static class ExportDFXMLThread extends Thread {
    // image model resource
    private ImageModel bookmarkImageModel = new ImageModel();
    private final ImageView bookmarkImageView = new ImageView();

    private File bookmarkFile = null;

    public void setBookmarkFile(File file) {
      bookmarkFile = file;
    }

    // run the export thread
    public void run() {

      try {

        // create the DOM doc object
        DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();
        DocumentBuilder builder = builderFactory.newDocumentBuilder();
        Document doc = builder.newDocument();

        // create the root dfxml element
        Element root = doc.createElement("dfxml");
        root.setAttribute("xmloutputversion", "1.0");
        doc.appendChild(root);

        // configure the image view
        bookmarkImageView.setAddressFormat(BEViewer.imageView.getAddressFormat());
        bookmarkImageView.setLineFormat(BEViewer.imageView.getLineFormat());

        // fill the bookmark elements from the array of bookmarks
        Enumeration<?> bookmarks = BEViewer.featureBookmarksModel.getListModel().elements();
        while (bookmarks.hasMoreElements()) {
          FeatureLine featureLine = (FeatureLine)bookmarks.nextElement();
          exportDFXML(doc, root, featureLine);
        }

        // create transformer for converting DOM doc source to XML-formatted StreamResult object
        TransformerFactory transformerFactory = TransformerFactory.newInstance();
        Transformer transformer = transformerFactory.newTransformer();
        transformer.setOutputProperty(OutputKeys.INDENT, "yes");

        // create transformer source from doc
        DOMSource domSource = new DOMSource(doc);

        // create transformer destination to bookmarks file
        FileOutputStream fileOutputStream = new FileOutputStream(bookmarkFile);
        StreamResult streamResult = new StreamResult(fileOutputStream);

        // transform DOM doc to XML-formatted StreamResult object
        transformer.transform(domSource, streamResult);

        // close the file output stream
        fileOutputStream.flush();
        fileOutputStream.close();

      } catch (Exception e) {
        WError.showError("Unable to export DFXML Bookmarks file " + bookmarkFile + ".",
                         "BEViewer file error", e);
      }
 
      // close all image readers
      bookmarkImageModel.closeAllImageReaders();

      // close the Export Bookmarks window
      closeWindow();

      WLog.log("WBookmarks: bookmarks to DFXML done.");
    }

    private void exportDFXML(Document doc, Element parent, FeatureLine featureLine) {

      // get the requisite bookmark attributes
      String imageFileString = FileTools.getAbsolutePath(featureLine.getImageFile());
      String featuresFileString = featureLine.getFeaturesFile().getAbsolutePath();

      // create the Bookmark element
      Element bookmark = doc.createElement("bookmark");

      // set the bookmark attributes
      bookmark.setAttribute("feature", featureLine.getFormattedFeatureText());
      String contextField = new String(featureLine.getContextField());
      bookmark.setAttribute("context", contextField);
      bookmark.setAttribute("path", featureLine.getFormattedFirstField(bookmarkImageView.getAddressFormat()));
      bookmark.setAttribute("image_file", imageFileString);
      bookmark.setAttribute("feature_file", featuresFileString);

      // add the Bookmark element to the case settings element
      parent.appendChild(bookmark);

      // abort if there is no image associated with this feature line
      if (featureLine.getImageFile() == null) {
        // write line node stating that the image file is not available
        Element line = doc.createElement("line");
        bookmark.appendChild(line);
        Text text = doc.createTextNode("An image file is not available for this Feature.");
        line.appendChild(text);
        return;
      }

      // start generating printable image lines
      bookmarkImageModel.setFeatureLine(featureLine);

      // wait until generation is done
      while (bookmarkImageModel.isBusy()) {
        Thread.yield();
      }

      // get image page from image model
      ImageModel.ImagePage imagePage = bookmarkImageModel.getImagePage();

      // generate the image view
      bookmarkImageView.setImagePage(imagePage);

      // get number of available image lines
      int size = bookmarkImageView.getNumLines();

      // export each line
      for (int i=0; i<size; i++) {
        ImageLine imageLine = bookmarkImageView.getLine(i);

        // add image line element
        Element line = doc.createElement("line");
        bookmark.appendChild(line);
        Text text = doc.createTextNode(imageLine.text);
        line.appendChild(text);
      }
    }
  }
}


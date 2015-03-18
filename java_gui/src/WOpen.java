import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.TreePath;
import java.awt.event.*;
import java.io.File;
import java.io.IOException;
import java.util.Enumeration;
import javax.swing.filechooser.FileFilter;  // not java.io.FileFilter
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPathFactory;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathExpressionException;
import org.w3c.dom.Document;
import org.xml.sax.SAXException;

/**
 * The dialog window for opening a report.
 * When a report is opened the image file is set, the features directory is set,
 * and the features file is cleared.
 * When a report is not opened, no change is made.
 * If a failure occurs when opening the image file or report file,
 * then the image file and report file are cleared and an error is reported.
 */
// NOTE: JFileChooser cannot be transitioned between FILES_ONLY and DIRECTORIES_ONLY
// because the Java API fails to perform the transition.  To compensate, chooser objects
// are recreated rather than cached.

public class WOpen extends JDialog {
  private static final long serialVersionUID = 1;

  private static WOpen wOpen;
  private final JTextField reportFileTF = new JTextField(30);
  private final JButton reportFileB = new FileChooserButton(BEViewer.getBEWindow(), "Select Report File (" + REPORT_FILE_NAME_STRING + ")", FileChooserButton.READ_REPORT_FILE, reportFileTF);
  private final JRadioButton defaultImageFileRB = new JRadioButton("Use path from Report:");
  private final PathComponent defaultImageFileLabel = new PathComponent();
  private final JRadioButton customImageFileRB = new JRadioButton("Select custom path:");
  private final JRadioButton noImageFileRB = new JRadioButton("No Image File");
  private final ButtonGroup imageFileGroup = new ButtonGroup();
  private final JTextField customImageFileTF = new JTextField(30);
  private final JButton customImageFileB = new FileChooserButton(BEViewer.getBEWindow(), "Select Image File", FileChooserButton.READ_IMAGE_FILE, customImageFileTF);
  private final JButton okB = new JButton("OK");
  private final JButton cancelB = new JButton("Cancel");

  private ImagePathPreference imagePathPreference;

  private static final String REPORT_FILE_NAME_STRING = "report.xml";
  private final DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();
  private DocumentBuilder builder;

  // reportFileFilter identifies valid "report.xml" files
  private static final FileFilter reportFileFilter = new FileFilter() {
    // accept filenames for valid report files
    public boolean accept(File pathname) {
      // accept directories and file report.xml
      return pathname.isDirectory()
          || pathname.getName().equals(REPORT_FILE_NAME_STRING);
    }
    public String getDescription() {
      return "Report Files (report.xml)";
    }
  };
 
  /**
   * The <code>PathComponent</code> class provides an uneditable text view of a file's filename.
   * The path is enabled when valid and PathComponent is enabled.
   * The path is grayed out when the path is null or the PathComponent is not enabled.
   * This class is a variation of FileComponent.
   */
  private static class PathComponent extends JLabel {
    private static final long serialVersionUID = 1;
    private static final String NULL_FILE = "None";
    private File file;
    private boolean isEnabled;

    /**
     * Creates a path component object.
     */
    public PathComponent() {
      setText(" "); // required during initialization for getPreferredSize()
      setPath(null);
      setMinimumSize(new Dimension(0, getPreferredSize().height));
    }

    /**
     * Sets the file to display.
     * @param file the file to display
     */
    public void setEnabled(boolean isEnabled) {
      // record request
      this.isEnabled = isEnabled;
      // activate request
      setPath(file);
    }

    /**
     * Sets the file to display.
     * @param file the file to display
     */
    public void setPath(File file) {
      this.file = file;
      Font font = getFont();
      if (file == null) {
        super.setEnabled(false);
        setFont(font.deriveFont(Font.ITALIC));
        setText(NULL_FILE);
      } else {
        super.setEnabled(isEnabled);
        setFont(font.deriveFont(Font.PLAIN));
        setText(file.getPath());
      }
    }

    /**
     * Returns the file being displayed.
     * @return the file being displayed
     */
    public File getPath() {
      return file;
    }
  }

  // class for enumerating the requested image source
  private static final class ImagePathPreference {
    private static final ImagePathPreference DEFAULT = new ImagePathPreference("Default");
    private static final ImagePathPreference CUSTOM = new ImagePathPreference("Custom");
    private static final ImagePathPreference NONE = new ImagePathPreference("None");
    private final String name;
    private ImagePathPreference(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }
 
  /**
   * Opens this window.
   */
  public static void openWindow() {
    if (wOpen == null) {
      // this is the first invocation
      // create the window
      wOpen = new WOpen();
    }

    // set the initial file values
    wOpen.setInitialFiles();

    // show the dialog window
    wOpen.setLocationRelativeTo(BEViewer.getBEWindow());
    wOpen.setVisible(true);
  }

  private void setReportFile(File reportFile) {
    reportFileTF.setText((reportFile == null) ? "" : reportFile.getPath());
  }

  private void setDefaultImageFile(File defaultImageFile) {
    // set the default image file
    defaultImageFileLabel.setPath(defaultImageFile);
  }

  private void setCustomImageFile(File customImageFile) {
    customImageFileTF.setText((customImageFile == null) ? "" : customImageFile.getPath());
  }

  private void setImagePathPreference(ImagePathPreference imagePathPreference) {
    this.imagePathPreference = imagePathPreference;

    if (imagePathPreference == ImagePathPreference.DEFAULT) {
      // use default path
      defaultImageFileRB.setSelected(true);

      // enable default image file inputs and disable custom image file inputs
      defaultImageFileLabel.setEnabled(true);
      customImageFileTF.setEnabled(false);
      customImageFileB.setEnabled(false);

    } else if (imagePathPreference == ImagePathPreference.CUSTOM) {

      // use custom path
      customImageFileRB.setSelected(true);

      // disable default image file inputs and enable custom image file inputs
      defaultImageFileLabel.setEnabled(false);
      customImageFileTF.setEnabled(true);
      customImageFileB.setEnabled(true);

    } else if (imagePathPreference == ImagePathPreference.NONE) {

      // use no path
      noImageFileRB.setSelected(true);

      // disable default image file inputs and custom image file inputs
      defaultImageFileLabel.setEnabled(false);
      customImageFileTF.setEnabled(false);
      customImageFileB.setEnabled(false);
    } else {
      throw new RuntimeException("Invalid type");
    }
  }

  private void setInitialFiles() {
    // use a report tree node as a template
    TreePath selectedTreePath = BEViewer.reportsModel.getSelectedTreePath();
    ReportsModel.ReportTreeNode reportTreeNode = ReportsModel.getReportTreeNodeFromTreePath(selectedTreePath);

    if (reportTreeNode == null) {
      // if not selected, use the first report loaded as a template, instead
      Enumeration<ReportsModel.ReportTreeNode> e = BEViewer.reportsModel.elements();
      if (e.hasMoreElements()) {
        reportTreeNode = e.nextElement();
      }
    }

    // use the identified report else null
    if (reportTreeNode != null) {
      // set reportFile and imageFile from report
      File featuresDirectory = reportTreeNode.featuresDirectory;
      File reportFile = new File(featuresDirectory, REPORT_FILE_NAME_STRING);
      setReportFile(reportFile);
      File defaultImageFile = readDefaultImageFile(reportFile);
      setDefaultImageFile(defaultImageFile);
      setCustomImageFile(reportTreeNode.reportImageFile);
    } else {
      // leave blank default
      setReportFile(null);
      setDefaultImageFile(null);
      setCustomImageFile(null);
    }

    // always start using default image file
    setImagePathPreference(ImagePathPreference.DEFAULT);
  }

  // returns the image filename from the report file else null
  private File readDefaultImageFile(File reportFile) {
    Document document;

    // stop now if reportFile "report.xml" doesn't exist or is a directory
    if (reportFile == null || reportFile.isDirectory() || !reportFile.canRead()
            || !reportFile.getName().equals(REPORT_FILE_NAME_STRING)) {
      return null;
    }

    // parse the XML report file using DOM
    try {
      builder.reset();
      document = builder.parse(new File (reportFile.getPath()));
    } catch (IOException ioe) {
      // give up on finding the image filename
      WLog.log("WOpen XML parser: IO failure in file " + reportFile.getPath());
      return null;
    } catch (SAXException se) {
      // give up on finding the image filename
      WLog.log("WOpen XML parser: SAX failure in file " + reportFile.getPath());
      return null;
    }

    // get the image filename
    XPathFactory xPathFactory = XPathFactory.newInstance();
    XPath xPath = xPathFactory.newXPath();
    try {
      final String IMAGE_XPATH = "/dfxml/source/image_filename";
      String filename = xPath.evaluate(IMAGE_XPATH, document);
      // check that the xPath was found, not found returns "".
      if (filename.equals("")) {
        WLog.log("WOpen XML parser: unable to parse image filename from report file at XPath "
                 + IMAGE_XPATH);
        return null;
      }
      File imageFile = new File(filename);

      // turn relative path into absolute path
      if (!imageFile.isAbsolute() && reportFile.getParentFile() != null
                     && reportFile.getParentFile().getParentFile() != null) {
        imageFile = new File(reportFile.getParentFile().getParentFile(), filename);
      }
      return imageFile;
    } catch (XPathExpressionException xpe) {
      // give up on finding the image filename
      WLog.log("WOpen XML parser: unable to parse image filename from report file");
      return null;
    }
  }

  // create the dialog window
  private WOpen() {
    super(BEViewer.getBEWindow(), "Open Report", true);
    setComponents();
    wireActions();

    // set the OK button as the default button
    getRootPane().setDefaultButton(okB);

    // establish the XML Document builder for parsing REPORT_FILE_NAME_STRING
    try {
      builder = builderFactory.newDocumentBuilder();
    } catch (ParserConfigurationException e) {
      WError.showError( "Parser failure.", "BEViewer file error", e);
    }
    pack();
  }

  /**
   * Constructs navigation interfaces
   */
  private void setComponents() {
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // ************************************************************
    // (0,0) inputs
    // ************************************************************
    c = new GridBagConstraints();
    c.insets = new Insets(20, 20, 20, 20);
    c.gridx = 0;
    c.gridy = 0;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the navigation title
    pane.add(getInputs(), c);

    // ************************************************************
    // (0,1) controls
    // ************************************************************
    c = new GridBagConstraints();
    c.insets = new Insets(0, 20, 20, 20);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 0;
    c.anchor = GridBagConstraints.LINE_START;

    // add the navigation control container
    pane.add(getControls(), c);
  }

  // add the report and image inputs
  private Container getInputs() {
    Container container = new Container();
    container.setLayout(new GridBagLayout());
    GridBagConstraints c;

    // first row: report file
    int y = 0;

    // (0,y) JLabel section title "Report file (report.xml)"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;

    // add the section title
    container.add(new JLabel("Report File (" + REPORT_FILE_NAME_STRING + ")"), c);

    // (1,y) JTextField report file input
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 1;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.HORIZONTAL;
    c.weightx = 1;

    // add the report file input textfield
    container.add(reportFileTF , c);

    // (2,y) JButton open report file chooser
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 2;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;

    // add the report file chooser button
    container.add(reportFileB, c);

    // next row: image file label
    y++;

    // (0,y) JLabel section title "image file"
    c = new GridBagConstraints();
    c.insets = new Insets(20, 0, 0, 10);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;

    // add the section title
    container.add(new JLabel("Image File"), c);

    // next row: default path to image file from report
    y++;

    // (0,y) JRadioButton "default path from report"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;

    // add the "default path from report" radiobutton
    container.add(defaultImageFileRB, c);

    // (1,y) <FileLabel of default image file path from report.xml lookup>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.HORIZONTAL;
    c.weightx = 1;

    // add the FileLabel
    container.add(defaultImageFileLabel, c);

    // next row: custom image file
    y++;

    // (0,y) JRadioButton "Use custom path"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;

    // add the "Use custom path" selection radiobutton
    container.add(customImageFileRB, c);

    // (1,y) JTextField custom image file input
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 1;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.HORIZONTAL;
    c.weightx = 1;

    // add the custom image file input textfield
    container.add(customImageFileTF, c);

    // (2,y) JButton open image file chooser
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 2;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;

    // add the image file chooser button
    container.add(customImageFileB, c);

    // next row: No image file
    y++;

    // (0,y) JRadioButton "no image file"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;

    // add the "no image file" selection radiobutton
    container.add(noImageFileRB, c);

    // group the JRadioButtons together
    imageFileGroup.add(defaultImageFileRB);
    imageFileGroup.add(customImageFileRB);
    imageFileGroup.add(noImageFileRB);

    return container;
  }

  // add the OK and Cancel controls
  private Component getControls() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) okB
    c = new GridBagConstraints();
    c.insets = new Insets(0, 40, 0, 0);
    c.gridx = 0;
    c.gridy = 0;
    container.add(okB, c);

    // (1,0) cancelB
    c = new GridBagConstraints();
    c.insets = new Insets(0, 30, 0, 0);
    c.gridx = 1;
    c.gridy = 0;
    container.add(cancelB, c);

    return container;
  }

  private void wireActions() {
    // when report file changes maybe update image file text
    reportFileTF.getDocument().addDocumentListener(new DocumentListener() {
      // JTextField responds to Document change
      public void insertUpdate(DocumentEvent e) {
        setDefaultImageFile(readDefaultImageFile(new File(reportFileTF.getText())));
      }
      public void removeUpdate(DocumentEvent e) {
        setDefaultImageFile(readDefaultImageFile(new File(reportFileTF.getText())));
      }
      public void changedUpdate(DocumentEvent e) {
        setDefaultImageFile(readDefaultImageFile(new File(reportFileTF.getText())));
      }
    });

    // on "default" radio button selection set for default image file
    defaultImageFileRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        setImagePathPreference(ImagePathPreference.DEFAULT);
      }
    });

    // on "custom" radio button selection set for custom image file
    customImageFileRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        setImagePathPreference(ImagePathPreference.CUSTOM);
      }
    });

    // on "No image path" radio button selection set for no image file
    noImageFileRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        setImagePathPreference(ImagePathPreference.NONE);
      }
    });

    // cancel the open operation on cancel button selection
    cancelB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // hide the dialog window
        wOpen.setVisible(false);
      }
    });

    // accept the open operation on OK button selection
    okB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        String errorString = null;

        // validate the report file
        File reportFile = new File(reportFileTF.getText());
        if (!reportFile.getName().equals(REPORT_FILE_NAME_STRING)) {
          // not "report.xml"
          errorString = "Report file " + reportFile.getPath() + "\ndoes not end in " + REPORT_FILE_NAME_STRING + ".";
        } else if (reportFile.isDirectory()) {
          // directory
          errorString = "Directory " + reportFile.getPath() + "\nis not a valid Report file.";
        } else if (!reportFile.canRead() && !reportFile.isAbsolute()) {
          // can't read and it is at a relative path
          errorString = "Report file " + reportFile.getPath() + "\ncannot be read.  Please select an absolute path.";
        } else if (!reportFile.canRead()) {
          // can't read
          errorString = "Report file " + reportFile.getPath() + "\ncannot be read.  Please verify that the Report file is valid.";
        } else {
          // good, the file is valid, normalize it
          try {
            reportFile = reportFile.getCanonicalFile();
          } catch (IOException ioe) {
            errorString = "Report file " + reportFile.getPath() + "\ncannot be read: " + ioe;
          }
        }

        // failed if errorString was defined
        if (errorString != null) {
          WError.showError(errorString, "BEViewer Open error", null);
          return;
        }

        // identify the image file
        File imageFile;
        if (imagePathPreference == ImagePathPreference.DEFAULT) {
          imageFile = defaultImageFileLabel.getPath();
          if (imageFile == null) {
            WError.showError("The Image File path could not be obtained from the report."
                    + "\nThe Image file may have been moved or the Report File may be invalid."
                    + "\nPlease click on \"select custom path\" to select an Image File path for this Report file.",
                    "BEViewer Open error", null);
            return;
          }
         
        } else if (imagePathPreference == ImagePathPreference.CUSTOM) {
          imageFile = new File(customImageFileTF.getText());

        } else if (imagePathPreference == ImagePathPreference.NONE) {
          imageFile = null;

        } else {
          throw new RuntimeException("Invalid type");
        }

        // validate the image file
        // NOTE: a directory is valid because it indicates the absolute
        // path to recursive files
        if (imagePathPreference != ImagePathPreference.NONE) {
          if (!imageFile.canRead() && !imageFile.isAbsolute()) {
            errorString = "Image file \"" + imageFile.getPath() + "\"\ncannot be read, likely because it is defined with a relative path."
                +"\nPlease click on \"Select custom path\" to select or type in an absolute path.";
          } else if (!imageFile.canRead()) {
            errorString = "Image file \"" + imageFile.getPath() + "\"\ncannot be read.";
          } else {
            // good, the file is valid, normalize it
            try {
              imageFile = imageFile.getCanonicalFile();
            } catch (IOException ioe) {
              errorString = "Image file \"" + imageFile.getPath() + "\"\ncannot be read: " + ioe;
            }
          }

          if (errorString != null) {
            WError.showError(errorString, "BEViewer Open error", null);
            return;
          }
        }

        // get the features directory from the path of the report file
        File featuresDirectory = reportFile.getParentFile();

        // Add the report
        BEViewer.reportsModel.addReport(featuresDirectory, imageFile);
        
        // get the tree path of the added report
        TreePath addedTreePath = BEViewer.reportsModel.getTreePath(featuresDirectory, imageFile);

        // open and scroll to the added report tree path
        BEViewer.reportsPane.scrollToTreePath(addedTreePath);

        // select the report
        BEViewer.reportsModel.getTreeSelectionModel().setSelectionPath(addedTreePath);

        // hide the dialog window
        wOpen.setVisible(false);
      }
    });
  }
}


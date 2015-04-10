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
public class WManageBookmarks extends JDialog {
  private static final long serialVersionUID = 1;

  private static WManageBookmarks wBookmarks;
//  private static JList<FeatureLine> bookmarkL = new JList<FeatureLine>();
  // this works because bookmarksComboBoxModel superclasses DefaultComboBoxModel
  private static JList bookmarkL = new JList(BEViewer.bookmarksModel.bookmarksComboBoxModel);
  private static JButton clearB = new JButton("Clear");
  private static JButton deleteB = new JButton("Delete");
  private static JButton navigateB = new JButton("Navigate To");
  private static JButton exportB = new JButton("Export");
  private static JButton closeB = new JButton("Close");

  static {
    bookmarkL.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    bookmarkL.setCellRenderer(new FeatureListCellRenderer());
  }


  /**
   * Use this static function to save bookmarks.
   */
  public static void saveBookmarks() {
    JFileChooser chooser = new JFileChooser();
    chooser.setDialogTitle("Export Bookmarks as");
    chooser.setDialogType(JFileChooser.SAVE_DIALOG);
    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);

    // if the user selects APPROVE then perform the export
    if (chooser.showOpenDialog(BEViewer.getBEWindow()) == JFileChooser.APPROVE_OPTION) {
      File file = chooser.getSelectedFile();

      // validate that the export path is empty and writable
      if (!validateTargetPath(file)) {
        return;
      }

      // start the thread that performs the export
      startThread(file);
    }
  }

  /**
   * Opens this window.
   */
  public static void openWindow() {
    if (wBookmarks == null) {
      // this is the first invocation
      // create the window
      wBookmarks = new WManageBookmarks();
    }

    // show the dialog window
    wBookmarks.setLocationRelativeTo(BEViewer.getBEWindow());
    wBookmarks.setVisible(true);
  }

  private static void closeWindow() {
    // the window is not instantiated in test mode
    if (wBookmarks != null) {
      WManageBookmarks.wBookmarks.setVisible(false);
    }
  }

  private WManageBookmarks() {
    // set parent window, title, and modality

    buildInterface();
    setButtonStates();
    wireActions();
    getRootPane().setDefaultButton(exportB);
    pack();
  }

  private void buildInterface() {
    setTitle("Bookmarks");
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,0) "Bookmarks List" label
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

    // (0,2) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildControls(), c);
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

  private void setButtonStates() {

    // set states for buttons
    clearB.setEnabled(!BEViewer.bookmarksModel.isEmpty());
    deleteB.setEnabled(bookmarkL.getSelectedIndex() >= 0);
    navigateB.setEnabled(bookmarkL.getSelectedIndex() >= 0);
    exportB.setEnabled(!BEViewer.bookmarksModel.isEmpty());
  }

  private void wireActions() {
    // listen to bookmarks JList selection because selection state
    // changes button states
    bookmarkL.addListSelectionListener(new ListSelectionListener() {
      public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting() == false) {
          setButtonStates();
        }
      }
    });

    // listen to bookmarks list changes
    BEViewer.bookmarksModel.bookmarksComboBoxModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        setButtonStates();
      }
      public void intervalAdded(ListDataEvent e) {
        setButtonStates();
      }
      public void intervalRemoved(ListDataEvent e) {
        setButtonStates();
      }
    });

    // clicking clearB deletes all bookmark entries
    clearB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        BEViewer.bookmarksModel.clear();
      }
    });

    // clicking deleteB deletes the bookmark entry selected in bookmarkL
    deleteB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        BEViewer.bookmarksModel.removeElement((FeatureLine)bookmarkL.getSelectedValue());
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
        saveBookmarks();
      }
    });

    // clicking closeB closes this window
    closeB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setVisible(false);
      }
    });
  }

  public static Thread startThread(File file) {

    // spawn the thread that performs the export
    ExportBookmarksThread exportBookmarksThread = new ExportBookmarksThread(file);
    exportBookmarksThread.start();
    return exportBookmarksThread;
  }

  private static boolean validateTargetPath(File bookmarkFile) {
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
  // export bookmarks
  // ************************************************************
  private static class ExportBookmarksThread extends Thread {
    public ExportBookmarksThread(File bookmarkFile) {
      this.bookmarkFile = bookmarkFile;
    }

    // image model resource
    private final ImageModel bookmarkImageModel = new ImageModel();
    private final ImageView bookmarkImageView = new ImageView();

    private final File bookmarkFile;

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
      bookmarkImageView.setUseHexPath(BEViewer.imageView.getUseHexPath());
      bookmarkImageView.setLineFormat(BEViewer.imageView.getLineFormat());

      // fill the bookmark elements from the array of bookmarks
      FeatureLine[] featureLines = BEViewer.bookmarksModel.getBookmarks();
        for (int i=0; i<featureLines.length; i++) {
        exportText(bookmarkTextWriter, featureLines[i]);
      }

      // close the output writer
      bookmarkTextWriter.close();

      // close the Export Bookmarks window
      closeWindow();

      WLog.log("WManageBookmarks: export bookmarks done.");
    }

    private void exportText(PrintWriter writer, FeatureLine featureLine) {
      // print the summary using featureLine's getSummaryString
      writer.println(featureLine.getSummaryString());

      // read image bytes from the image model
      bookmarkImageModel.setImageSelection(featureLine);

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
}


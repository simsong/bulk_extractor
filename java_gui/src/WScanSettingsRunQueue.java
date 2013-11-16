import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.event.*;
import java.io.File;
import java.io.IOException;
import java.util.Observable;
import java.util.Observer;

/**
 * The dialog window for managing the bulk_extractor run queue scheduler
 */

//@SuppressWarnings("unchecked") // hacked until we don't require javac6
public class WScanSettingsRunQueue extends JDialog {
  private static final long serialVersionUID = 1;

  private static WScanSettingsRunQueue wScanSettingsRunQueue
  private static JList runQueueL = new JList(ScanSettingsRunQueue.getListModel());
  private static JButton upB = new JButton("Up");
  private static JButton downB = new JButton("Down");
  private static JButton editB = new JButton("Edit");
  private static JButton closeB = new JButton("Close");

  static {
    runQueueL.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    runQueueL.setCellRenderer(new ScanSettingsListCellRenderer());
  }

  /**
   * Opens this window.
   */
  public static void openWindow() {
    if (wScanSettingsRunQueue == null) {
      // this is the first invocation
      // create the window
      wScanSettingsRunQueue = new WScanSettingsRunQueue();
    }

    // show the dialog window
    wScanSettingsRunQueue.setLocationRelativeTo(BEViewer.getBEWindow());
    wScanSettingsRunQueue.setVisible(true);
  }

  private static void closeWindow() {
    // the window is not instantiated in test mode
    if (wScanSettingsRunQueue != null) {
      WScanSettingsRunQueue.wScanSettingsRunQueue.setVisible(false);
    }
  }

  private WScanSettingsRunQueue() {
    // set parent window, title, and modality

    buildInterface();
    setButtonStates();
    wireActions();
    getRootPane().setDefaultButton(closeB);
    pack();
  }

  private void buildInterface() {
    setTitle("bulk_extractor Run Queue");
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,0) "Run Queue" label
    c = new GridBagConstraints();
    c.insets = new Insets(10, 10, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.BOTH;
    pane.add(new JLabel("Run Queue"), c);

    // (0,1) scroll pane containing the Scan Settings jobs
    c = new GridBagConstraints();
    c.insets = new Insets(0, 10, 10, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // put the Scan Settings job list in a scroll pane
    JScrollPane scrollPane = new JScrollPane(bookmarkL);
    scrollPane.setPreferredSize(new Dimension(600, 400));

     // add the scroll pane containing the Scan Settings job list
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

    int x=0;
    int y=0;
    // upB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(upB, c);

    // downB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(downB, c);

    // editB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(editB, c);

    // vertical separator
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(new JSeparator(SwingConstants.VERTICAL), c);

    // deleteB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(deleteB, c);

    // vertical separator
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(new JSeparator(SwingConstants.VERTICAL), c);

    // closeB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(closeB, c);

    return container;
  }

  private void setButtonStates() {

    // set states for buttons
    upB.setEnabled(bookmarkL.getSelectedIndex() > 1
                   && ScanSettingsRunQueue.size() >= 2);
    downB.setEnabled(bookmarkL.getSelectedIndex()
                     < (ScanSettingsRunQueue.size() - 1);
    editB.setEnabled(bookmarkL.getSelectedIndex() >= 0);
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

      // close all image readers
      bookmarkImageModel.closeAllImageReaders();

      // close the Export Bookmarks window
      closeWindow();

      WLog.log("WScanSettingsRunQueue: export bookmarks done.");
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


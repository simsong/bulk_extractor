import java.awt.*;
import javax.swing.*;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.event.*;
import java.io.File;

/**
 * The dialog window for running automated tests.
 */

public class WTest extends JDialog {
  private static final long serialVersionUID = 1;
  private final WTest wTest;

  public static String testWorkSettingsFileString;
  public static String testOutputDirectoryString;
  public static String testScanDirective;

  private JTextField testWorkSettingsFileTF = new JTextField(testWorkSettingsFileString, 50);
  private JTextField testOutputDirectoryTF = new JTextField(testOutputDirectoryString, 50);
  private JTextField testScanDirectiveTF = new JTextField(testScanDirective, 50);
  private JButton testWorkSettingsFileB = new FileChooserButton(BEViewer.getBEWindow(), "Select work settings File", FileChooserButton.READ_FILE, testWorkSettingsFileTF);
  private JButton testOutputDirectoryB = new FileChooserButton(BEViewer.getBEWindow(), "Select Output Directory", FileChooserButton.READ_DIRECTORY, testOutputDirectoryTF);

  private JCheckBox preferencesCB = new JCheckBox("Preferences");
  private JCheckBox bookmarksCB = new JCheckBox("Bookmarks");
  private JCheckBox scanCB = new JCheckBox("Scan");

  private JButton runB = new JButton("Run");
  private JButton closeB = new JButton("Close");

  // note: tests must run on theread else test hang hangs Swing queue
  private class TestThread extends Thread {
    public void run() {
      // run each test selected by checkbox
      WLog.log("Starting tests");
      if (preferencesCB.isSelected()) {
        // test the integrity of the Preferences capability
        BETests.checkPreferences();
      }
      if (bookmarksCB.isSelected()) {
        // create Bookmarks from a specified work settings file
        File testWorkSettingsFile = new File(testWorkSettingsFileString);
        File testOutputDirectory = new File(testOutputDirectoryString);
        BETests.makeBookmarks(testWorkSettingsFile, testOutputDirectory);
      }
      WLog.log("Tests will be done when scan competes.");
      if (scanCB.isSelected()) {
        // Run a bulk_extractor scan using provided input
        ScanSettings scanSettings = new ScanSettings(testScanDirective);
        BEViewer.scanSettingsListModel.add(scanSettings);
      }
    }
  }

  public WTest() {
    wTest = this;
    buildInterface();
    setDefaults();
    pack();
    setLocationRelativeTo(BEViewer.getBEWindow());
    wireActions();
    setVisible(true);
  }

  private void buildInterface() {
    // set the title to include the image filename
    setTitle("Test Bulk Extractor");

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    Container pane = getContentPane();
    pane.setLayout(new GridBagLayout());

    // (0,0) inputs
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    pane.add(getInputsContainer(), c);

    // (0,1) test selections
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(getTestSelectionsContainer(), c);

    // (0,2) Control
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 15, 5);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(getControlsContainer(), c);
  }

  private void wireActions() {

    // on work settings file chooser button selection open a work settings file chooser
    testWorkSettingsFileB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        JFileChooser chooser = new JFileChooser();
        chooser.setDialogTitle("Select Work Settings File");
        chooser.setDialogType(JFileChooser.OPEN_DIALOG);
        chooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
        chooser.setSelectedFile(new File(testWorkSettingsFileTF.getText()));

        // if the user selects APPROVE then take the report file text
        if (chooser.showOpenDialog(wTest) == JFileChooser.APPROVE_OPTION) {
          // get the report file text and put it in the report file textfield without validating it
          String reportFileString = chooser.getSelectedFile().getAbsolutePath();
          testWorkSettingsFileTF.setText(reportFileString);
        }
      }
    });

    // on output directory chooser button selection open a work settings file chooser
    testOutputDirectoryB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        JFileChooser chooser = new JFileChooser();
        chooser.setDialogTitle("Select Output Directory");
        chooser.setDialogType(JFileChooser.OPEN_DIALOG);
        chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
        chooser.setSelectedFile(new File(testOutputDirectoryTF.getText()));

        // if the user selects APPROVE then take the report file text
        if (chooser.showOpenDialog(wTest) == JFileChooser.APPROVE_OPTION) {
          // get the report file text and put it in the report file textfield without validating it
          String reportFileString = chooser.getSelectedFile().getAbsolutePath();
          testOutputDirectoryTF.setText(reportFileString);
        }
      }
    });

    // run button
    runB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {

        // export test settings
        testWorkSettingsFileString = testWorkSettingsFileTF.getText();
        testOutputDirectoryString = testOutputDirectoryTF.getText();
        testScanDirective = testScanDirectiveTF.getText();

        // run
        TestThread testThread = new TestThread();
        testThread.start();
      }
    });

    // close button
    closeB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // close
        dispose();
      }
    });
  }

  private void setDefaults() {
    // default button, default checkbox settings
    getRootPane().setDefaultButton(runB);
    preferencesCB.setSelected(true);
    bookmarksCB.setSelected(true);
    scanCB.setSelected(true);
  }

  // inputs container
  private Container getInputsContainer() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) "Test Work Settings File"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Test Work Settings File"), c);

    // (1,0) <test work settings file>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(testWorkSettingsFileTF, c);

    // (2,0) <test work settings file chooser button>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 0);
    c.gridx = 2;
    c.gridy = 0;
    container.add(testWorkSettingsFileB, c);

    // (0,1) "Test Output Directory"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Test Output Directory"), c);

    // (1,1) <test output directory>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 1;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(testOutputDirectoryTF, c);

    // (2,1) <test output directory chooser button>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 0);
    c.gridx = 2;
    c.gridy = 1;
    container.add(testOutputDirectoryB, c);

    // (0,2) "Test Scan Directive"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Test Scan Directive"), c);

    // (1,2) <test scan directive>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 2;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(testScanDirectiveTF, c);

    return container;
  }

  // test selections container
  private Container getTestSelectionsContainer() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) preferences checkbox
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(preferencesCB, c);

    // (0,1) bookmarks checkbox
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(bookmarksCB, c);

    // (0,2) scan checkbox
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(scanCB, c);

    return container;
  }

  // controls container
  private Container getControlsContainer() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) run button
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(runB, c);

    // (1,0) close button
    c = new GridBagConstraints();
    c.insets = new Insets(0, 20, 0, 0);
    c.gridx = 1;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(closeB, c);

    return container;
  }
}


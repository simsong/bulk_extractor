import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.*;
import javax.swing.filechooser.FileFilter;  // not java.io.FileFilter
import java.io.File;
import java.util.Vector;
import java.util.Enumeration;

/**
 * The dialog window for starting a bulk_extractor scan process.
 * Multiple windows and scans may be active.
 * This window is not modal.
 */

public class WScan {
//  private static final long serialVersionUID = 1;

  // formatting
  public static final int NARROW_FIELD_WIDTH = 50;
  public static final int WIDE_FIELD_WIDTH = 100;
  public static final int EXTRA_WIDE_FIELD_WIDTH = 250;

  // Control
  private static WScan wScan;
  private final JDialog wScanWindow = new JDialog();
  private final JButton queueB = new JButton("Manage Queue\u2026");
  private final JButton importB = new JButton("Import\u2026");
  private final JButton submitB = new JButton("Submit Run");
  private final JButton cancelB = new JButton("Cancel");

  // boxed components
  private final WScanBoxedRequired wScanBoxedRequired;
  private final WScanBoxedGeneral wScanBoxedGeneral;
  private final WScanBoxedTuning wScanBoxedTuning;
  private final WScanBoxedControls wScanBoxedControls;
  private final WScanBoxedParallelizing wScanBoxedParallelizing;
  private final WScanBoxedDebugging wScanBoxedDebugging;
  private final WScanBoxedScanners wScanBoxedScanners;

  /**
   * Returns the window WScan runs from.
   * Popup windows may use this as their parent.
   */
  public static Component getWScanWindow() {
    return wScan.wScanWindow;
  }

  public static void openWindow(ScanSettings scanSettings) {
    if (wScan == null) {
      // this is the first invocation
      // create WScan
      new WScan();
    }

    if (wScan.wScanWindow.isVisible()) {
      WLog.log("WScan already visible.  Replacing scan settings.");
    }

    // set all the scan settings
    wScan.setScanSettings(scanSettings);

    // show the window
    wScan.wScanWindow.setLocationRelativeTo(BEViewer.getBEWindow());
    wScan.wScanWindow.getRootPane().setDefaultButton(wScan.submitB);
    wScan.wScanWindow.setVisible(true);
  }

  private WScan() {
    wScan = this;
    wScanBoxedRequired = new WScanBoxedRequired();
    wScanBoxedGeneral = new WScanBoxedGeneral();
    wScanBoxedTuning = new WScanBoxedTuning();
    wScanBoxedParallelizing = new WScanBoxedParallelizing();
    wScanBoxedDebugging = new WScanBoxedDebugging();
    wScanBoxedControls = new WScanBoxedControls();
    wScanBoxedScanners = new WScanBoxedScanners();
    buildScanInterface();
    wireActions();
    wScanWindow.pack();
  }

  private void buildScanInterface() {
    wScanWindow.setTitle("Run bulk_extractor");
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();
    wScanWindow.getContentPane().add(container);


    GridBagConstraints c;
    container.setLayout(new GridBagLayout());

    // (0,0) options scrollpane on upper part
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;
    container.add(buildOptionsScrollPane(), c);

    // (0,1) control buttons on lower part
    c = new GridBagConstraints();
    c.insets = new Insets(10, 5, 10, 5);
    c.gridx = 0;
    c.gridy = 1;
    container.add(buildControlContainer(), c);
  }

  private JScrollPane buildOptionsScrollPane() {
    Container optionsContainer = buildOptionsContainer();
    JScrollPane optionsScrollPane = new JScrollPane(optionsContainer);

    // fix optionsScrollPane background to match component background, necessary only for Mac
    optionsScrollPane.getViewport().setBackground(wScanBoxedRequired.component.getBackground());

    return optionsScrollPane;
  }

  private Container buildOptionsContainer() {
    // container using GridBagLayout with GridBagConstraints
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    GridBagConstraints c;
    int y=0;

    // required parameters
    c = new GridBagConstraints();
    c.insets = new Insets(10, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedRequired.component, c);

    // General Options
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedGeneral.component, c);

    // Tuning Parameters
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedTuning.component, c);

    // Parallelizing
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedParallelizing.component, c);

    // Debugging
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedDebugging.component, c);

    // controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 10, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.gridwidth = 2;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedControls.component, c);

    // Scanners in second column
    c = new GridBagConstraints();
    c.insets = new Insets(10, 5, 5, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.gridheight = 6;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedScanners.component, c);

    return container;
  }

  private Component buildControlContainer() {
    // container using GridBagLayout with GridBagConstraints
    Container container = new Container();
    container.setLayout(new GridBagLayout());
    GridBagConstraints c;

    int x=0;
    // Queue...
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = x++;
    c.gridy = 0;
    container.add(queueB, c);
    queueB.setToolTipText("Manage the bulk_extractor Run Queue");

    // Import Settings...
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = x++;
    c.gridy = 0;
    container.add(importB, c);
    importB.setToolTipText("Import settings for a new bulk_extractor run");

    // Submit run
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = x++;
    c.gridy = 0;
    container.add(submitB, c);
    submitB.setToolTipText("Submit run to the bulk_extractor Run Queue");

    // Cancel
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = x++;
    c.gridy = 0;
    container.add(cancelB, c);

    return container;
  }

  // ************************************************************
  // low-level line components
  // ************************************************************
  public static void addOptionalFileLine(Container container, int y,
             JCheckBox checkBox, JTextField textField, JButton button) {
    GridBagConstraints c;

    // file checkbox (0,y)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(checkBox, c);

    // file textField (1,y)
    c = new GridBagConstraints();
    textField.setMinimumSize(new Dimension(EXTRA_WIDE_FIELD_WIDTH, textField.getPreferredSize().height));
    textField.setPreferredSize(new Dimension(EXTRA_WIDE_FIELD_WIDTH, textField.getPreferredSize().height));
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = y;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(textField, c);

    // file chooser (2,y)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 2;
    c.gridy = y;
    container.add(button, c);
  }

/*
  public static void addTextLine(Container container, int y,
             JLabel label, JTextField textField, int width) {
    GridBagConstraints c;

    // text label (0,y)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(label, c);

    // textField (1,y)
    textField.setMinimumSize(new Dimension(width, textField.getPreferredSize().height));
    textField.setPreferredSize(new Dimension(width, textField.getPreferredSize().height));
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = y;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(textField, c);
  }
*/

  public static void addOptionalTextLine(Container container, int y,
             JCheckBox checkBox, JTextField textField, int width) {
    GridBagConstraints c;

    // text checkbox (0,y)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(checkBox, c);

    // textField (1,y)
    textField.setMinimumSize(new Dimension(width, textField.getPreferredSize().height));
    textField.setPreferredSize(new Dimension(width, textField.getPreferredSize().height));
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = y;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(textField, c);
  }

  public static void addOptionLine(Container container, int y, JCheckBox checkBox) {
    GridBagConstraints c;

    // checkbox (0,y)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(checkBox, c);
  }

  // copy values from scanSettings into UI
  private void setScanSettings(ScanSettings scanSettings) {
    wScanBoxedRequired.setScanSettings(scanSettings);
    wScanBoxedGeneral.setScanSettings(scanSettings);
    wScanBoxedTuning.setScanSettings(scanSettings);
    wScanBoxedParallelizing.setScanSettings(scanSettings);
    wScanBoxedDebugging.setScanSettings(scanSettings);
    wScanBoxedControls.setScanSettings(scanSettings);
    wScanBoxedScanners.setScanSettings(scanSettings);
  }

  // copy values from UI into scanSettings
  private void getScanSettings(ScanSettings scanSettings) {
    wScanBoxedRequired.getScanSettings(scanSettings);
    wScanBoxedGeneral.getScanSettings(scanSettings);
    wScanBoxedTuning.getScanSettings(scanSettings);
    wScanBoxedParallelizing.getScanSettings(scanSettings);
    wScanBoxedDebugging.getScanSettings(scanSettings);
    wScanBoxedControls.getScanSettings(scanSettings);
    wScanBoxedScanners.getScanSettings(scanSettings);
  }

  public static class FileChooserActionListener implements ActionListener {

    // read/write file/directory/image
    public final int READ_FILE = 0;
    public final int READ_DIRECTORY = 1;
    public final int WRITE_FILE = 2;
    public final int WRITE_DIRECTORY = 3;
    public final int READ_IMAGE_FILE = 4;

    private final String title;
    private final int dialogType;
    private final int fileSelectionMode;
    private final JTextField textField;
    private final boolean preferImageFile;

    public FileChooserActionListener(String title, JTextField textField, int mode) {
      this.title = title;
      this.textField = textField;

      // set properties for chooser based on mode
      if (mode == READ_FILE) {
        dialogType = JFileChooser.OPEN_DIALOG;
        fileSelectionMode = JFileChooser.FILES_ONLY;
        preferImageFile = false;
      } else if (mode == READ_DIRECTORY) {
        dialogType = JFileChooser.OPEN_DIALOG;
        fileSelectionMode = JFileChooser.DIRECTORIES_ONLY;
        preferImageFile = false;
      } else if (mode == WRITE_FILE) {
        dialogType = JFileChooser.SAVE_DIALOG;
        fileSelectionMode = JFileChooser.FILES_AND_DIRECTORIES;
        preferImageFile = false;
      } else if (mode == WRITE_DIRECTORY) {
        dialogType = JFileChooser.SAVE_DIALOG;
        fileSelectionMode = JFileChooser.FILES_AND_DIRECTORIES;
        preferImageFile = false;
      } else if (mode == READ_IMAGE_FILE) {
        dialogType = JFileChooser.OPEN_DIALOG;
        fileSelectionMode = JFileChooser.FILES_ONLY;
        preferImageFile = true;
      } else {
        throw new RuntimeException("Invalid mode");
      }
    }

    public void actionPerformed(ActionEvent e) {
      JFileChooser chooser = new JFileChooser();
      chooser.setDialogTitle(title);
      chooser.setDialogType(dialogType);
      chooser.setFileSelectionMode(fileSelectionMode);
      chooser.setSelectedFile(new File(textField.getText()));
      if (preferImageFile) {
        chooser.setFileFilter(ImageFileType.imageFileFilter);
      }

      // if the user selects APPROVE then take the text
      if (chooser.showOpenDialog(wScan.wScanWindow) == JFileChooser.APPROVE_OPTION) {
        // put the text in the text field
        String directoryString = chooser.getSelectedFile().getAbsolutePath();
        textField.setText(directoryString);
      }
    }
  }

  private void wireActions() {
    // queue
    queueB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        // put selection values into variables and close this window
        WScanSettingsRunQueue.openWindow();
      }
    });

    // submit
    submitB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {

        // get the command settings from the UI
        ScanSettings scanSettings = new ScanSettings();
        getScanSettings(scanSettings);

        WLog.log("WScan submit scan settings: '" + scanSettings.getCommandString() + "'\n");

        // validate some of the settings
        boolean success = scanSettings.validateSomeSettings();
        if (!success) {

          // abort request
          return;
        }

        // enqueue job onto the run queue
        BEViewer.scanSettingsListModel.add(scanSettings);

        // close this window
        wScanWindow.setVisible(false);
      }
    });

    // import
    importB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        // get the command settings from the UI
        ScanSettings scanSettings = new ScanSettings();
        wScan.getScanSettings(scanSettings);
        String commandString = scanSettings.getCommandString();

        // open the import settings window with existing settings
        new WImportScanSettings(commandString);
      }
    });

    // cancel
    cancelB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        // put selection values into variables and close this window
        wScanWindow.setVisible(false);
      }
    });
  }
}


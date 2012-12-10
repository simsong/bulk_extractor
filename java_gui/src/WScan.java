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
  public static final int FILE_FIELD_WIDTH = 250;
  public static final int NARROW_FIELD_WIDTH = 50;
  public static final int WIDE_FIELD_WIDTH = 100;

  // Control
  private static WScan wScan;
  private final JDialog wScanWindow = new JDialog();
  private final JButton loadDefaultsB = new JButton("Restore Defaults");
  private final JButton startB = new JButton("Start bulk_extractor");
  private final JButton cancelB = new JButton("Cancel");

  // boxed components
  private final WScanBoxedRequired wScanBoxedRequired;
  private final WScanBoxedGeneral wScanBoxedGeneral;
  private final WScanBoxedTuning wScanBoxedTuning;
  private final WScanBoxedScanners wScanBoxedScanners;
  private final WScanBoxedControls wScanBoxedControls;
  private final WScanBoxedParallelizing wScanBoxedParallelizing;
  private final WScanBoxedDebugging wScanBoxedDebugging;

  /**
   * Returns the window WScan runs from.
   * Popup windows may use this as their parent.
   */
  public static Component getWScanWindow() {
    return wScan.wScanWindow;
  }

  public static void openWindow() {
    if (wScan == null) {
      // this is the first invocation
      // create WScan
      new WScan();
    }

    // show the window
    wScan.wScanWindow.setLocationRelativeTo(BEViewer.getBEWindow());
    wScan.wScanWindow.getRootPane().setDefaultButton(wScan.startB);
    wScan.wScanWindow.setVisible(true);
  }

  private WScan() {
    wScan = this;
    wScanBoxedRequired = new WScanBoxedRequired();
    wScanBoxedGeneral = new WScanBoxedGeneral();
    wScanBoxedTuning = new WScanBoxedTuning();
    wScanBoxedScanners = new WScanBoxedScanners();
    wScanBoxedControls = new WScanBoxedControls();
    wScanBoxedDebugging = new WScanBoxedDebugging();
    wScanBoxedParallelizing = new WScanBoxedParallelizing();
    buildScanInterface();
    setDefaultValues();
    setUIValues();
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

    // controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.gridwidth = 2;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedControls.component, c);

    // Parallelizing
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedParallelizing.component, c);

    // Debugging
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 10, 5);
    c.gridx = 0;
    c.gridy = y++;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedDebugging.component, c);

    // Scanners in second column
    c = new GridBagConstraints();
    c.insets = new Insets(10, 5, 5, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.gridheight = 4;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(wScanBoxedScanners.component, c);

    return container;
  }

  private Component buildControlContainer() {
    // container using GridBagLayout with GridBagConstraints
    Container container = new Container();
    container.setLayout(new GridBagLayout());
    GridBagConstraints c;

    // (0,0) load defaults
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 0;
    container.add(loadDefaultsB, c);

    // (1,0) Start
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 0;
    container.add(startB, c);

    // (2,0) Cancel
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 2;
    c.gridy = 0;
    container.add(cancelB, c);

    return container;
  }

  // ************************************************************
  // integer helper
  // ************************************************************
  /**
   * read int from textField or else give error and use default value.
   */
  public static int getInt(JTextField textField, String name, int def) {
    try {
      int value = Integer.valueOf(textField.getText()).intValue();
      return value;
    } catch (NumberFormatException e) {
      WError.showError("Invalid input for " + name + ": " + textField.getText()
                       + ".\nUsing default value " + def + ".",
                       "bulk_extractor input error", null);
      return def;
    }
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
    textField.setMinimumSize(new Dimension(FILE_FIELD_WIDTH, textField.getPreferredSize().height));
    textField.setPreferredSize(new Dimension(FILE_FIELD_WIDTH, textField.getPreferredSize().height));
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

  private void setDefaultValues() {
    wScanBoxedRequired.setDefaultValues();
    wScanBoxedGeneral.setDefaultValues();
    wScanBoxedTuning.setDefaultValues();
    wScanBoxedScanners.setDefaultValues();
    wScanBoxedControls.setDefaultValues();
    wScanBoxedParallelizing.setDefaultValues();
    wScanBoxedDebugging.setDefaultValues();
  }

  private void setUIValues() {
    wScanBoxedRequired.setUIValues();
    wScanBoxedGeneral.setUIValues();
    wScanBoxedTuning.setUIValues();
    wScanBoxedScanners.setUIValues();
    wScanBoxedControls.setUIValues();
    wScanBoxedParallelizing.setUIValues();
    wScanBoxedDebugging.setUIValues();
  }

  private void getUIValues() {
    wScanBoxedRequired.getUIValues();
    wScanBoxedGeneral.getUIValues();
    wScanBoxedTuning.getUIValues();
    wScanBoxedScanners.getUIValues();
    wScanBoxedControls.getUIValues();
    wScanBoxedParallelizing.getUIValues();
    wScanBoxedDebugging.getUIValues();
  }

  private boolean validateRequiredParameters() {
    return wScanBoxedRequired.validateValues();
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

    // Control: defaults, start, cancel
    loadDefaultsB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setDefaultValues();
        setUIValues();
      }
    });

    startB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        getUIValues();

        // validate required parameters
        boolean success = validateRequiredParameters();
        if (!success) {
          // abort request
          return;
        }

        // start the scan and close this window
        String[] command = getCommand();
        new WScanProgress(wScanWindow, command);
        wScanWindow.setVisible(false);
      }
    });

    cancelB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        // put selection values into variables and close this window
        wScanWindow.setVisible(false);
      }
    });
  }

  private String[] getCommand() {
    Vector<String> cmd = new Vector<String>();

    // basic usage: bulk_extractor [options] imagefile
    // program name
    cmd.add("bulk_extractor");

    // options
    // required parameters
    cmd.add("-o");
    cmd.add(wScanBoxedRequired.outdir);
    
    // general options
    if (wScanBoxedGeneral.useBannerFile) {
      cmd.add("-b");
      cmd.add(wScanBoxedGeneral.bannerFile);
    }
    if (wScanBoxedGeneral.useAlertlistFile) {
      cmd.add("-r");
      cmd.add(wScanBoxedGeneral.alertlistFile);
    }
    if (wScanBoxedGeneral.useStoplistFile) {
      cmd.add("-w");
      cmd.add(wScanBoxedGeneral.stoplistFile);
    }
    if (wScanBoxedGeneral.useFindRegexTextFile) {
      cmd.add("-F");
      cmd.add(wScanBoxedGeneral.findRegexTextFile);
    }
    if (wScanBoxedGeneral.useFindRegexText) {
      cmd.add("-f");
      cmd.add(wScanBoxedGeneral.findRegexText);
    }

    // tuning parameters
    if (wScanBoxedTuning.useContextWindowSize) {
      cmd.add("-C");
      cmd.add(Integer.toString(wScanBoxedTuning.contextWindowSize));
    }
    if (wScanBoxedTuning.usePageSize) {
      cmd.add("-G");
      cmd.add(Integer.toString(wScanBoxedTuning.pageSize));
    }
    if (wScanBoxedTuning.useMarginSize) {
      cmd.add("-g");
      cmd.add(Integer.toString(wScanBoxedTuning.marginSize));
    }
    if ((wScanBoxedTuning.useMinWordSize && !wScanBoxedTuning.useMaxWordSize)
    || (!wScanBoxedTuning.useMinWordSize && wScanBoxedTuning.useMaxWordSize)) {
      // this is a program error so fail
      throw new RuntimeException("Invalid state");
    }
    if (wScanBoxedTuning.useMinWordSize || wScanBoxedTuning.useMaxWordSize) {
      // currently, bulk_extractor binds min and max together
      cmd.add("-W");
      cmd.add(Integer.toString(wScanBoxedTuning.minWordSize)
            + ":" + Integer.toString(wScanBoxedTuning.maxWordSize));
    }
    if (wScanBoxedTuning.useBlockSize) {
      cmd.add("-B");
      cmd.add(Integer.toString(wScanBoxedTuning.blockSize));
    }
    if (wScanBoxedTuning.useNumThreads) {
      cmd.add("-j");
      cmd.add(Integer.toString(wScanBoxedTuning.numThreads));
    }

    // controls
    if (wScanBoxedControls.usePluginDirectory) {
      cmd.add("-P");
      cmd.add(wScanBoxedControls.pluginDirectory);
    }
    if (wScanBoxedControls.useOptionName) {
      cmd.add("-s");
      cmd.add(wScanBoxedControls.optionName);
    }
    if (wScanBoxedControls.useMaxWait) {
      cmd.add("-m");
      cmd.add(wScanBoxedControls.maxWait);
    }

    // parallelizing
    if (wScanBoxedParallelizing.useStartProcessingAt) {
      cmd.add("-Y");
      cmd.add(wScanBoxedParallelizing.startProcessingAt);
    }
    if (wScanBoxedParallelizing.useProcessRange) {
      cmd.add("-Y");
      cmd.add(wScanBoxedParallelizing.processRange);
    }
    if (wScanBoxedParallelizing.useAddOffset) {
      cmd.add("-A");
      cmd.add(wScanBoxedParallelizing.addOffset);
    }

    // Debugging
    if (wScanBoxedDebugging.useMaxRecursionDepth) {
      cmd.add("-M");
      cmd.add(Integer.toString(wScanBoxedDebugging.maxRecursionDepth));
    }
    if (wScanBoxedDebugging.useStartOnPageNumber) {
      cmd.add("-z");
      cmd.add(Integer.toString(wScanBoxedDebugging.startOnPageNumber));
    }
    if (wScanBoxedDebugging.useDebugNumber) {
      cmd.add("-d" + Integer.toString(wScanBoxedDebugging.debugNumber));
    }
    if (wScanBoxedDebugging.useEraseOutputDirectory) {
      cmd.add("-Z");
    }
  
    // Scanners (feature recorders)
//    for (Enumeration<WScanBoxedScanners.FeatureScanner> e = (Enumeration<WScanBoxedScanners.FeatureScanner>)(featureScanners.elements()); e.hasMoreElements();); {
    for (Enumeration e = wScanBoxedScanners.featureScanners.elements(); e.hasMoreElements();) {
      WScanBoxedScanners.FeatureScanner featureScanner = (WScanBoxedScanners.FeatureScanner)e.nextElement();
      if (featureScanner.defaultUseScanner) {
        if (!featureScanner.useScanner) {
          // disable this scanner that is enabled by default
          cmd.add("-x");
          cmd.add(featureScanner.command);
        }
      } else {
        if (featureScanner.useScanner) {
          // enable this scanner that is disabled by default
          cmd.add("-e");
          cmd.add(featureScanner.command);
        }
      }
    }

    // required imagefile
    if (wScanBoxedRequired.imageSourceType == ImageSourceType.DIRECTORY_OF_FILES) {
      // recurse through a directory of files
      cmd.add("-R");
    }
    cmd.add(wScanBoxedRequired.inputImage);

    // compose the command string
    String[] command = cmd.toArray(new String[0]);

    // log the command string
    for (int i=0; i<command.length; i++) {
      WLog.log("cmd " + i + ": '" + command[i] + "'");
    }

    // return the command string
    return command;
  }
}


import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.io.File;

/**
 * The dialog window for importing opened Reports and Bookmarks.
 */

public class WImportWorkSettings extends JDialog {
  private static final long serialVersionUID = 1;

  private static WImportWorkSettings wImportWorkSettings;
  private JTextField filenameTF = new JTextField();
  private JButton fileChooserB = new FileChooserButton(BEViewer.getBEWindow(), "Import Work Settings From", FileChooserButton.READ_FILE, filenameTF);
  private JCheckBox keepExistingWorkSettingsCB = new JCheckBox("Keep Existing Work Settings");
  private JButton importB = new JButton("Import");
  private JButton cancelB = new JButton("Cancel");

/**
 * Opens this window.
 */
  public static void openWindow() {
    if (wImportWorkSettings == null) {
      // this is the first invocation
      // create the window
      wImportWorkSettings = new WImportWorkSettings();
    }

    // show the dialog window
    wImportWorkSettings.setLocationRelativeTo(BEViewer.getBEWindow());
    wImportWorkSettings.setVisible(true);
  }

  private WImportWorkSettings() {
    // set parent window, title, and modality

    buildInterface();
    wireActions();
    getRootPane().setDefaultButton(importB);
    pack();
  }

  private void buildInterface() {
    setTitle("Import Work Settings");
    setModal(true);
    setAlwaysOnTop(true);
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,0) add the filename input
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildFilePath(), c);

    // (0,1) add option control
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildOptionControl(), c);

    // (0,2) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 2;
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
    container.add(new JLabel("Work Settings File"), c);

    // file filenameTF (1,0)
    filenameTF.setMinimumSize(new Dimension(250, filenameTF.getPreferredSize().height));
    filenameTF.setPreferredSize(new Dimension(250, filenameTF.getPreferredSize().height));
    filenameTF.setToolTipText("Import opened Report and Bookmark settings from this file");
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(filenameTF, c);

    // file chooser (2,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 2;
    c.gridy = 0;
    container.add(fileChooserB, c);

    return container;
  }

  private Component buildOptionControl() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) keep existing settings checkbox
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    keepExistingWorkSettingsCB.setToolTipText("Do not clear opened Reports and Bookmarks that are already active");
    container.add(keepExistingWorkSettingsCB, c);

    return container;
  }

  private Component buildControls() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) importB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 0;
    container.add(importB, c);

    // (1,0) cancelB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(cancelB, c);

    return container;
  }

  private void wireActions() {

    // clicking importB imports the opened reports and the bookmarks settings
    importB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        File workSettingsFile = new File(filenameTF.getText());
        boolean keepExistingWorkSettings = keepExistingWorkSettingsCB.isSelected();
        boolean isSuccessful = BEPreferences.importWorkSettings(workSettingsFile, keepExistingWorkSettings);
        if (isSuccessful) {
          setVisible(false);
        }
      }
    });

    // clicking cancelB closes this window
    cancelB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setVisible(false);
      }
    });
  }
}


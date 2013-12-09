import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

/**
 * The dialog window for importing scan settings
 */

public class WImportScanSettings extends JDialog {
  private static final long serialVersionUID = 1;

  private static WImportScanSettings wImportScanSettings;
  private JTextField settingsTF = new JTextField();
  private JButton importB = new JButton("Import");
  private JButton cancelB = new JButton("Cancel");

/**
 * Opens this window.
 */
  public static void openWindow(String commandString) {
    if (wImportScanSettings == null) {
      // this is the first invocation
      // create the window
      wImportScanSettings = new WImportScanSettings();
    }

    // show the dialog window
    wImportScanSettings.setLocationRelativeTo(BEViewer.getBEWindow());
    wImportScanSettings.settingsTF.setText(commandString);
    wImportScanSettings.setVisible(true);
  }

  private WImportScanSettings() {
    // set parent window, title, and modality

    buildInterface();
    wireActions();
    getRootPane().setDefaultButton(importB);
    pack();
  }

  private void buildInterface() {
    setTitle("Import Scan Settings");
    setModal(true);
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,0) add the scan settings text input
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildSettingsInput(), c);

    // (0,1) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildControls(), c);
  }

  private Component buildSettingsInput() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // settings text label (0,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Settings Text"), c);

    // settingsTF (1,0)
    settingsTF.setMinimumSize(new Dimension(400, settingsTF.getPreferredSize().height));
    settingsTF.setPreferredSize(new Dimension(400, settingsTF.getPreferredSize().height));
    settingsTF.setToolTipText("Import settings from command line text");
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(settingsTF, c);

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

    // clicking importB imports the settings
    importB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettings scanSettings = new ScanSettings(settingsTF.getText());
        boolean success = scanSettings.validateSomeSettings();
        if (success) {
          WScan.openWindow(scanSettings);
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


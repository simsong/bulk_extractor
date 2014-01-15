import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

/**
 * The dialog window for importing scan settings
 */

public class WImportScanSettings extends JDialog {
  private static final long serialVersionUID = 1;

  private JTextField settingsTF = new JTextField();
  private JButton importB = new JButton("Import");
  private JButton cancelB = new JButton("Cancel");

/**
 * Dialog window for importing bulk_extractor run scan settings
 */
  public WImportScanSettings(String commandString) {
    setLocationRelativeTo(BEViewer.getBEWindow());
    buildInterface();
    wireActions();
    getRootPane().setDefaultButton(importB);
    settingsTF.setText(commandString);
    pack();
    setModal(true);
    setAlwaysOnTop(true);
    setVisible(true);
  }

  private void buildInterface() {
    setTitle("Import Scan Settings");
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,0) add the scan settings text input
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.fill=GridBagConstraints.HORIZONTAL;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildSettingsInput(), c);

    // (0,1) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 1;
//    c.anchor = GridBagConstraints.LINE_START;
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
//    settingsTF.setMinimumSize(new Dimension(600, settingsTF.getPreferredSize().height));
    settingsTF.setPreferredSize(new Dimension(600, settingsTF.getPreferredSize().height));
    settingsTF.setToolTipText("Import settings from command line text");
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.fill=GridBagConstraints.HORIZONTAL;
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

        if (scanSettings.validTokens
            && scanSettings.validateSomeSettings()) {

          // good, use it
          WScan.openWindow(scanSettings);
          setVisible(false);
        } else {
          // bad input, ignore import request
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


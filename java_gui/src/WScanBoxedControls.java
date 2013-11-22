import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.event.FocusListener;
import java.awt.event.FocusEvent;
import javax.swing.*;
import javax.swing.filechooser.FileFilter;  // not java.io.FileFilter
import java.util.Vector;
import java.util.Enumeration;

/**
 * Manage Scanner controls.
 */

public class WScanBoxedControls {

  public final Component component;

  public static final JCheckBox usePluginDirectoryCB = new JCheckBox("Use Plugin Directory");
  public static final JTextField pluginDirectoryTF = new JTextField();
//  private final JButton pluginDirectoryChooserB = new JButton("\u2026"); // ...
  private final FileChooserButton pluginDirectoryChooserB = new FileChooserButton(WScan.getWScanWindow(), "Select Plugin Directory", FileChooserButton.READ_DIRECTORY, pluginDirectoryTF);

  private final JCheckBox useSettableOptionsCB = new JCheckBox("Use Settable Options");
  private final JTextField settableOptionsTF = new JTextField();

  public WScanBoxedControls() {
    component = buildContainer();
    wireActions();
  }

  private Component buildContainer() {
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();
    container.setBorder(BorderFactory.createTitledBorder("Scanner Controls"));
    container.setLayout(new GridBagLayout());
    int y = 0;
    WScan.addOptionalFileLine(container, y++, usePluginDirectoryCB, pluginDirectoryTF,
                                        pluginDirectoryChooserB);
    WScan.addOptionalTextLine(container, y++, useSettableOptionsCB, settableOptionsTF, WScan.EXTRA_WIDE_FIELD_WIDTH);

    // tool tip text
    usePluginDirectoryCB.setToolTipText("Path to plugin directory");
    useSettableOptionsCB.setToolTipText("Provide settable options in form <key>=<value> separated by vertical bar character \"|\"");

    return container;
  }

  public void setScanSettings(ScanSettings scanSettings) {
    // controls
    usePluginDirectoryCB.setSelected(scanSettings.usePluginDirectory);
    pluginDirectoryTF.setEnabled(scanSettings.usePluginDirectory);
    pluginDirectoryTF.setText(scanSettings.pluginDirectory);
    pluginDirectoryChooserB.setEnabled(scanSettings.usePluginDirectory);

    useSettableOptionsCB.setSelected(scanSettings.useSettableOptions);
    settableOptionsTF.setEnabled(scanSettings.useSettableOptions);
    settableOptionsTF.setText(scanSettings.settableOptions);
  }

  public void getScanSettings(ScanSettings scanSettings) {
    // controls
    scanSettings.usePluginDirectory = usePluginDirectoryCB.isSelected();
    scanSettings.pluginDirectory = pluginDirectoryTF.getText();

    scanSettings.useSettableOptions = useSettableOptionsCB.isSelected();
    scanSettings.settableOptions = settableOptionsTF.getText();
  }

  // this listener keeps UI widget visibility up to date
  private class GetUIValuesActionListener implements ActionListener {
    public void actionPerformed(ActionEvent e) {
      ScanSettings scanSettings = new ScanSettings();
      getScanSettings(scanSettings);
      setScanSettings(scanSettings);
    }
  }

  private void wireActions() {
    // controls
    GetUIValuesActionListener getUIValuesActionListener
                  = new GetUIValuesActionListener();
    usePluginDirectoryCB.addActionListener(getUIValuesActionListener);
    useSettableOptionsCB.addActionListener(getUIValuesActionListener);
  }
}


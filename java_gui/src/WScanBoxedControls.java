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

  // defaults
  private static final String DEFAULT_MAX_WAIT = "60";

  // Controls
  public boolean usePluginDirectory;
  public boolean useSettableOptions;
  public boolean useMaxWait;

  public String pluginDirectory;
  public String settableOptions;
  public String maxWait;

  public static final JCheckBox usePluginDirectoryCB = new JCheckBox("Use Plugin Directory");
  public static final JTextField pluginDirectoryTF = new JTextField();
//  private final JButton pluginDirectoryChooserB = new JButton("\u2026"); // ...
  private final FileChooserButton pluginDirectoryChooserB = new FileChooserButton(WScan.getWScanWindow(), "Select Plugin Directory", FileChooserButton.READ_DIRECTORY, pluginDirectoryTF);

  private final JCheckBox useSettableOptionsCB = new JCheckBox("Use Settable Options");
  private final JTextField settableOptionsTF = new JTextField();

  private final JCheckBox useMaxWaitCB= new JCheckBox("Use Wait Time");
  private final JTextField maxWaitTF = new JTextField();

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
    WScan.addOptionalTextLine(container, y++, useMaxWaitCB, maxWaitTF, WScan.NARROW_FIELD_WIDTH);

    // tool tip text
    usePluginDirectoryCB.setToolTipText("Path to plugin directory");
    useSettableOptionsCB.setToolTipText("Provide settable options in form <key>=<value> separated by space");
    useMaxWaitCB.setToolTipText("Time, in minutes, to wait for memory starvation");

    return container;
  }

  public void setDefaultValues() {
    // Scanner Controls
    usePluginDirectory = false;
    useSettableOptions = false;
    maxWait = DEFAULT_MAX_WAIT;
    useMaxWait = false;
  }

  public void setUIValues() {
    // controls
    usePluginDirectoryCB.setSelected(usePluginDirectory);
    pluginDirectoryTF.setEnabled(usePluginDirectory);
    pluginDirectoryTF.setText(pluginDirectory);
    pluginDirectoryChooserB.setEnabled(usePluginDirectory);

    useSettableOptionsCB.setSelected(useSettableOptions);
    settableOptionsTF.setEnabled(useSettableOptions);
    settableOptionsTF.setText(settableOptions);

    useMaxWaitCB.setSelected(useMaxWait);
    maxWaitTF.setEnabled(useMaxWait);
    maxWaitTF.setText(maxWait);
  }

  public void getUIValues() {
    // controls
    usePluginDirectory = usePluginDirectoryCB.isSelected();
    pluginDirectory = pluginDirectoryTF.getText();

    useSettableOptions = useSettableOptionsCB.isSelected();
    settableOptions = settableOptionsTF.getText();

    useMaxWait = useMaxWaitCB.isSelected();
    maxWait = maxWaitTF.getText();
  }

  public boolean validateValues() {
    return true;
  }

  // the sole purpose of this listener is to keep UI widget visibility up to date
  private class GetUIValuesActionListener implements ActionListener {
    public void actionPerformed(ActionEvent e) {
      getUIValues();
      setUIValues();
    }
  }

  // these listeners keep the scanners list up to date
  private class SetScannerListActionListener implements ActionListener {
    public void actionPerformed(ActionEvent e) {
      WScanBoxedScanners.setScannerList();
    }
  }
  private class SetScannerListFocusListener implements FocusListener {
    public void focusGained(FocusEvent e) {
      // disable the scanners
      Component[] components = WScanBoxedScanners.container.getComponents();
      for (Component component : components) {
        component.setEnabled(false);
      }
    }
    public void focusLost(FocusEvent e) {
      // enable the scanners
      WScanBoxedScanners.setScannerList();
      Component[] components = WScanBoxedScanners.container.getComponents();
      for (Component component : components) {
        component.setEnabled(true);
      }
    }
  }

  private void wireActions() {
    // controls
    GetUIValuesActionListener getUIValuesActionListener
                  = new GetUIValuesActionListener();
    usePluginDirectoryCB.addActionListener(getUIValuesActionListener);
    useSettableOptionsCB.addActionListener(getUIValuesActionListener);
    useMaxWaitCB.addActionListener(getUIValuesActionListener);

    // controls to keep the scanners list up to date
    SetScannerListActionListener setScannerListActionListener
                  = new SetScannerListActionListener();
    usePluginDirectoryCB.addActionListener(setScannerListActionListener);

    SetScannerListFocusListener setScannerListFocusListener
                  = new SetScannerListFocusListener();
    pluginDirectoryTF.addFocusListener(setScannerListFocusListener);

    pluginDirectoryChooserB.chooser.addActionListener(setScannerListActionListener);
  }
}


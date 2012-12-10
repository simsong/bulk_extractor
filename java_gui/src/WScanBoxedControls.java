import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
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
  public boolean useOptionName;
  public boolean useMaxWait;

  public String pluginDirectory;
  public String optionName;
  public String maxWait;

  private final JCheckBox usePluginDirectoryCB = new JCheckBox("Use Plugin Directory");
  private final JTextField pluginDirectoryTF = new JTextField();
  private final JButton pluginDirectoryChooserB = new JButton("\u2026"); // ...

  private final JCheckBox useOptionNameCB = new JCheckBox("Use Scan Option Name");
  private final JTextField optionNameTF = new JTextField();

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
    WScan.addOptionalTextLine(container, y++, useOptionNameCB, optionNameTF, WScan.WIDE_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useMaxWaitCB, maxWaitTF, WScan.NARROW_FIELD_WIDTH);

    // tool tip text
    usePluginDirectoryCB.setToolTipText("Path to plugin directory");
    useOptionNameCB.setToolTipText("bulk_extractor option name in form <key>=<value>");
    useMaxWaitCB.setToolTipText("Time, in minutes, to wait for memory starvation");

    return container;
  }

  public void setDefaultValues() {
    // Scanner Controls
    usePluginDirectory = false;
    useOptionName = false;
    maxWait = DEFAULT_MAX_WAIT;
    useMaxWait = false;
  }

  public void setUIValues() {
    // controls
    usePluginDirectoryCB.setSelected(usePluginDirectory);
    pluginDirectoryTF.setEnabled(usePluginDirectory);
    pluginDirectoryTF.setText(pluginDirectory);
    pluginDirectoryChooserB.setEnabled(usePluginDirectory);

    useOptionNameCB.setSelected(useOptionName);
    optionNameTF.setEnabled(useOptionName);
    optionNameTF.setText(optionName);

    useMaxWaitCB.setSelected(useMaxWait);
    maxWaitTF.setEnabled(useMaxWait);
    maxWaitTF.setText(maxWait);
  }

  public void getUIValues() {
    // controls
    usePluginDirectory = usePluginDirectoryCB.isSelected();
    pluginDirectory = pluginDirectoryTF.getText();

    useOptionName = useOptionNameCB.isSelected();
    optionName = optionNameTF.getText();

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

  private void wireActions() {
    // controls
    GetUIValuesActionListener getUIValuesActionListener = new GetUIValuesActionListener();
    usePluginDirectoryCB.addActionListener(getUIValuesActionListener);
    useOptionNameCB.addActionListener(getUIValuesActionListener);
    useMaxWaitCB.addActionListener(getUIValuesActionListener);
  }
}


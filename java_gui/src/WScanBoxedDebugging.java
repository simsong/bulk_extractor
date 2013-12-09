import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.*;

/**
 * Manage the debugging values.
 */
public class WScanBoxedDebugging {

  public final Component component;
  private final JCheckBox useStartOnPageNumberCB = new JCheckBox("Start on Page Number");
  private final JTextField startOnPageNumberTF = new JTextField();
  private final JCheckBox useDebugNumberCB = new JCheckBox("Use Debug Mode Number");
  private final JTextField debugNumberTF = new JTextField();
  private final JCheckBox useEraseOutputDirectoryCB = new JCheckBox("Erase Output Directory");

  public WScanBoxedDebugging() {
    component = buildContainer();
    wireActions();
  }

  private Component buildContainer() {
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();
    container.setBorder(BorderFactory.createTitledBorder("Debugging Options"));
    container.setLayout(new GridBagLayout());
    int y = 0;
    WScan.addOptionalTextLine(container, y++, useStartOnPageNumberCB, startOnPageNumberTF, WScan.WIDE_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useDebugNumberCB, debugNumberTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionLine(container, y++, useEraseOutputDirectoryCB);

    return container;
  }

  public void setScanSettings(ScanSettings scanSettings) {
    // Debugging
    useStartOnPageNumberCB.setSelected(scanSettings.useStartOnPageNumber);
    startOnPageNumberTF.setEnabled(scanSettings.useStartOnPageNumber);
    startOnPageNumberTF.setText(scanSettings.startOnPageNumber);
    useDebugNumberCB.setSelected(scanSettings.useDebugNumber);
    debugNumberTF.setEnabled(scanSettings.useDebugNumber);
    debugNumberTF.setText(scanSettings.debugNumber);
    useEraseOutputDirectoryCB.setSelected(scanSettings.useEraseOutputDirectory);
  }

  public void getScanSettings(ScanSettings scanSettings) {
    // Debugging
    scanSettings.useStartOnPageNumber = useStartOnPageNumberCB.isSelected();
    scanSettings.startOnPageNumber = startOnPageNumberTF.getText();
    scanSettings.useDebugNumber = useDebugNumberCB.isSelected();
    scanSettings.debugNumber = debugNumberTF.getText();
    scanSettings.useEraseOutputDirectory = useEraseOutputDirectoryCB.isSelected();
  }

  // the sole purpose of this listener is to keep UI widget visibility up to date
  private class GetUIValuesActionListener implements ActionListener {
    public void actionPerformed(ActionEvent e) {
      ScanSettings scanSettings = new ScanSettings();
      getScanSettings(scanSettings);
      setScanSettings(scanSettings);
    }
  }

  private void wireActions() {
    // Debugging
    GetUIValuesActionListener getUIValuesActionListener = new GetUIValuesActionListener();
    useStartOnPageNumberCB.addActionListener(getUIValuesActionListener);
    useDebugNumberCB.addActionListener(getUIValuesActionListener);
    useEraseOutputDirectoryCB.addActionListener(getUIValuesActionListener);
  }
}


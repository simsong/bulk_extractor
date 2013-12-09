import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.*;

/**
 * Manage Tuning Parameters.
 */

public class WScanBoxedParallelizing {

  public final Component component;

  private final JCheckBox useStartProcessingAtCB = new JCheckBox("Use start processing at offset");
  private final JCheckBox useProcessRangeCB = new JCheckBox("Use process range offset o1-o2");
  private final JCheckBox useAddOffsetCB = new JCheckBox("Use add offset to reported feature offsets");

  private final JTextField startProcessingAtTF = new JTextField();
  private final JTextField processRangeTF = new JTextField();
  private final JTextField addOffsetTF = new JTextField();

  public WScanBoxedParallelizing() {
    component = buildContainer();
    wireActions();
  }

  private Component buildContainer() {
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();
    container.setBorder(BorderFactory.createTitledBorder("Parallelizing"));
    container.setLayout(new GridBagLayout());
    int y = 0;
    WScan.addOptionalTextLine(container, y++, useStartProcessingAtCB, startProcessingAtTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useProcessRangeCB, processRangeTF, WScan.WIDE_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useAddOffsetCB, addOffsetTF, WScan.NARROW_FIELD_WIDTH);

    // tooltip help
    useStartProcessingAtCB.setToolTipText("Start processing at offset, may use K, M, or G, for example 1G");
    useProcessRangeCB.setToolTipText("Process range, may use K, M, or G, for example 4G-7G");
    useAddOffsetCB.setToolTipText("Add offset to feature offset, may use K, M, or G, for example 1G");

    return container;
  }
 
  public void setScanSettings(ScanSettings scanSettings) {
    // tuning parameters
    useStartProcessingAtCB.setSelected(scanSettings.useStartProcessingAt);
    useProcessRangeCB.setSelected(scanSettings.useProcessRange);
    useAddOffsetCB.setSelected(scanSettings.useAddOffset);

    startProcessingAtTF.setEnabled(scanSettings.useStartProcessingAt);
    processRangeTF.setEnabled(scanSettings.useProcessRange);
    addOffsetTF.setEnabled(scanSettings.useAddOffset);

    startProcessingAtTF.setText(scanSettings.startProcessingAt);
    processRangeTF.setText(scanSettings.processRange);
    addOffsetTF.setText(scanSettings.addOffset);
  }

  public void getScanSettings(ScanSettings scanSettings) {
    // tuning parameters
    scanSettings.useStartProcessingAt = useStartProcessingAtCB.isSelected();
    scanSettings.useProcessRange = useProcessRangeCB.isSelected();
    scanSettings.useAddOffset = useAddOffsetCB.isSelected();

    scanSettings.startProcessingAt = startProcessingAtTF.getText();
    scanSettings.processRange = processRangeTF.getText();
    scanSettings.addOffset = addOffsetTF.getText();
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
    final ActionListener getUIValuesActionListener = new GetUIValuesActionListener();

    // tuning parameters
    useStartProcessingAtCB.addActionListener(new ActionListener() {
      // currently, bulk_extractor binds min and max together
      public void actionPerformed(ActionEvent e) {
        if (useStartProcessingAtCB.isSelected()) {
          useProcessRangeCB.setSelected(false);
          getUIValuesActionListener.actionPerformed(null);
        }
      }
    });
    useProcessRangeCB.addActionListener(new ActionListener() {
      // currently, bulk_extractor binds min and max together
      public void actionPerformed(ActionEvent e) {
        if (useProcessRangeCB.isSelected()) {
          useStartProcessingAtCB.setSelected(false);
          getUIValuesActionListener.actionPerformed(null);
        }
      }
    });
    useAddOffsetCB.addActionListener(getUIValuesActionListener);
  }
}


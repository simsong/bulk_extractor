import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.*;

/**
 * Manage Tuning Parameters.
 */

public class WScanBoxedTuning {

  public final Component component;

  private final JCheckBox useContextWindowSizeCB = new JCheckBox("Use Context Window Size");
  private final JCheckBox usePageSizeCB = new JCheckBox("Use Page Size");
  private final JCheckBox useMarginSizeCB = new JCheckBox("Use Margin Size");
  private final JCheckBox useBlockSizeCB = new JCheckBox("Use Block Size");
  private final JCheckBox useNumThreadsCB = new JCheckBox("Use Number of Threads");
  private final JCheckBox useMaxRecursionDepthCB = new JCheckBox("Use Maximum Recursion Depth");
  private final JCheckBox useMaxWaitCB= new JCheckBox("Use Wait Time");

  private final JTextField contextWindowSizeTF = new JTextField();
  private final JTextField pageSizeTF = new JTextField();
  private final JTextField marginSizeTF = new JTextField();
  private final JTextField blockSizeTF = new JTextField();
  private final JTextField numThreadsTF = new JTextField();
  private final JTextField maxRecursionDepthTF = new JTextField();
  private final JTextField maxWaitTF = new JTextField();

  public WScanBoxedTuning() {
    component = buildContainer();
    wireActions();
  }

  private Component buildContainer() {
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();
    container.setBorder(BorderFactory.createTitledBorder("Tuning Parameters"));
    container.setLayout(new GridBagLayout());
    int y = 0;
    WScan.addOptionalTextLine(container, y++, useContextWindowSizeCB, contextWindowSizeTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, usePageSizeCB, pageSizeTF, WScan.WIDE_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useMarginSizeCB, marginSizeTF, WScan.WIDE_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useBlockSizeCB, blockSizeTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useNumThreadsCB, numThreadsTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useMaxRecursionDepthCB, maxRecursionDepthTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useMaxWaitCB, maxWaitTF, WScan.NARROW_FIELD_WIDTH);

    // tool tip text
    useNumThreadsCB.setToolTipText("Number of Analysis Threads");
    useMaxWaitCB.setToolTipText("Time, in minutes, to wait for memory starvation");

    return container;
  }
 
  public void setScanSettings(ScanSettings scanSettings) {
    // tuning parameters
    useContextWindowSizeCB.setSelected(scanSettings.useContextWindowSize);
    usePageSizeCB.setSelected(scanSettings.usePageSize);
    useMarginSizeCB.setSelected(scanSettings.useMarginSize);
    useBlockSizeCB.setSelected(scanSettings.useBlockSize);
    useNumThreadsCB.setSelected(scanSettings.useNumThreads);
    useMaxRecursionDepthCB.setSelected(scanSettings.useMaxRecursionDepth);
    useMaxWaitCB.setSelected(scanSettings.useMaxWait);

    contextWindowSizeTF.setEnabled(scanSettings.useContextWindowSize);
    pageSizeTF.setEnabled(scanSettings.usePageSize);
    marginSizeTF.setEnabled(scanSettings.useMarginSize);
    blockSizeTF.setEnabled(scanSettings.useBlockSize);
    numThreadsTF.setEnabled(scanSettings.useNumThreads);
    maxRecursionDepthTF.setEnabled(scanSettings.useMaxRecursionDepth);
    maxWaitTF.setEnabled(scanSettings.useMaxWait);

    contextWindowSizeTF.setText(scanSettings.contextWindowSize);
    pageSizeTF.setText(scanSettings.pageSize);
    marginSizeTF.setText(scanSettings.marginSize);
    blockSizeTF.setText(scanSettings.blockSize);
    numThreadsTF.setText(scanSettings.numThreads);
    maxRecursionDepthTF.setText(scanSettings.maxRecursionDepth);
    maxWaitTF.setText(scanSettings.maxWait);
  }

  public void getScanSettings(ScanSettings scanSettings) {
    // tuning parameters
    scanSettings.useContextWindowSize = useContextWindowSizeCB.isSelected();
    scanSettings.usePageSize = usePageSizeCB.isSelected();
    scanSettings.useMarginSize = useMarginSizeCB.isSelected();
    scanSettings.useBlockSize = useBlockSizeCB.isSelected();
    scanSettings.useNumThreads = useNumThreadsCB.isSelected();
    scanSettings.useMaxRecursionDepth = useMaxRecursionDepthCB.isSelected();
    scanSettings.useMaxWait = useMaxWaitCB.isSelected();

    scanSettings.contextWindowSize = contextWindowSizeTF.getText();
    scanSettings.pageSize = pageSizeTF.getText();
    scanSettings.marginSize = marginSizeTF.getText();
    scanSettings.blockSize = blockSizeTF.getText();
    scanSettings.numThreads = numThreadsTF.getText();
    scanSettings.maxRecursionDepth = maxRecursionDepthTF.getText();
    scanSettings.maxWait = maxWaitTF.getText();
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
    useContextWindowSizeCB.addActionListener(getUIValuesActionListener);
    usePageSizeCB.addActionListener(getUIValuesActionListener);
    useMarginSizeCB.addActionListener(getUIValuesActionListener);
    useBlockSizeCB.addActionListener(getUIValuesActionListener);
    useNumThreadsCB.addActionListener(getUIValuesActionListener);
    useMaxRecursionDepthCB.addActionListener(getUIValuesActionListener);
    useMaxWaitCB.addActionListener(getUIValuesActionListener);
  }
}


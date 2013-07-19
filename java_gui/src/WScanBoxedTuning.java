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

  // defaults
  private static final int DEFAULT_CONTEXT_WINDOW_SIZE = 16;
  private static final int DEFAULT_PAGE_SIZE = 16777216;
  private static final int DEFAULT_MARGIN_SIZE = 4194304;
  private static final int DEFAULT_NUM_THREADS = 1;
  private static final int DEFAULT_BLOCK_SIZE = 512;
  private static final int DEFAULT_MAX_RECURSION_DEPTH = 7;
  private static final String DEFAULT_MAX_WAIT = "60";

  // tuning parameters
  public boolean useContextWindowSize;
  public boolean usePageSize;
  public boolean useMarginSize;
  public boolean useBlockSize;
  public boolean useNumThreads;
  public boolean useMaxRecursionDepth;
  public boolean useMaxWait;

  public int contextWindowSize;
  public int pageSize;
  public int marginSize;
  public int blockSize;
  public int numThreads;
  public int maxRecursionDepth;
  public String maxWait;

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
    setDefaultValues();
    setUIValues();
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
 
  public void setDefaultValues() {
    // tuning parameters
    useContextWindowSize = false;
    usePageSize = false;
    useMarginSize = false;
    useBlockSize = false;
    useNumThreads = false;
    useMaxRecursionDepth = false;
    useMaxWait = false;
 
    contextWindowSize = DEFAULT_CONTEXT_WINDOW_SIZE;
    pageSize = DEFAULT_PAGE_SIZE;
    marginSize = DEFAULT_MARGIN_SIZE;
    blockSize = DEFAULT_BLOCK_SIZE;
    numThreads = DEFAULT_NUM_THREADS;
    maxRecursionDepth = DEFAULT_MAX_RECURSION_DEPTH;
    maxWait = DEFAULT_MAX_WAIT;
  }

  public void setUIValues() {
    // tuning parameters
    useContextWindowSizeCB.setSelected(useContextWindowSize);
    usePageSizeCB.setSelected(usePageSize);
    useMarginSizeCB.setSelected(useMarginSize);
    useBlockSizeCB.setSelected(useBlockSize);
    useNumThreadsCB.setSelected(useNumThreads);
    useMaxRecursionDepthCB.setSelected(useMaxRecursionDepth);
    useMaxWaitCB.setSelected(useMaxWait);

    contextWindowSizeTF.setEnabled(useContextWindowSize);
    pageSizeTF.setEnabled(usePageSize);
    marginSizeTF.setEnabled(useMarginSize);
    blockSizeTF.setEnabled(useBlockSize);
    numThreadsTF.setEnabled(useNumThreads);
    maxRecursionDepthTF.setEnabled(useMaxRecursionDepth);
    maxWaitTF.setEnabled(useMaxWait);

    contextWindowSizeTF.setText(Integer.toString(contextWindowSize));
    pageSizeTF.setText(Integer.toString(pageSize));
    marginSizeTF.setText(Integer.toString(marginSize));
    blockSizeTF.setText(Integer.toString(blockSize));
    numThreadsTF.setText(Integer.toString(numThreads));
    maxRecursionDepthTF.setText(Integer.toString(maxRecursionDepth));
    maxWaitTF.setText(maxWait);
  }

  public void getUIValues() {
    // tuning parameters
    useContextWindowSize = useContextWindowSizeCB.isSelected();
    usePageSize = usePageSizeCB.isSelected();
    useMarginSize = useMarginSizeCB.isSelected();
    useBlockSize = useBlockSizeCB.isSelected();
    useNumThreads = useNumThreadsCB.isSelected();
    useMaxRecursionDepth = useMaxRecursionDepthCB.isSelected();
    useMaxWait = useMaxWaitCB.isSelected();

    contextWindowSize = WScan.getInt(contextWindowSizeTF, "context window size", DEFAULT_CONTEXT_WINDOW_SIZE);
    pageSize = WScan.getInt(pageSizeTF, "page size", DEFAULT_PAGE_SIZE);
    marginSize = WScan.getInt(marginSizeTF, "margin size", DEFAULT_MARGIN_SIZE);
    blockSize = WScan.getInt(blockSizeTF, "block size", DEFAULT_BLOCK_SIZE);
    numThreads = WScan.getInt(numThreadsTF, "number of threads", DEFAULT_NUM_THREADS);
    maxRecursionDepth = WScan.getInt(maxRecursionDepthTF, "maximum recursion depth", DEFAULT_MAX_RECURSION_DEPTH);
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


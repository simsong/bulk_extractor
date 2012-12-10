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
  private static final int DEFAULT_MARGIN_SIZE = 1048576;
  private static final int DEFAULT_MIN_WORD_SIZE = 6;
  private static final int DEFAULT_MAX_WORD_SIZE = 14;
  private static final int DEFAULT_NUM_THREADS = 1;
  private static final int DEFAULT_BLOCK_SIZE = 512;

  // tuning parameters
  public boolean useContextWindowSize;
  public boolean usePageSize;
  public boolean useMarginSize;
  public boolean useMinWordSize;
  public boolean useMaxWordSize;
  public boolean useBlockSize;
  public boolean useNumThreads;

  public int contextWindowSize;
  public int pageSize;
  public int marginSize;
  public int minWordSize;
  public int maxWordSize;
  public int blockSize;
  public int numThreads;

  private final JCheckBox useContextWindowSizeCB = new JCheckBox("Use Context Window Size");
  private final JCheckBox usePageSizeCB = new JCheckBox("Use Page Size");
  private final JCheckBox useMarginSizeCB = new JCheckBox("Use Margin Size");
  private final JCheckBox useMinWordSizeCB = new JCheckBox("Use Min Word Size");
  private final JCheckBox useMaxWordSizeCB = new JCheckBox("Use Max Word Size");
  private final JCheckBox useBlockSizeCB = new JCheckBox("Use Block Size");
  private final JCheckBox useNumThreadsCB = new JCheckBox("Use Number of Threads");

  private final JTextField contextWindowSizeTF = new JTextField();
  private final JTextField pageSizeTF = new JTextField();
  private final JTextField marginSizeTF = new JTextField();
  private final JTextField minWordSizeTF = new JTextField();
  private final JTextField maxWordSizeTF = new JTextField();
  private final JTextField blockSizeTF = new JTextField();
  private final JTextField numThreadsTF = new JTextField();

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
    WScan.addOptionalTextLine(container, y++, useMinWordSizeCB, minWordSizeTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useMaxWordSizeCB, maxWordSizeTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useBlockSizeCB, blockSizeTF, WScan.NARROW_FIELD_WIDTH);
    WScan.addOptionalTextLine(container, y++, useNumThreadsCB, numThreadsTF, WScan.NARROW_FIELD_WIDTH);

    return container;
  }
 
  public void setDefaultValues() {
    // tuning parameters
    useContextWindowSize = false;
    usePageSize = false;
    useMarginSize = false;
    useMinWordSize = false;
    useMaxWordSize = false;
    useBlockSize = false;
    useNumThreads = false;

    contextWindowSize = DEFAULT_CONTEXT_WINDOW_SIZE;
    pageSize = DEFAULT_PAGE_SIZE;
    marginSize = DEFAULT_MARGIN_SIZE;
    minWordSize = DEFAULT_MIN_WORD_SIZE;
    maxWordSize = DEFAULT_MAX_WORD_SIZE;
    blockSize = DEFAULT_BLOCK_SIZE;
    numThreads = DEFAULT_NUM_THREADS;
  }

  public void setUIValues() {
    // tuning parameters
    useContextWindowSizeCB.setSelected(useContextWindowSize);
    usePageSizeCB.setSelected(usePageSize);
    useMarginSizeCB.setSelected(useMarginSize);
    useMinWordSizeCB.setSelected(useMinWordSize);
    useMaxWordSizeCB.setSelected(useMaxWordSize);
    useBlockSizeCB.setSelected(useBlockSize);
    useNumThreadsCB.setSelected(useNumThreads);

    contextWindowSizeTF.setEnabled(useContextWindowSize);
    pageSizeTF.setEnabled(usePageSize);
    marginSizeTF.setEnabled(useMarginSize);
    minWordSizeTF.setEnabled(useMinWordSize);
    maxWordSizeTF.setEnabled(useMaxWordSize);
    blockSizeTF.setEnabled(useBlockSize);
    numThreadsTF.setEnabled(useNumThreads);

    contextWindowSizeTF.setText(Integer.toString(contextWindowSize));
    pageSizeTF.setText(Integer.toString(pageSize));
    marginSizeTF.setText(Integer.toString(marginSize));
    minWordSizeTF.setText(Integer.toString(minWordSize));
    maxWordSizeTF.setText(Integer.toString(maxWordSize));
    blockSizeTF.setText(Integer.toString(blockSize));
    numThreadsTF.setText(Integer.toString(numThreads));
  }

  public void getUIValues() {
    // tuning parameters
    useContextWindowSize = useContextWindowSizeCB.isSelected();
    usePageSize = usePageSizeCB.isSelected();
    useMarginSize = useMarginSizeCB.isSelected();
    useMinWordSize = useMinWordSizeCB.isSelected();
    useMaxWordSize = useMaxWordSizeCB.isSelected();
    useBlockSize = useBlockSizeCB.isSelected();
    useNumThreads = useNumThreadsCB.isSelected();

    contextWindowSize = WScan.getInt(contextWindowSizeTF, "context window size", DEFAULT_CONTEXT_WINDOW_SIZE);
    pageSize = WScan.getInt(pageSizeTF, "page size", DEFAULT_PAGE_SIZE);
    marginSize = WScan.getInt(marginSizeTF, "margin size", DEFAULT_MARGIN_SIZE);
    minWordSize = WScan.getInt(minWordSizeTF, "minimum word size", DEFAULT_MIN_WORD_SIZE);
    maxWordSize = WScan.getInt(maxWordSizeTF, "maximum word size", DEFAULT_MAX_WORD_SIZE);
    blockSize = WScan.getInt(blockSizeTF, "block size", DEFAULT_BLOCK_SIZE);
    numThreads = WScan.getInt(numThreadsTF, "number of threads", DEFAULT_NUM_THREADS);
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
    useMinWordSizeCB.addActionListener(new ActionListener() {
      // currently, bulk_extractor binds min and max together
      public void actionPerformed(ActionEvent e) {
        useMaxWordSizeCB.setSelected(useMinWordSizeCB.isSelected());
        getUIValuesActionListener.actionPerformed(null);
      }
    });
    useMaxWordSizeCB.addActionListener(new ActionListener() {
      // currently, bulk_extractor binds min and max together
      public void actionPerformed(ActionEvent e) {
        useMinWordSizeCB.setSelected(useMaxWordSizeCB.isSelected());
        getUIValuesActionListener.actionPerformed(null);
      }
    });
    useBlockSizeCB.addActionListener(getUIValuesActionListener);
    useNumThreadsCB.addActionListener(getUIValuesActionListener);
  }
}


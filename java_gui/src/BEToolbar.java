import java.awt.Dimension;
import java.awt.Color;
import java.awt.Container;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.util.Observer;
import java.util.Observable;
import javax.swing.BoxLayout;
import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JToolBar;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.event.DocumentListener;
import javax.swing.event.DocumentEvent;

public class BEToolbar extends JToolBar {

  // close
//  private final JButton closeToolbarB = new JButton(BEIcons.CLOSE_24);
  private final JButton closeToolbarB = new JButton(BEIcons.CLOSE_16);

  // shortcuts
  private final JButton openReportB = new JButton(BEIcons.OPEN_REPORT_24);
  private final JButton runB = new JButton(BEIcons.RUN_BULK_EXTRACTOR_24);
  private final JButton copyB = new JButton(BEIcons.COPY_24);
  private final JButton exportB = new JButton(BEIcons.EXPORT_BOOKMARKS_24);
  private final JButton printB = new JButton(BEIcons.PRINT_FEATURE_24);

  // highlight
  private final JTextField highlightTF = new JTextField();
  private final JCheckBox matchCaseCB = new JCheckBox("Match case");
  private Color defaultHighlightTFBackgroundColor = null;

  // resources
  private final ModelChangedNotifier<Object> toolbarChangedNotifier = new ModelChangedNotifier<Object>();

  /**
   * This notifies when visibility changes.
   */
   public void setVisible(boolean visible) {
    super.setVisible(visible);
    toolbarChangedNotifier.fireModelChanged(null);
  }

  // this simply gets this interface to appear in javadocs
  public boolean isVisible() {
    return super.isVisible();
  }

  public BEToolbar() {
    super ("BEViewer Toolbar", JToolBar.HORIZONTAL);
    setFloatable(false); // disabled because it looks bad

    // close Toolbar button
    closeToolbarB.setFocusable(false);
    closeToolbarB.setOpaque(false);
    closeToolbarB.setBorderPainted(false);
    closeToolbarB.setToolTipText("Close toolbar");
    closeToolbarB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        setVisible(false);
      }
    });
    add(closeToolbarB);

    // separator
    addSeparator(new Dimension(30, 0));

    // shortcuts
    addShortcutsControl();

    // separator
    addSeparator(new Dimension(30, 0));

    // highlight control
    addHighlightControl();

    // separator
    addSeparator(new Dimension(30, 0));

    // bookmark control
//    addBookmarkControl();
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param toolbarChangedListener the <code>Observer</code> to be added
   */
  public void addToolbarChangedListener(Observer toolbarChangedListener) {
    toolbarChangedNotifier.addObserver(toolbarChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param toolbarChangedListener the <code>Observer</code> to be removed
   */
  public void removeToolbarChangedListener(Observer toolbarChangedListener) {
    toolbarChangedNotifier.deleteObserver(toolbarChangedListener);
  }

  // ************************************************************
  // shortcuts
  // ************************************************************
  private void addShortcutsControl() {

    // open Report
    openReportB.setFocusable(false);
    openReportB.setOpaque(false);
    openReportB.setBorderPainted(false);
    openReportB.setToolTipText("Open a Report for browsing");
    openReportB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WOpen.openWindow();
      }
    });
    add(openReportB);

    // run bulk_extractor
    runB.setFocusable(false);
    runB.setOpaque(false);
    runB.setBorderPainted(false);
    runB.setToolTipText("Generate a Report using bulk_extractor");
    runB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WScan.openWindow();
      }
    });
    add(runB);

    // separator
    add(Box.createRigidArea(new Dimension(BEViewer.GUI_X_PADDING, 0)));

    // Copy
    copyB.setFocusable(false);
    copyB.setOpaque(false);
    copyB.setBorderPainted(false);
    copyB.setToolTipText("Copy Feature or Image selection to the System Clipboard");
    copyB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        RangeSelectionManager.setSystemClipboard(BEViewer.rangeSelectionManager.getSelection());
      }
    });
    copyB.setEnabled(false);

    // wire listener to manage when a buffer is available to copy
    BEViewer.rangeSelectionManager.addRangeSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        copyB.setEnabled(BEViewer.rangeSelectionManager.hasSelection());
      }
    });
    add(copyB);

    // export bookmarks
    exportB.setFocusable(false);
    exportB.setOpaque(false);
    exportB.setBorderPainted(false);
    exportB.setEnabled(false);
    exportB.setToolTipText("Export bookmarks");
    exportB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WBookmarks.openWindow();
      }
    });

    // wire listener to manage when bookmarks are available for export
    BEViewer.featureBookmarksModel.addBookmarksModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        exportB.setEnabled(BEViewer.featureBookmarksModel.size() > 0);
      }
    });
    add(exportB);

    // print selected Feature
    printB.setFocusable(false);
    printB.setOpaque(false);
    printB.setBorderPainted(false);
    printB.setEnabled(false);
    printB.setToolTipText("Print the selected range");
    printB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.printRange();
      }
    });
    // wire listener to know when a feature is available for printing
    BEViewer.rangeSelectionManager.addRangeSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        printB.setEnabled(BEViewer.rangeSelectionManager.hasSelection());
      }
    });
    add(printB);
  }

  // ************************************************************
  // highlight
  // ************************************************************
  private void addHighlightControl() {
//    Container container = new Container();
//    container.setLayout(new BoxLayout(container, BoxLayout.X_AXIS));

    // ************************************************************
    // "Highlight:" label
    // ************************************************************
    add(new JLabel("Highlight:"));

    // separator
    add(Box.createRigidArea(new Dimension(BEViewer.GUI_X_PADDING, 0)));

    // ************************************************************
    // highlight input text field
    // ************************************************************
    defaultHighlightTFBackgroundColor = highlightTF.getBackground();

    // require a fixed size
    Dimension highlightDimension = new Dimension(220, highlightTF.getPreferredSize().height);
    highlightTF.setMinimumSize(highlightDimension);
    highlightTF.setPreferredSize(highlightDimension);
    highlightTF.setMaximumSize(highlightDimension);

    // create the JTextField highlightTF for containing the highlight text
    highlightTF.setToolTipText("Type Text to Highlight, type \"|\" between words to enter multiple Highlights");

    // change the user highlight model when highlightTF changes
    highlightTF.getDocument().addDocumentListener(new DocumentListener() {
      private void setHighlightString() {
        // and set the potentially escaped highlight text in the image model
        byte[] highlightBytes = highlightTF.getText().getBytes(UTF8Tools.UTF_8);
        BEViewer.userHighlightModel.setHighlightBytes(highlightBytes);

        // also make the background yellow while there is text in highlightTF
        if (highlightBytes.length > 0) {
          highlightTF.setBackground(Color.YELLOW);
        } else {
          highlightTF.setBackground(defaultHighlightTFBackgroundColor);
        }
      }
      // JTextField responds to Document change
      public void insertUpdate(DocumentEvent e) {
        setHighlightString();
      }
      public void removeUpdate(DocumentEvent e) {
        setHighlightString();
      }
      public void changedUpdate(DocumentEvent e) {
        setHighlightString();
      }
    });

    // add the highlight text textfield
    add(highlightTF);

    // separator
    add(Box.createRigidArea(new Dimension(10, 0)));

    // ************************************************************
    // Match Case checkbox
    // ************************************************************
    // set initial value
    matchCaseCB.setSelected(BEViewer.userHighlightModel.isHighlightMatchCase());

    // set up the highlight match case checkbox
    matchCaseCB.setFocusable(false);
    matchCaseCB.setOpaque(false); // this looks better with the ToolBar's gradient
    matchCaseCB.setRequestFocusEnabled(false);
    matchCaseCB.setToolTipText("Match case in highlight text");

    // match case action listener
    matchCaseCB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // toggle the case setting
        BEViewer.userHighlightModel.setHighlightMatchCase(matchCaseCB.isSelected());

        // as a convenience, change focus to highlightTF
        highlightTF.requestFocusInWindow();
      }
    });

    // wire listener to keep view in sync with the model
    BEViewer.userHighlightModel.addUserHighlightModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        // checkbox
        matchCaseCB.setSelected(BEViewer.userHighlightModel.isHighlightMatchCase());

        // text field, which can include user escape codes
        byte[] fieldBytes = highlightTF.getText().getBytes(UTF8Tools.UTF_8);
        byte[] modelBytes = BEViewer.userHighlightModel.getHighlightBytes();
        if (!UTF8Tools.bytesMatch(fieldBytes, modelBytes)) {
          highlightTF.setText(new String(modelBytes));
        }
      }
    });

    // add the match case button
    add(matchCaseCB);
  }
}

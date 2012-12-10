import java.awt.Dimension;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.util.Observer;
import java.util.Observable;
import javax.swing.JButton;
import javax.swing.JToolBar;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JTextField;

public class BEShortcutsToolbar extends JToolBar {

//  private final JButton closeToolbarB = new JButton(BEIcons.CLOSE_24);
  private final JButton closeToolbarB = new JButton(BEIcons.CLOSE_16);
  private final JButton openReportB = new JButton(BEIcons.OPEN_REPORT_24);
  private final JButton runB = new JButton(BEIcons.RUN_BULK_EXTRACTOR_24);
  private final JButton copyB = new JButton(BEIcons.COPY_24);
  private final JButton exportB = new JButton(BEIcons.EXPORT_BOOKMARKS_24);
  private final JButton printB = new JButton(BEIcons.PRINT_FEATURE_24);

  // resources
  private final ModelChangedNotifier<Object> shortcutsToolbarChangedNotifier = new ModelChangedNotifier<Object>();

  /**
   * This notifies when visibility changes.
   */
   public void setVisible(boolean visible) {
    super.setVisible(visible);
    shortcutsToolbarChangedNotifier.fireModelChanged(null);
  }

  // this simply gets this interface to appear in javadocs
  public boolean isVisible() {
    return super.isVisible();
  }

  public BEShortcutsToolbar() {
    super ("BEViewer Toolbar", JToolBar.HORIZONTAL);
    setFloatable(false);

    // close Toolbar
    closeToolbarB.setFocusable(false);
    closeToolbarB.setOpaque(false);
    closeToolbarB.setBorderPainted(false);
    closeToolbarB.setToolTipText("Close shortcuts toolbar");
    closeToolbarB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        setVisible(false);
      }
    });
    add(closeToolbarB);

    // separator
    addSeparator();

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
    addSeparator();

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

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param shortcutsToolbarChangedListener the <code>Observer</code> to be added
   */
  public void addShortcutsToolbarChangedListener(Observer shortcutsToolbarChangedListener) {
    shortcutsToolbarChangedNotifier.addObserver(shortcutsToolbarChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param shortcutsToolbarChangedListener the <code>Observer</code> to be removed
   */
  public void removeShortcutsToolbarChangedListener(Observer shortcutsToolbarChangedListener) {
    shortcutsToolbarChangedNotifier.deleteObserver(shortcutsToolbarChangedListener);
  }
}

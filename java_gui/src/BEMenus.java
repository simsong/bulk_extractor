import javax.swing.JMenuBar;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JRadioButtonMenuItem;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.ButtonGroup;
import javax.swing.event.TreeModelListener;
import javax.swing.event.TreeModelEvent;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;
import javax.swing.event.ListDataListener;
import javax.swing.event.ListDataEvent;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;
import javax.swing.KeyStroke;
import java.awt.event.KeyEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.IOException;
import java.util.Observable;
import java.util.Observer;
import java.util.Vector;
import java.util.Enumeration;
import java.awt.Toolkit;
import java.awt.datatransfer.Clipboard;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.StringSelection;
import java.net.URL;

/**
 * The <code>BEViewer</code> class provides the main entry
 * for the Bulk Extractor Viewer application.
 */
public class BEMenus extends JMenuBar {

  private final long serialVersionUID = 1;
  private JCheckBoxMenuItem cbShowToolbar;
  private JRadioButtonMenuItem rbFeatureText;
  private JRadioButtonMenuItem rbTypedText;
  private JRadioButtonMenuItem rbTextView;
  private JRadioButtonMenuItem rbHexView;
  private JRadioButtonMenuItem rbDecimal;
  private JRadioButtonMenuItem rbHex;
  private JRadioButtonMenuItem rbReferencedFeaturesVisible;
  private JRadioButtonMenuItem rbReferencedFeaturesCollapsible;
  private JCheckBoxMenuItem cbShowStoplistFiles;
  private JCheckBoxMenuItem cbShowEmptyFiles;
  private JMenuItem miPrintFeature;
  private JMenuItem miClearBookmarks;
  private JMenuItem miExportBookmarks;
  private JMenuItem miCopy;
  private JMenuItem miClose;
  private JMenuItem miCloseAll;
  private JMenuItem miAddBookmark;
  private JMenuItem miManageBookmarks;
  private JMenuItem miPanToStart;
  private JMenuItem miPanToEnd;
  private JMenuItem miShowReportFile;
  private JMenuItem miShowLog;

  private final int KEY_MASK = Toolkit.getDefaultToolkit().getMenuShortcutKeyMask();
  private final KeyStroke KEYSTROKE_O = KeyStroke.getKeyStroke(KeyEvent.VK_O, KEY_MASK);
  private final KeyStroke KEYSTROKE_Q = KeyStroke.getKeyStroke(KeyEvent.VK_Q, KEY_MASK);
  private final KeyStroke KEYSTROKE_C = KeyStroke.getKeyStroke(KeyEvent.VK_C, KEY_MASK);
  private final KeyStroke KEYSTROKE_R = KeyStroke.getKeyStroke(KeyEvent.VK_R, KEY_MASK);
  private final KeyStroke KEYSTROKE_A = KeyStroke.getKeyStroke(KeyEvent.VK_A, KEY_MASK);
  private final KeyStroke KEYSTROKE_M = KeyStroke.getKeyStroke(KeyEvent.VK_M, KEY_MASK);
  private final KeyStroke KEYSTROKE_B = KeyStroke.getKeyStroke(KeyEvent.VK_B, KEY_MASK);
  private final KeyStroke KEYSTROKE_L = KeyStroke.getKeyStroke(KeyEvent.VK_L, KEY_MASK);

  // ********************************************************************************
  // Create the menus
  // ********************************************************************************
  /**
   * Creates the menus for BEViewer.
   */
  public BEMenus() {

    JMenuItem mi;

    // file
    JMenu file = new JMenu("File");
    add(file);

    // file Open
    mi = new JMenuItem("Open Report\u2026", BEIcons.OPEN_REPORT_16);	// ...
    file.add(mi);
    mi.setAccelerator(KEYSTROKE_O);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WOpen.openWindow();
      }
    });

    // file Close 
    miClose = new JMenuItem("Close Selected Report");
    file.add(miClose);
    miClose.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // get the currently selected report
        TreePath selectedTreePath = BEViewer.reportsModel.getSelectedTreePath();
        ReportsModel.ReportTreeNode reportTreeNode = ReportsModel.getReportTreeNodeFromTreePath(selectedTreePath);
        if (reportTreeNode != null) {
          BEViewer.closeReport(reportTreeNode);
        } else {
          WLog.log("BEMenus Close Report failure");
        }
      }
    });
    miClose.setEnabled(false);

    // wire listener to manage when miClose is enabled
    BEViewer.reportsModel.getTreeSelectionModel().addTreeSelectionListener(new TreeSelectionListener() {
      public void valueChanged(TreeSelectionEvent e) {
        // get the currently selected report
        TreePath selectedTreePath = BEViewer.reportsModel.getSelectedTreePath();
        ReportsModel.ReportTreeNode reportTreeNode = ReportsModel.getReportTreeNodeFromTreePath(selectedTreePath);
        if (reportTreeNode != null) {
          miClose.setEnabled(true);
        } else {
          miClose.setEnabled(false);
        }
      }
    });

    // file Close all
    miCloseAll = new JMenuItem("Close all Reports");
    file.add(miCloseAll);
    miCloseAll.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.closeAllReports();
      }
    });
    miCloseAll.setEnabled(false);

    // wire ReportsModel's report tree model listener to manage when miCloseAll is enabled
    BEViewer.reportsModel.getTreeModel().addTreeModelListener(new TreeModelListener() {
      public void treeNodesChanged(TreeModelEvent e) {
        setCloseAllVisibility();
      }
      public void treeNodesInserted(TreeModelEvent e) {
        setCloseAllVisibility();
      }
      public void treeNodesRemoved(TreeModelEvent e) {
        setCloseAllVisibility();
      }
      public void treeStructureChanged(TreeModelEvent e) {
        setCloseAllVisibility();
      }
      private void setCloseAllVisibility() {
        Enumeration<ReportsModel.ReportTreeNode> e = BEViewer.reportsModel.elements();
        miCloseAll.setEnabled(e.hasMoreElements());
      }
    });

    // file|<separator>
    file.addSeparator();

    // file|export bookmarks
    miExportBookmarks = new JMenuItem("Export Bookmarks\u2026", BEIcons.EXPORT_BOOKMARKS_16);	// ...
    file.add(miExportBookmarks);
    miExportBookmarks.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WManageBookmarks.saveBookmarks();
      }
    });

    // wire listener to manage when bookmarks are available for export
    BEViewer.bookmarksModel.bookmarksComboBoxModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        miExportBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
      public void intervalAdded(ListDataEvent e) {
        miExportBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
      public void intervalRemoved(ListDataEvent e) {
        miExportBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
    });
    miExportBookmarks.setEnabled(false);

    // file|<separator>
    file.addSeparator();

    // file|Import Work Settings
    mi = new JMenuItem("Import Work Settings\u2026");	// ...
    file.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WImportWorkSettings.openWindow();
      }
    });

    // file|Export Work Settings
    mi = new JMenuItem("Export Work Settings\u2026");	// ...
    file.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WExportWorkSettings.openWindow();
      }
    });

    // file|<separator>
    file.addSeparator();

    // file|Print Range
    miPrintFeature = new JMenuItem("Print Range\u2026", BEIcons.PRINT_FEATURE_16);	// ...
    miPrintFeature.setEnabled(false);
    file.add(miPrintFeature);
    miPrintFeature.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.printRange();
      }
    });
    // wire listener to know when a feature is available for printing
    BEViewer.rangeSelectionManager.addRangeSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        miPrintFeature.setEnabled(BEViewer.rangeSelectionManager.hasSelection());
      }
    });
 
    // file|<separator>
    file.addSeparator();

    // file Quit
    mi = new JMenuItem("Quit", BEIcons.EXIT_16);
    file.add(mi);
    mi.setAccelerator(KEYSTROKE_Q);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // close down BEViewer
        BEViewer.doClose();
      }
    });

    // edit
    JMenu edit = new JMenu("Edit");
    add(edit);

    // edit|copy
    miCopy = new JMenuItem("Copy", BEIcons.COPY_16);
    miCopy.setEnabled(false);
    edit.add(miCopy);
    miCopy.setAccelerator(KEYSTROKE_C);
    miCopy.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // put text onto System Clipboard
        RangeSelectionManager.setSystemClipboard(BEViewer.rangeSelectionManager.getSelection());
      }
    });

    // wire listener to manage when a buffer is available to copy
    BEViewer.rangeSelectionManager.addRangeSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        miCopy.setEnabled(BEViewer.rangeSelectionManager.hasSelection());
      }
    });

    // edit|<separator>
    edit.addSeparator();

    // edit|clear all bookmarks
    miClearBookmarks = new JMenuItem("Clear Bookmarks");
    edit.add(miClearBookmarks);
    miClearBookmarks.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.bookmarksModel.clear();
      }
    });
    miClearBookmarks.setEnabled(false);

    // wire listener to know when bookmarks are empty
    BEViewer.bookmarksModel.bookmarksComboBoxModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        miClearBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
      public void intervalAdded(ListDataEvent e) {
        miClearBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
      public void intervalRemoved(ListDataEvent e) {
        miClearBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
    });

    // view
    JMenu view = new JMenu("View");
    add(view);
    ButtonGroup imageGroup = new ButtonGroup();
    ButtonGroup forensicPathNumericBaseGroup = new ButtonGroup();
    ButtonGroup referencedFeaturesGroup = new ButtonGroup();

    // view|Toolbar
    cbShowToolbar = new JCheckBoxMenuItem("Toolbar");
    cbShowToolbar.setSelected(true);
    view.add(cbShowToolbar);
    cbShowToolbar.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.toolbar.setVisible(cbShowToolbar.isSelected());
      }
    });
    // wire listener to know when the toolbar is visible
    BEViewer.toolbar.addToolbarChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        cbShowToolbar.setSelected(BEViewer.toolbar.isVisible());
      }
    });

    // view|Image Format
    JMenu imageFormatMenu = new JMenu("Image Format");
    view.add(imageFormatMenu);

    // view|Image Format|Text
    rbTextView = new JRadioButtonMenuItem("Text");
    imageGroup.add(rbTextView);
    imageFormatMenu.add(rbTextView);
    rbTextView.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.imageView.setLineFormat(ImageLine.LineFormat.TEXT_FORMAT);
      }
    });

    // view|Image Format|Hex
    rbHexView = new JRadioButtonMenuItem("Hex");
    imageGroup.add(rbHexView);
    imageFormatMenu.add(rbHexView);
    rbHexView.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.imageView.setLineFormat(ImageLine.LineFormat.HEX_FORMAT);
      }
    });

    // wire listener to manage which image view type button is selected
    BEViewer.imageView.addImageViewChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        // this could be changed to act only on ImageView.ChangeType.FORENSIC_PATH_NUMERIC_BASE_CHANGED
        ImageLine.LineFormat lineFormat = BEViewer.imageView.getLineFormat();
        if (lineFormat == ImageLine.LineFormat.TEXT_FORMAT) {
          rbTextView.setSelected(true);
        } else if (lineFormat == ImageLine.LineFormat.HEX_FORMAT) {
          rbHexView.setSelected(true);
        } else {
          throw new RuntimeException("invalid image format source");
        }
      }
    });
    
    // view|Forensic Path Numeric Base
    JMenu forensicPathNumericBaseMenu = new JMenu("Forensic Path Numeric Base");
    view.add(forensicPathNumericBaseMenu);

    // view|Forensic Path Numeric Base|Decimal
    rbDecimal = new JRadioButtonMenuItem("Decimal");
    forensicPathNumericBaseGroup.add(rbDecimal);
    forensicPathNumericBaseMenu.add(rbDecimal);
    rbDecimal.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.featuresModel.setUseHexPath(false);
        BEViewer.referencedFeaturesModel.setUseHexPath(false);
        BEViewer.imageView.setUseHexPath(false);
        BEViewer.bookmarksModel.fireViewChanged();
      }
    });

    // view|Forensic Path Numeric Base|Hex
    rbHex = new JRadioButtonMenuItem("Hexadecimal");
    forensicPathNumericBaseGroup.add(rbHex);
    forensicPathNumericBaseMenu.add(rbHex);
    rbHex.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.featuresModel.setUseHexPath(true);
        BEViewer.referencedFeaturesModel.setUseHexPath(true);
        BEViewer.imageView.setUseHexPath(true);
        BEViewer.bookmarksModel.fireViewChanged();
      }
    });
    
    // wire listener to manage which forensic path numeric base is shown in the menu
    BEViewer.imageView.addImageViewChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        if (BEViewer.imageView.getChangeType() == ImageView.ChangeType.FORENSIC_PATH_NUMERIC_BASE_CHANGED) {
          boolean isHex = BEViewer.imageView.getUseHexPath();
          if (!isHex) {
            rbDecimal.setSelected(true);
          } else {
            rbHex.setSelected(true);
          }
        }
      }
    });
    
    // view|Feature Font Size
    JMenu featureFontSizeMenu = new JMenu("Feature Font Size");
    view.add(featureFontSizeMenu);

    // view|Feature Font Size|Zoom In
    mi = new JMenuItem("Zoom In");
    featureFontSizeMenu.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.featuresModel.setFontSize(BEViewer.featuresModel.getFontSize() + 1);
        BEViewer.referencedFeaturesModel.setFontSize(BEViewer.featuresModel.getFontSize());
      }
    });

    // view|Feature Font Size|Zoom Out
    mi = new JMenuItem("Zoom Out");
    featureFontSizeMenu.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        int oldFontSize = BEViewer.featuresModel.getFontSize();
        if (oldFontSize > 6) {
          BEViewer.featuresModel.setFontSize(oldFontSize - 1);
          BEViewer.referencedFeaturesModel.setFontSize(BEViewer.featuresModel.getFontSize());
        } else {
          WError.showError("Already at minimum font size of " + oldFontSize + ".",
                           "BEViewer Feature Font Size error", null);
        }
      }
    });

    // view|Feature File View|Normal Size
    mi = new JMenuItem("Normal Size");
    featureFontSizeMenu.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.featuresModel.setFontSize(BEPreferences.DEFAULT_FEATURE_FONT_SIZE);
        BEViewer.referencedFeaturesModel.setFontSize(BEViewer.featuresModel.getFontSize());
      }
    });

    // view|Image Font Size
    JMenu imageFontSizeMenu = new JMenu("Image Font Size");
    view.add(imageFontSizeMenu);

    // view|Image Font Size|Zoom In
    mi = new JMenuItem("Zoom In");
    imageFontSizeMenu.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.imageView.setFontSize(BEViewer.imageView.getFontSize() + 1);
      }
    });

    // view|Image Font Size|Zoom Out
    mi = new JMenuItem("Zoom Out");
    imageFontSizeMenu.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        int oldFontSize = BEViewer.imageView.getFontSize();
        if (oldFontSize > 6) {
          BEViewer.imageView.setFontSize(oldFontSize - 1);
        } else {
          WError.showError("Already at minimum font size of " + oldFontSize + ".",
                           "BEViewer Image Font Size Size error", null);
        }
      }
    });

    // view|Image Font Size|Normal Size
    mi = new JMenuItem("Normal Size");
    imageFontSizeMenu.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.imageView.setFontSize(BEPreferences.DEFAULT_IMAGE_FONT_SIZE);
      }
    });

    // view|Reports
    JMenu reportsMenu = new JMenu("Reports");
    view.add(reportsMenu);

    // view|Reports|Show Stoplist Files
    cbShowStoplistFiles = new JCheckBoxMenuItem("Show Stoplist Files");
    reportsMenu.add(cbShowStoplistFiles);
    cbShowStoplistFiles.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.reportsModel.setIncludeStoplistFiles(cbShowStoplistFiles.getState());
      }
    });

    // wire ReportsModel's report tree model listener to manage when show stoplist files is enabled
    BEViewer.reportsModel.getTreeModel().addTreeModelListener(new TreeModelListener() {
      public void treeNodesChanged(TreeModelEvent e) {
        setShowStoplistVisibility();
      }
      public void treeNodesInserted(TreeModelEvent e) {
        setShowStoplistVisibility();
      }
      public void treeNodesRemoved(TreeModelEvent e) {
        setShowStoplistVisibility();
      }
      public void treeStructureChanged(TreeModelEvent e) {
        setShowStoplistVisibility();
      }
      private void setShowStoplistVisibility() {
        Enumeration<ReportsModel.ReportTreeNode> e = BEViewer.reportsModel.elements();
        cbShowStoplistFiles.setSelected(BEViewer.reportsModel.isIncludeStoplistFiles());
      }
    });

    // view|Reports|Show Empty Files
    cbShowEmptyFiles = new JCheckBoxMenuItem("Show Empty Files");
    reportsMenu.add(cbShowEmptyFiles);
    cbShowEmptyFiles.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.reportsModel.setIncludeEmptyFiles(cbShowEmptyFiles.getState());
      }
    });

    // wire ReportsModel's report tree model listener to manage when show empty files is enabled
    BEViewer.reportsModel.getTreeModel().addTreeModelListener(new TreeModelListener() {
      public void treeNodesChanged(TreeModelEvent e) {
        setShowEmptyFilesVisibility();
      }
      public void treeNodesInserted(TreeModelEvent e) {
        setShowEmptyFilesVisibility();
      }
      public void treeNodesRemoved(TreeModelEvent e) {
        setShowEmptyFilesVisibility();
      }
      public void treeStructureChanged(TreeModelEvent e) {
        setShowEmptyFilesVisibility();
      }
      private void setShowEmptyFilesVisibility() {
        Enumeration<ReportsModel.ReportTreeNode> e = BEViewer.reportsModel.elements();
        cbShowEmptyFiles.setSelected(BEViewer.reportsModel.isIncludeEmptyFiles());
      }
    });

    // view|Reports|Refresh
    mi = new JMenuItem("Refresh");
    reportsMenu.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.reportsModel.refreshFiles();
      }
    });

    // wire ReportsModel's report tree model listener to manage when show empty files is enabled
    BEViewer.reportsModel.getTreeModel().addTreeModelListener(new TreeModelListener() {
      public void treeNodesChanged(TreeModelEvent e) {
        doRefresh();
      }
      public void treeNodesInserted(TreeModelEvent e) {
        doRefresh();
      }
      public void treeNodesRemoved(TreeModelEvent e) {
        doRefresh();
      }
      public void treeStructureChanged(TreeModelEvent e) {
        doRefresh();
      }
      private void doRefresh() {
        Enumeration<ReportsModel.ReportTreeNode> e = BEViewer.reportsModel.elements();
      }
    });

    // view|Referenced Features
    JMenu referencedFeaturesMenu = new JMenu("Referenced Features");
    view.add(referencedFeaturesMenu);

    // view|Referenced Features|always visible
    rbReferencedFeaturesVisible = new JRadioButtonMenuItem("Always Visible");
    referencedFeaturesGroup.add(rbReferencedFeaturesVisible);
    referencedFeaturesMenu.add(rbReferencedFeaturesVisible);
    rbReferencedFeaturesVisible.setSelected(!BEViewer.reportSelectionManager.isRequestHideReferencedFeatureView());
    rbReferencedFeaturesVisible.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.reportSelectionManager.setRequestHideReferencedFeatureView(false);
      }
    });

    // view|Referenced Features|collapsible
    rbReferencedFeaturesCollapsible = new JRadioButtonMenuItem("Collapsed when not Referenced");
    referencedFeaturesGroup.add(rbReferencedFeaturesCollapsible);
    referencedFeaturesMenu.add(rbReferencedFeaturesCollapsible);
    rbReferencedFeaturesCollapsible.setSelected(BEViewer.reportSelectionManager.isRequestHideReferencedFeatureView());
    rbReferencedFeaturesCollapsible.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.reportSelectionManager.setRequestHideReferencedFeatureView(true);
      }
    });

    // wire listener to manage which visibility mode is shown in the menu
    BEViewer.reportSelectionManager.addReportSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        boolean requestHide = BEViewer.reportSelectionManager.isRequestHideReferencedFeatureView();
        if (requestHide) {
          rbReferencedFeaturesCollapsible.setSelected(true);
        } else {
          rbReferencedFeaturesVisible.setSelected(true);
        }
      }
    });
    
    // view|<separator>
    view.addSeparator();

    // view|Selected Feature
    JMenu selectedFeature = new JMenu("Selected Feature");
    view.add(selectedFeature);

    // view|SelectedFeature|Pan to Start of Path
    miPanToStart = new JMenuItem("Pan to Start of Path");
    selectedFeature.add(miPanToStart);
    miPanToStart.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // move to start of feature currently in the image model
        BEViewer.imageModel.setImageSelection(ForensicPath.getAdjustedPath(
                     BEViewer.imageView.getImagePage().pageForensicPath, 0));
      }
    });
    miPanToStart.setEnabled(false);

    // view|SelectedFeature|Pan to  End of Path
    miPanToEnd = new JMenuItem("Pan to End of Path");
    selectedFeature.add(miPanToEnd);
    miPanToEnd.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // move to end of feature currently in the image model
        ImageModel.ImagePage imagePage = BEViewer.imageView.getImagePage();
        String pageForensicPath = imagePage.pageForensicPath;
        long imageSize = imagePage.imageSize;
        long imageEndOffset = (imageSize > 0) ? imageSize - 1 : 0;
        BEViewer.imageModel.setImageSelection(ForensicPath.getAdjustedPath(
                     pageForensicPath, imageEndOffset));
      }
    });
    miPanToEnd.setEnabled(false);

    // view|Selected Feature|<separator>
    selectedFeature.addSeparator();

    // view|Selected Feature|report.xml File
    miShowReportFile = new JMenuItem("Show report.xml File");
    selectedFeature.add(miShowReportFile);
    miShowReportFile.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // get the currently selected feature line
        FeatureLine featureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();
        if (featureLine.featuresFile == null) {
          WError.showError("A Feature must be selected before viewing the Report file.", 
                           "BEViewer Selected Feature error", null);
        } else {
          try {
            File reportFile = new File(featureLine.featuresFile.getParentFile(), "report.xml");
            URL url = reportFile.toURI().toURL();
            new WURL("Bulk Extractor Viewer Report file " + reportFile.toString(), url);
          } catch (Exception exc) {
            WError.showError("Unable to read report.xml file.", 
                             "BEViewer Read error", exc);
          }
        }
      }
    });
    miShowReportFile.setEnabled(false);

    // wire action for all view|selected feature items
    BEViewer.featureLineSelectionManager.addFeatureLineSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        FeatureLine selectedFeatureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();
        boolean hasSelection = !selectedFeatureLine.isBlank();
        miPanToStart.setEnabled(hasSelection);
        miPanToEnd.setEnabled(hasSelection);
        miShowReportFile.setEnabled(hasSelection);
      }
    });

    // bookmarks
    JMenu bookmarks = new JMenu("Bookmarks");
    add(bookmarks);

    // bookmarks|Bookmark selected Feature
    miAddBookmark = new JMenuItem("Bookmark selected Feature", BEIcons.ADD_BOOKMARK_16);	// ...
    bookmarks.add(miAddBookmark);
    miAddBookmark.setAccelerator(KEYSTROKE_B);
    miAddBookmark.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        FeatureLine selectedFeatureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();
        BEViewer.bookmarksModel.addElement(selectedFeatureLine);
      }
    });

    // a feature line has been selected and may be added as a bookmark
    BEViewer.featureLineSelectionManager.addFeatureLineSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {

      // same as in BEToolbar

      // enabled if feature line is not blank and is not already bookmarked
      FeatureLine selectedFeatureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();
      miAddBookmark.setEnabled(!selectedFeatureLine.isBlank()
                   && !BEViewer.bookmarksModel.contains(selectedFeatureLine));
      }
    });
    FeatureLine selectedFeatureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();
    miAddBookmark.setEnabled(!selectedFeatureLine.isBlank()
                   && !BEViewer.bookmarksModel.contains(selectedFeatureLine));

    // bookmarks|Manage Bookmarks
    miManageBookmarks = new JMenuItem("Manage Bookmarks\u2026", BEIcons.MANAGE_BOOKMARKS_16);	// ...
    bookmarks.add(miManageBookmarks);
    miManageBookmarks.setAccelerator(KEYSTROKE_M);
    miManageBookmarks.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WManageBookmarks.openWindow();
      }
    });
    // wire listener to manage when bookmarks are available for management
    BEViewer.bookmarksModel.bookmarksComboBoxModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        miManageBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
      public void intervalAdded(ListDataEvent e) {
        miManageBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
      public void intervalRemoved(ListDataEvent e) {
        miManageBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());
      }
    });
    miManageBookmarks.setEnabled(!BEViewer.bookmarksModel.isEmpty());

    // tools
    JMenu tools = new JMenu("Tools");
    add(tools);

    // tools|Run bulk_extractor
    mi = new JMenuItem("Run bulk_extractor\u2026", BEIcons.RUN_BULK_EXTRACTOR_16);	// ...
    tools.add(mi);
    mi.setAccelerator(KEYSTROKE_R);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WScan.openWindow(new ScanSettings());
      }
    });

    // tools|bulk_extractor Run Queue...
    mi = new JMenuItem("bulk_extractor Run Queue\u2026");
    tools.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WScanSettingsRunQueue.openWindow();
      }
    });

    // help
    JMenu help = new JMenu("Help");
    add(help);

    // help|About
    mi = new JMenuItem("About Bulk Extractor Viewer " + Config.VERSION, BEIcons.HELP_ABOUT_16);
    help.add(mi);
    mi.setAccelerator(KEYSTROKE_A);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WAbout.openWindow();
      }
    });

    // help|Check Versions
    mi = new JMenuItem("Check Versions");
    help.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BulkExtractorVersionReader.showVersions();
      }
    });

    // help|<separator>
    help.addSeparator();

    // help|diagnostics
    JMenu diagnostics = new JMenu("Diagnostics");
    help.add(diagnostics);

    // help|diagnostics|Show Log
    miShowLog = new JMenuItem("Show Log");
    diagnostics.add(miShowLog);
    miShowLog.setAccelerator(KEYSTROKE_L);
    miShowLog.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WLog.setVisible();
      }
    });

    // help|diagnostics|Clear Log
    mi = new JMenuItem("Clear Log");
    diagnostics.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WLog.clearLog();
      }
    });

    // help|diagnostics|Copy Log to System Clipboard
    mi = new JMenuItem("Copy Log to System Clipboard");
    diagnostics.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // clear the selection manager
        BEViewer.rangeSelectionManager.clear();

        // get the Transferable log
        Transferable log = new StringSelection(WLog.getLog());

        // copy the log to the system clipboard and, if available, to the selection clipboard
        RangeSelectionManager.setSystemClipboard(log);
        RangeSelectionManager.setSelectionClipboard(log);
      }
    });

    // help|diagnostics|<separator>
    diagnostics.addSeparator();

    // help|diagnostics|run tests
    mi = new JMenuItem("Run Tests\u2026");	// ...
    diagnostics.add(mi);
    mi.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        new WTest();
      }
    });
  }
}


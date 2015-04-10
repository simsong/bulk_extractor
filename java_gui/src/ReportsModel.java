import java.util.Vector;
import java.util.Enumeration;
import java.util.Observer;
import java.util.Comparator;
import java.util.Arrays;
import java.io.File;
import java.io.FileFilter;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.DefaultTreeSelectionModel;
import javax.swing.tree.TreeSelectionModel;
import javax.swing.tree.TreeModel;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;

/**
 * The <code>ReportsModel</code> class manages the Reports Tree
 * and provides the DefaultTreeModel and TreeSelectionModel
 * suitable for managing reports in a JTree view.
 * Unlike other models, signaling is sent to DefaultTreeModel treeModel.
 */
public class ReportsModel {

  // Report Tree node root
  private final RootTreeNode rootTreeNode = new RootTreeNode();

  /**
   * Default Tree Model suitable for displaying the Reports Tree in a JTree view.
   */
  private DefaultTreeModel treeModel = new DefaultTreeModel(rootTreeNode);

  /**
   * Default Tree Selection Model suitable for displaying the Reports Tree in a JTree view.
   */
  private DefaultTreeSelectionModel treeSelectionModel = new DefaultTreeSelectionModel();

  private boolean includeStoplistFiles;
  private boolean includeEmptyFiles;

  // Features File filename filter
  private static final FileFilter featuresFileFilter = new FileFilter() {
    // accept filenames for valid feature files
    public boolean accept(File pathname) {

      // reject unreadable files or paths that are directories
      if (!pathname.canRead() || pathname.isDirectory()) {
        return false;
      }

      // reject specific filenames
      String name = pathname.getName();
      if (!name.endsWith(".txt")) {
         // must end with .txt
         return false;
      }
      if (name.startsWith("wordlist_")) {
         // exclude wordlist
         return false;
      }

      // accept the file
      return true;
    }
  };

  // filename comparator for sorting feature files
  private static final Comparator<File> fileComparator = new Comparator<File>() {
    public int compare(File f1, File f2) {
      return f1.compareTo(f2);
    }
  };

  // a file equality checker that allows null
  private static boolean filesEqual(File f1, File f2) {
    // allow null when checking for equals
    if (f1 == null && f2 == null) {
      return true;
    } else if (f1 != null && f1.equals(f2)) {
      return true;
    } else {
      return false;
    }
  }

  /**
   * Provides a Root TreeNode that can contain Report TreeNode children.
   */
  public static class RootTreeNode implements TreeNode {
    private final Vector<ReportTreeNode> reportTreeNodes = new Vector<ReportTreeNode>();

    public RootTreeNode() {
    }

    public int getIndex(File featuresDirectory, File reportImageFile) {

      // log if features directory does not contain report.xml
      File reportFile = new File(featuresDirectory, "report.xml");
      if (!reportFile.isFile()) {
        WLog.log("ReportsModel.getIndex: unexpected missing file " + reportFile);
      }

      // return the index of the equivalent ReportTreeNode
      Enumeration<ReportTreeNode> e = reportTreeNodes.elements();
      while (e.hasMoreElements()) {
        ReportTreeNode reportTreeNode = e.nextElement();
        if (filesEqual(reportTreeNode.featuresDirectory, featuresDirectory)
         && filesEqual(reportTreeNode.reportImageFile, reportImageFile)) {
          return reportTreeNodes.indexOf(reportTreeNode);
        }
      }

      // a matching report was not found
      return -1;
    }

    // add unless present
    private void add(ReportTreeNode reportTreeNode) {
      if (getIndex(reportTreeNode) == -1) {
        // the report is new, so add it
        reportTreeNodes.add(reportTreeNode);
      } else {
        WLog.log("ReportsModel.RootTreeNode.add: Node already present: " + reportTreeNode);
      }
    }

    private void removeElementAt(int index) {
      if (index != -1) {
        // remove the report
        reportTreeNodes.removeElementAt(index);
      }
    }

    private int removeAllElements() {
      int size = reportTreeNodes.size();
      reportTreeNodes.removeAllElements();
      return size;
    }

    // the remaining functions implement TreeNode
    public Enumeration<ReportTreeNode> children() {
      return reportTreeNodes.elements();
    }
    public boolean getAllowsChildren() {
      return true;
    }
    public ReportTreeNode getChildAt(int childIndex) {
      return reportTreeNodes.get(childIndex);
    }
    public int getChildCount() {
      return reportTreeNodes.size();
    }
    public int getIndex(TreeNode treeNode) {
      return reportTreeNodes.indexOf(treeNode);
    }
    public TreeNode getParent() {
      return null;
    }
    public boolean isLeaf() {
      return false;
    }
  }

  /**
   * Provides a Report TreeNode that can contain Features File TreeNode children.
   */
  public static class ReportTreeNode implements TreeNode {

    public final RootTreeNode parent;
    // these files define a Report
    public final File featuresDirectory;
    public final File reportImageFile;

    // a Report provides a vector of feature file nodes
    private final Vector<FeaturesFileTreeNode> featuresFileNodes = new Vector<FeaturesFileTreeNode>();

    public ReportTreeNode(RootTreeNode parent, File featuresDirectory, File reportImageFile) {
      this.parent = parent;
      this.featuresDirectory = featuresDirectory;
      this.reportImageFile = reportImageFile;
    }

    private void setFiles(boolean includeStoplistFiles, boolean includeEmptyFiles) {
      // get the initial array of feature files
      File[] featuresFiles;
      if (featuresDirectory == null) {
        featuresFiles = new File[0];
      } else {
        featuresFiles = featuresDirectory.listFiles(featuresFileFilter);
        if (featuresFiles == null) {
          WLog.log("ReportsModel.ReportTreeNode: unexpected invalid null featuresFiles.");
          featuresFiles = new File[0];
        }
      }

      // sort the array of features files
      Arrays.sort(featuresFiles, fileComparator);

      // add accepted files to featuresFileNodes
      featuresFileNodes.clear();
      for (int i=0; i<featuresFiles.length; i++) {
        if (!includeStoplistFiles && featuresFiles[i].getName().endsWith("_stoplist.txt")) {
          // skip stoplists when not showing them
          continue;
        }
        if (!includeEmptyFiles && featuresFiles[i].length() == 0) {
          // skip empty files when not showing them
          continue;
        }
        featuresFileNodes.add(new FeaturesFileTreeNode(this, featuresFiles[i]));
      }
    }

    public String toString() {
      if (reportImageFile == null) {
        return "features directory: " + featuresDirectory + ", no image file, features files size: " + featuresFileNodes.size();
      } else {
        return "features directory: " + featuresDirectory + ", report image file: " + reportImageFile + ", features files size: " + featuresFileNodes.size();
      }
    }

    public Enumeration<FeaturesFileTreeNode> children() {
      return featuresFileNodes.elements();
    }
    public boolean getAllowsChildren() {
      return true;
    }
    public FeaturesFileTreeNode getChildAt(int childIndex) {
      return featuresFileNodes.get(childIndex);
    }
    public int getChildCount() {
      return featuresFileNodes.size();
    }
    public int getIndex(TreeNode treeNode) {
      FeaturesFileTreeNode featuresFileTreeNode = (FeaturesFileTreeNode)treeNode;
      if (featuresFileTreeNode == null) {
        return -1;
      }
      // find the matching report in the report nodes vector
      Enumeration<FeaturesFileTreeNode> e = featuresFileNodes.elements();
      while (e.hasMoreElements()) {
        FeaturesFileTreeNode node = e.nextElement();
        if (filesEqual(featuresFileTreeNode.featuresFile, node.featuresFile)) {
          return featuresFileNodes.indexOf(node);
        }
      }
      return -1;
    }

    public TreeNode getParent() {
      return parent;
    }
    public boolean isLeaf() {
      return false;
    }
  }

  /**
   * Provides a Features File TreeNode leaf.
   */
  public static class FeaturesFileTreeNode implements TreeNode {

    private final ReportTreeNode parent;
    public final File featuresFile;

    public FeaturesFileTreeNode(ReportTreeNode parent, File featuresFile) {
      this.parent = parent;
      this.featuresFile = featuresFile;
    }

    public Enumeration<FeaturesFileTreeNode> children() {
      return null;
    }
    public boolean getAllowsChildren() {
      return false;
    }
    public TreeNode getChildAt(int childIndex) {
      return null;
    }
    public int getChildCount() {
      return 0;
    }
    public int getIndex(TreeNode treeNode) {
      return -1;
    }
    public TreeNode getParent() {
      return parent;
    }
    public boolean isLeaf() {
      return true;
    }
  }

  /**
   * Constructs a <code>ReportsModel</code> object.
   */
  public ReportsModel() {
    // configure the tree selection model to work with ReportsModel
    treeSelectionModel.setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
  }

  /**
   * Obtain the tree model.
   */
  public TreeModel getTreeModel() {
    return treeModel;
  }

  /**
   * Obtain the tree selection model.
   */
  public TreeSelectionModel getTreeSelectionModel() {
    return treeSelectionModel;
  }

  /**
   * Set user preference to include stoplist files.
   */
  public void setIncludeStoplistFiles(boolean includeStoplistFiles) {
    this.includeStoplistFiles = includeStoplistFiles;
    setFiles();
  }

  /**
   * Return user preference to include stoplist files.
   */
  public boolean isIncludeStoplistFiles() {
    return includeStoplistFiles;
  }

  /**
   * Set user preference to include empty files.
   */
  public void setIncludeEmptyFiles(boolean includeEmptyFiles) {
    this.includeEmptyFiles = includeEmptyFiles;
    setFiles();
  }

  /**
   * Return user preference to include empty files.
   */
  public boolean isIncludeEmptyFiles() {
    return includeEmptyFiles;
  }

  /**
   * Refresh the list of files.
   */
  public void refreshFiles() {
    setFiles();
  }

  // reload FeaturesFileTreeNode and reload the treeModel
  private void setFiles() {
    // reload the FeaturesFileTreeNode leafs
    Enumeration<ReportsModel.ReportTreeNode> e = elements();
    while (e.hasMoreElements()) {
      ReportsModel.ReportTreeNode reportTreeNode = e.nextElement();
      reportTreeNode.setFiles(includeStoplistFiles, includeEmptyFiles);
    }

    // signal change to Tree Model
    treeModel.reload();
  }

  /**
   * Adds the report if it is new and updates the tree model.
   */
  public void addReport(File featuresDirectory, File reportImageFile) {
    // check to see if an equivalent report is already opened
    int index = rootTreeNode.getIndex(featuresDirectory, reportImageFile);

    if (index == -1) {
      // the report is new, so add it
      ReportTreeNode reportTreeNode = new ReportTreeNode(rootTreeNode, featuresDirectory, reportImageFile);
      rootTreeNode.add(reportTreeNode);

      // update the features file list
      reportTreeNode.setFiles(includeStoplistFiles, includeEmptyFiles);

      // signal change to Tree Model
      index = rootTreeNode.getIndex(reportTreeNode);
      int[] insertedIndex = { index };
      treeModel.nodesWereInserted(rootTreeNode, insertedIndex);
    } else {
      // already there, so no action
      WError.showError("Report already opened for directory '"
                       + featuresDirectory + "'\nReport Image file '" + reportImageFile + "'.",
                       "Already Open", null);
    }
  }

  /**
   * Removes the report and updates the tree model.
   */
  public void remove(ReportTreeNode reportTreeNode) {
    int index = rootTreeNode.getIndex(reportTreeNode);
    if (index != -1) {

      // if the report removed is selected, deselect it
      TreePath selectedTreePath = treeSelectionModel.getSelectionPath();
      ReportTreeNode selectedReportTreeNode = getReportTreeNodeFromTreePath(selectedTreePath);
      if (reportTreeNode == selectedReportTreeNode) {
        treeSelectionModel.clearSelection();
        treeSelectionModel.setSelectionPath(null);
      }

      // remove the report tree node from the root tree node
      rootTreeNode.removeElementAt(index);

      // signal change to Tree Model
      int[] removedIndex = { index };
      TreeNode[] removedNodes = { reportTreeNode };
      treeModel.nodesWereRemoved(rootTreeNode, removedIndex, removedNodes);
    } else {
      // not there, so no action
      WLog.log("ReportsModel.RootTreeNode.remove: Report not present: " + reportTreeNode);
    }
  }

  /**
   * Clears all reports and updates the tree model.
   */
  public void clear() {
    int size = rootTreeNode.removeAllElements();
    if (size > 0) {
      // signal change to Tree Model
      treeModel.reload();

      // deselect any selection
      treeSelectionModel.clearSelection();
    } else {
      // already empty, so no action
      WLog.log("ReportsModel.RootTreeNode.removeAllElements: Already empty");
    }
  }

  /**
   * Returns the report elements.
   */
  public Enumeration<ReportTreeNode> elements() {
    return rootTreeNode.children();
  }

  /**
   * Obtain the TreePath to a report else null.
   */
  public TreePath getTreePath(File featuresDirectory, File reportImageFile) {
    // find the index to a matching report
    int index = rootTreeNode.getIndex(featuresDirectory, reportImageFile);
    if (index != -1) {
      ReportTreeNode node = rootTreeNode.getChildAt(index);
      return new TreePath(new Object[] {node.parent, node});
    } else {
      return null;
    }
  }

  /**
   * Convenience function for returning the currently selected TreePath in the TreeSelectionModel.
   */
  public TreePath getSelectedTreePath() {
    return treeSelectionModel.getSelectionPath();
  }

  /**
   * Convenience function to get the ReportTreeNode from tree path
   */
  public static ReportTreeNode getReportTreeNodeFromTreePath(TreePath path) {
    if ((path != null) && path.getPathCount() >= 2) {
      return ((ReportTreeNode)(path.getPathComponent(1)));
    } else {
      return null;
    }
  }

  /**
   * Convenience function to get the FeaturesFileTreeNode from tree path
   */
  public static FeaturesFileTreeNode getFeaturesFileTreeNodeFromTreePath(TreePath path) {
    if ((path != null) && path.getPathCount() == 3) {
      return ((FeaturesFileTreeNode)(path.getPathComponent(2)));
    } else {
      return null;
    }
  }
}


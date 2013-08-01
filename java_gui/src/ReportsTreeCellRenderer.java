import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeSelectionModel;
import javax.swing.tree.DefaultMutableTreeNode;
import java.io.File;
import java.util.Observable;
import java.util.Observer;

/**
 * The <code>ReportsTreeCellRenderer</code> class provides a text list renderer view
 * for the tree nodes.
 */
public class ReportsTreeCellRenderer extends DefaultTreeCellRenderer {
  private static final long serialVersionUID = 1;

  public Component getTreeCellRendererComponent(
                                  JTree tree,
                                  Object value,
                                  boolean selected,
                                  boolean expanded,
                                  boolean leaf,
                                  int row,
                                  boolean hasFocus) {

    // configure the display based on the type of the value
    String text;
    String toolTipText;
    boolean isEnabled = true;

    if (value instanceof ReportsModel.RootTreeNode) {
      // RootTreeNode
      // only the root node can do this, but just prepare warning text
      text = "error";
      toolTipText = "";

    } else if (value instanceof ReportsModel.ReportTreeNode) {
      // ReportTreeNode
      ReportsModel.ReportTreeNode reportTreeNode = (ReportsModel.ReportTreeNode)value;

      // set text
      if (reportTreeNode.featuresDirectory != null) {
        text = reportTreeNode.featuresDirectory.getName();
      } else {
        text = "No features directory";
      }

      // set tool tip text
      if (reportTreeNode.featuresDirectory != null) {
        toolTipText = reportTreeNode.featuresDirectory.toString();
      } else {
        toolTipText = "No features directory";
      }
      if (reportTreeNode.reportImageFile != null) {
        toolTipText += ", " + reportTreeNode.reportImageFile.toString();
      } else {
        toolTipText += ", no image file";
      }

    } else if (value instanceof ReportsModel.FeaturesFileTreeNode) {
      ReportsModel.FeaturesFileTreeNode featuresFileTreeNode = (ReportsModel.FeaturesFileTreeNode)value;
      // features file
      File featuresFile = featuresFileTreeNode.featuresFile;
      if (featuresFile != null) {
        text = featuresFile.getName();
        toolTipText = featuresFile.toString();
      } else {
        text = "No features file";
        toolTipText = "No features file";
      }

      // disable the cell unless the file size > 0
      try {
        if (featuresFile.length() == 0) {
          isEnabled = false;
        }
      } catch (Exception e) {
         isEnabled = false;
      }

    } else {
      throw new RuntimeException("invalid tree node: " + value);
    }

    // fill out and return this renderer
    super.getTreeCellRendererComponent(tree, text, selected, expanded, leaf, row, hasFocus);
    setToolTipText(toolTipText);
    setEnabled(isEnabled);
    return this;
  }
}


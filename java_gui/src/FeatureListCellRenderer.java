import java.awt.Component;
import javax.swing.*;
import java.awt.Font;

/**
 * The <code>FeatureListCellRenderer</code> class provides a ListCellRenderer interface
 * for FeatureLine objects, specifically,
 * the FeatureLine Selection ComboBox for NavigationPane
 * and the FeatureLine list for WBookmarks.
 */

//public class FeatureListCellRenderer extends JLabel implements ListCellRenderer<FeatureLine> {
public class FeatureListCellRenderer extends JLabel implements ListCellRenderer {
  private static final long serialVersionUID = 1;
  /**
   * Creates a list cell renderer object for rendering feature lines in a ListCellRenderer.
   */
  public FeatureListCellRenderer() {
    setOpaque(true);
  }

  /**
   * Returns a Component containing the rendered feature selection text
   * derived from the FeatureLine.
   */
  public Component getListCellRendererComponent(
//                                JList<? extends FeatureLine> list, // the list
                                JList list, // the list
//                                FeatureLine featureLine, // value to display
                                Object featureLine, // value to display
                                int index,               // cell index
                                boolean isSelected,      // is the cell selected
                                boolean cellHasFocus) {  // does the cell have focus

    // set the text
    Font font = list.getFont();
    if (featureLine == null) {
      // this occurs when the list is empty.
      setEnabled(false);
      setFont(font.deriveFont(Font.ITALIC));
      setText("None");
    } else {
      // featureLine is available
      setEnabled(true);
      setFont(font.deriveFont(Font.PLAIN));
      setText(((FeatureLine)featureLine).getSummaryString());
    }

    // set color
    if (isSelected) {
      setBackground(list.getSelectionBackground());
      setForeground(list.getSelectionForeground());
    } else {
      setBackground(list.getBackground());
      setForeground(list.getForeground());
    }
    return this;
  }
}


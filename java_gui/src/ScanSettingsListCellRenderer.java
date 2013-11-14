import javax.swing.ListCellRenderer;
import java.awt.Component;
//import java.util.Map;
//import java.util.Vector;
//import java.util.Iterator;
import javax.swing.JLabel;
import javax.swing.JList;
//import java.io.IOException;

/**
 * Implements ListCellRenderer to list a scan setting in a JList.
 */
public class ScanSettingsListCellRenderer extends JLabel implements ListCellRenderer {
  private static final long serialVersionUID = 1;

  public Component getListCellRendererComponent(
             JList list,
             Object scanSettingObject,
             int index,
             boolean isSelected,
             boolean cellHasFocus) {

    // set the text
    setText(((ScanSettings)scanSettingObject).getCommandString());

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


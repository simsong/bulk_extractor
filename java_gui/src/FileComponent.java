import javax.swing.JLabel;
import javax.swing.ToolTipManager;
import java.awt.Dimension;
import java.awt.Font;
import java.io.File;

/**
 * The <code>FileComponent</code> class provides an uneditable text view of a file's filename.
 * The tooltiptext name is the long file path.
 */
public class FileComponent extends JLabel {
  private static final long serialVersionUID = 1;
  private static final String NULL_FILE = "None";
  private static final String NULL_TOOLTIP = null;

  /**
   * Creates a file component object.
   */
  public FileComponent() {
    setText(" "); // required during initialization for getPreferredSize()
    setFile(null);
    ToolTipManager.sharedInstance().registerComponent(this);  // tooltip is normally off on JLabel
    setMinimumSize(new Dimension(0, getPreferredSize().height));
  }

  /**
   * Sets the file to display.
   * @param file the file to display
   */
  public void setFile(File file) {
    Font font = getFont();
    if (file == null) {
      setEnabled(false);
      setFont(font.deriveFont(Font.ITALIC));
      setText(NULL_FILE);
      setToolTipText(NULL_TOOLTIP);
    } else {
      setEnabled(true);
      setFont(font.deriveFont(Font.PLAIN));
      setText(file.getName());
      setToolTipText(file.getAbsolutePath());
    }
  }
}


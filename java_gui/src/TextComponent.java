import javax.swing.JLabel;
import java.awt.Dimension;
import java.awt.Font;

/**
 * The <code>TextComponent</code> class provides an uneditable text view of a text string.
 */
public class TextComponent extends JLabel {
  private static final long serialVersionUID = 1;
  private static final String NULL_TEXT = "None";

  /**
   * Creates a text component object.
   */
  public TextComponent() {
    setText(" "); // required during initialization for getPreferredSize()
    setComponentText(null);
    setMinimumSize(new Dimension(0, getPreferredSize().height));
  }

  /**
   * Sets the text to display.
   * @param text the text to display
   */
  public void setComponentText(String text) {
    Font font = getFont();
    if (text == null || text == "") {
      setEnabled(false);
      setFont(font.deriveFont(Font.ITALIC));
      setText(NULL_TEXT);
    } else {
      setEnabled(true);
      setFont(font.deriveFont(Font.PLAIN));
      setText(text);
    }
  }
}


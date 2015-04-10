import javax.swing.JLabel;
import javax.swing.UIManager;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.util.Observer;
import java.util.Observable;

/**
 * The <code>TextComponent</code> class provides an uneditable text view of a text string.
 */
public class TextComponent extends JLabel implements MouseListener, CopyableLineInterface {
  private static final long serialVersionUID = 1;
  private static final String NULL_TEXT = "None";
  private RangeSelectionManagerChangedListener rangeSelectionManagerChangedListener;

  /**
   * Creates a text component object.
   */
  public TextComponent() {
    setText(" "); // required during initialization for getPreferredSize()
    setComponentText(null);
    setMinimumSize(new Dimension(0, getPreferredSize().height));
    setBackground(UIManager.getColor("List.selectionBackground")); // for when opaque
    addMouseListener(this);

    rangeSelectionManagerChangedListener = new RangeSelectionManagerChangedListener();
    BEViewer.rangeSelectionManager.addRangeSelectionManagerChangedListener(
                                       rangeSelectionManagerChangedListener);
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

  private void doSetSelectionManager() {
    setOpaque(true);
    repaint();
    BEViewer.rangeSelectionManager.setRange(this, 0, 0);
  }

  private TextComponent getThisComponent() {
    return this;
  }

  // MouseListener itnerfaces
  public void mouseClicked(MouseEvent e) {
  }
  public void mouseExited(MouseEvent e) {
  }
  public void mouseEntered(MouseEvent e) {
  }
  public void mousePressed(MouseEvent e) {
    doSetSelectionManager();
  }
  public void mouseReleased(MouseEvent e) {
  }

  // CopyableLineInterface
  public String getCopyableLine(int line) {
    return getText();
  }

  // this listener is notified when the range selection changes
  private class RangeSelectionManagerChangedListener implements Observer {
    public void update(Observable o, Object arg) {
      // clear range if the change is from another provider
      if (BEViewer.rangeSelectionManager.getProvider() != getThisComponent()) {
        setOpaque(false);
        repaint();
      }
    }
  }
}


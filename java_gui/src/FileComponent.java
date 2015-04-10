import javax.swing.JLabel;
import javax.swing.ToolTipManager;
import javax.swing.UIManager;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.util.Observer;
import java.util.Observable;
import java.io.File;

/**
 * The <code>FileComponent</code> class provides an uneditable text view of a file's filename.
 * The tooltiptext name is the long file path.
 */
public class FileComponent extends JLabel implements MouseListener, CopyableLineInterface {
  private static final long serialVersionUID = 1;
  private static final String NULL_FILE = "None";
  private static final String NULL_TOOLTIP = null;
  private RangeSelectionManagerChangedListener rangeSelectionManagerChangedListener;

  /**
   * Creates a file component object.
   */
  public FileComponent() {
    setText(" "); // required during initialization for getPreferredSize()
    setFile(null);
    ToolTipManager.sharedInstance().registerComponent(this);  // tooltip is normally off on JLabel
    setMinimumSize(new Dimension(0, getPreferredSize().height));
    setBackground(UIManager.getColor("List.selectionBackground")); // for when opaque
    addMouseListener(this);

    rangeSelectionManagerChangedListener = new RangeSelectionManagerChangedListener();
    BEViewer.rangeSelectionManager.addRangeSelectionManagerChangedListener(
                                       rangeSelectionManagerChangedListener);
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

  private void doSetSelectionManager() {
    setOpaque(true);
    repaint();
    BEViewer.rangeSelectionManager.setRange(this, 0, 0);
  }

  private FileComponent getThisComponent() {
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


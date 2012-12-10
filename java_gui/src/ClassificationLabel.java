import java.util.Observer;
import java.util.Observable;
import javax.swing.JLabel;
import javax.swing.ToolTipManager;
import javax.swing.SwingConstants;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.io.File;

/**
 * The <code>ClassificationLabel</code> class provides a JLabel containing an enumerated
 * <code>Classification</code> identifying a classification text and color.
 */
public class ClassificationLabel extends JLabel {
  private static final long serialVersionUID = 1;
  private final ClassificationManager classificationManager;
  private String classificationLabel = null;
  private Color classificationColor = null;

  /**
   * Creates a classification label object.
   */
  public ClassificationLabel(ClassificationManager classificationManager) {
    super("", SwingConstants.CENTER);
    this.classificationManager = classificationManager;
    classificationManager.addClassificationManagerChangedListener(new ClassificationManagerChangedListener());
    setOpaque(true);
    setVisible(false);
  }

  /**
   * Sets the classification label.
   */
  public void setClassificationLabel(boolean active, String label, Color color) {
    setVisible(active);
    setText(label);
    setBackground(color);
  }

  // this listener is notified by the classification manager
  private class ClassificationManagerChangedListener implements Observer {
    public void update(Observable o, Object arg) {
      boolean active = classificationManager.isActive();
      String label = classificationManager.getLabel();
      Color color = classificationManager.getColor();
      setClassificationLabel(active, label, color);
    }
  }
}


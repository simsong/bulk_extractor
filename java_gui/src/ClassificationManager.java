import java.awt.Color;
import java.util.Observer;

/**
 * The <code>ClassificationManager</code> class signals classification levels to listeners.
 */
public class ClassificationManager {
  private boolean active = false;
  private String label = "";
  private Color color = Color.BLACK;

  /**
   * Clears the classification setting.
   */
  public void clearClassification() {
    setClassification(false, "", Color.BLACK);
  }

  /**
   * Adds to the classification setting
   */
  public void addClassification(String text) {
    setClassification(true, "F/NSM", Color.GREEN);
  }

  private void setClassification(boolean active, String label, Color color) {
    this.active = active;
    this.label = label;
    this.color = color;

    // fire changed
    classificationManagerChangedNotifier.fireModelChanged(null);
  }

  // resources
  private final ModelChangedNotifier<String> classificationManagerChangedNotifier = new ModelChangedNotifier<String>();

  public ClassificationManager() {
  }

  /**
   * Returns whether the classification is active.
   */
  public boolean isActive() {
    return active;
  }

  /**
   * Returns the classification label.
   */
  public String getLabel() {
    return label;
  }

  /**
   * Returns the classificatin color.
   */
  public Color getColor() {
    return color;
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param classificationManagerChangedListener the <code>Observer</code> to be added
   */
  public void addClassificationManagerChangedListener(Observer classificationManagerChangedListener) {
    classificationManagerChangedNotifier.addObserver(classificationManagerChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param classificationManagerChangedListener the <code>Observer</code> to be removed
   */
  public void removeClassificationManagerChangedListener(Observer classificationManagerChangedListener) {
    classificationManagerChangedNotifier.deleteObserver(classificationManagerChangedListener);
  }
}


import java.util.Observer;

/**
 * This simple class provides notification when a FeatureLine is selected
 * and decouples the selected feature line from the ImageModel.
 */
public class FeatureLineSelectionManager {

  // model state
  private FeatureLine featureLine = new FeatureLine();

  private final ModelChangedNotifier<Object> featureLineSelectionManagerChangedNotifier = new ModelChangedNotifier<Object>();

  public FeatureLineSelectionManager() {
  }

  /**
   * Sets the selected FeatureLine.
   */
  public void setFeatureLineSelection(FeatureLine featureLine) {
if (featureLine == null) {
  throw new RuntimeException("bad");
}
// zz clean up check because new way is never null featureLine
    // do nothing if the feature line is already selected
    // determine equivalency, allowing null
    if (featureLine == null || this.featureLine == null) {
      if (featureLine != this.featureLine) {
        this.featureLine = featureLine;
        featureLineSelectionManagerChangedNotifier.fireModelChanged(null);
      }
    } else if (!featureLine.equals(this.featureLine)) {
      this.featureLine = featureLine;
      featureLineSelectionManagerChangedNotifier.fireModelChanged(null);
    } else {
      // no change
    }
  }

  /**
   * Returns the selected FeatureLine.
   */
  public FeatureLine getFeatureLineSelection() {
    return featureLine;
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param featureLineSelectionManagerChangedListener the <code>Observer</code> to be added
   */
  public void addFeatureLineSelectionManagerChangedListener(Observer featureLineSelectionManagerChangedListener) {
    featureLineSelectionManagerChangedNotifier.addObserver(featureLineSelectionManagerChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param featureLineSelectionManagerChangedListener the <code>Observer</code> to be removed
   */
  public void removeFeatureLineSelectionManagerChangedListener(Observer featureLineSelectionManagerChangedListener) {
    featureLineSelectionManagerChangedNotifier.deleteObserver(featureLineSelectionManagerChangedListener);
  }
}


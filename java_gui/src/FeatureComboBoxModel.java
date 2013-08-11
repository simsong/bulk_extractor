import javax.swing.DefaultComboBoxModel;
import java.util.Observable;
import java.util.Observer;
import java.io.File;

/**
 * The <code>FeatureComboBoxModel</code> class extends <code>DefaultComboBoxModel</code>
 * to provide a list of selectable features for a ComboBox.
 * It is supplied with features as feature selections are made.
 */
//public class FeatureComboBoxModel extends DefaultComboBoxModel<FeatureLine> {
public class FeatureComboBoxModel extends DefaultComboBoxModel {
  private final FeatureLineSelectionManager featureLineSelectionManager;
  private static final long serialVersionUID = 0;

  /**
   * Constructs a feature ComboBox model that adds and selects Feature Lines
   * from the image model as selections are made.
   * The feature list may be removed via <code>super.removeAllElements()</code>.
   */
  public FeatureComboBoxModel(FeatureLineSelectionManager featureLineSelectionManager) {
    this.featureLineSelectionManager = featureLineSelectionManager;
    featureLineSelectionManager.addFeatureLineSelectionManagerChangedListener(new FeatureLineSelectionManagerChangedListener());
  }

  // this listener is notified when the feature selection changes.
  private class FeatureLineSelectionManagerChangedListener implements Observer {
    public void update(Observable o, Object arg) {

      FeatureLine featureLine = featureLineSelectionManager.getFeatureLineSelection();
      // feature lines are navigable if they have a valid path
      // but not if they have a histogram field
      if (!featureLine.isBlank()
       && !ForensicPath.isHistogram(featureLine.forensicPath)) {
        selectFeature(featureLine);
      }
    }
  }

  /**
   * Selects the feature, creating it if it is not already there.
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public void selectFeature(FeatureLine featureLine) {
    // if blank then select null
    if (featureLine.isBlank()) {
      doSelect(null);
      return;
    }

    // if available, select an equivalent feature line
    for (int i=0; i<getSize(); i++) {
      FeatureLine residentFeatureLine = (FeatureLine)getElementAt(i);
      if (residentFeatureLine.equals(featureLine)) {

        // select the feature line
        doSelect(residentFeatureLine);
        return;
      }
    }

    // it is new, so add it and then select it
    addElement(featureLine);
    doSelect(featureLine);
  }

  private void doSelect(FeatureLine featureLine) {
    if (featureLine != getSelectedItem()) {
      setSelectedItem(featureLine); // in DefaultComboBoxModel
    }
  }

  /**
   * Clear the feature history.
   */
  public void removeAllFeatures() {
    removeAllElements();
  }

  /**
   * Clear the feature history of features associated with the given Report.
   */
  public void removeAssociatedFeatures(ReportsModel.ReportTreeNode reportTreeNode) {

    // if the selected item is going to be removed, clear it or else the
    // model will assign another one
    FeatureLine selectedFeatureLine = (FeatureLine)getSelectedItem();
    if (selectedFeatureLine != null && selectedFeatureLine.isFromReport(reportTreeNode)) {
      setSelectedItem(null);
    }
    
    // DefaultComboBoxModel doesn't provide an iterator so copy to an array for use below
    FeatureLine[] featureLines = new FeatureLine[getSize()];
    for (int i=0; i<featureLines.length; i++) {
      featureLines[i] = (FeatureLine)getElementAt(i);
    }

    // now remove the associated features from DefaultComboBoxModel
    for (int i=0; i<featureLines.length; i++) {
      if (featureLines[i].isFromReport(reportTreeNode)) {
        removeElement(featureLines[i]);
      }
    }
  }
}


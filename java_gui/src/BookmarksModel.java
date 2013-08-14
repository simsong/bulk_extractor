/*
 * Provides bookmark services, specifically, management and selection of
 * a bookmark list, by wrapping DefaultListModel and DefaultComboBoxModel
 * services.
 *
 * Note that equality is redefined to use equals rather than sameness.
 */
//import javax.swing.DefaultListModel;
import javax.swing.DefaultComboBoxModel;

public class BookmarksModel {

  public static class BookmarksComboBoxModel extends DefaultComboBoxModel {
    /*
     * Offer facility to manage when the feature's view changes.
     */
    public void fireViewChanged() {
      fireContentsChanged(this, 0, getSize());
    }
  }

  public final BookmarksComboBoxModel bookmarksComboBoxModel = new BookmarksComboBoxModel();

@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public void addElement(FeatureLine featureLine) {
    if (featureLine.isBlank()) {
      // this can happen on keystroke bookmark request
      WLog.log("BookmarksModel.addElement blank element not added");
    } else if (contains(featureLine)) {
      // this can happen on keystroke bookmark request
      WLog.log("BookmarksModel.addElement already has element");
    } else {
      bookmarksComboBoxModel.addElement((Object)featureLine);
    }
  }
  public FeatureLine get(int i) {
    return (FeatureLine)bookmarksComboBoxModel.getElementAt(i);
  }
  public int size() {
    return bookmarksComboBoxModel.getSize();
  }
  public boolean isEmpty() {
    return (bookmarksComboBoxModel.getSize() == 0);
  }
  public boolean contains(FeatureLine featureLine) {
    int size = bookmarksComboBoxModel.getSize();
    // can't just use getIndexOf because we check sameness, not same object
    for (int i=0; i<size; i++) {
      if (featureLine.equals(
                   (FeatureLine)bookmarksComboBoxModel.getElementAt(i))) {
        return true;
      }
    }
    return false;
    // this fails because it checks sameness, not object equality
    //return (bookmarksComboBoxModel.getIndexOf((Object)featureLine) >= 0);
  }

  public void removeElement(FeatureLine featureLine) {
    int size = bookmarksComboBoxModel.getSize();
    for (int i=0; i<size; i++) {
      if (featureLine.equals(
                   (FeatureLine)bookmarksComboBoxModel.getElementAt(i))) {
        bookmarksComboBoxModel.removeElementAt(i);
        return;
      }
    }
    WLog.log("requested element failed to remove: " + featureLine);
  }
  public void clear() {
    bookmarksComboBoxModel.removeAllElements();
  }
  public FeatureLine[] getBookmarks() {
    int size = bookmarksComboBoxModel.getSize();
    FeatureLine[] featureLines = new FeatureLine[size];
    for (int i=0; i<size; i++) {
      featureLines[i] = (FeatureLine)bookmarksComboBoxModel.getElementAt(i);
    }
    return featureLines;
  }

  public void fireViewChanged() {
    bookmarksComboBoxModel.fireViewChanged();
  }

  // selection
  public FeatureLine getSelectedItem() {
    return (FeatureLine)bookmarksComboBoxModel.getSelectedItem();
  }
  public void setSelectedItem(FeatureLine featureLine) {
    bookmarksComboBoxModel.setSelectedItem((Object)featureLine);
  }

  // bookmark-specific remove capability
  public void removeAssociatedFeatureLines(ReportsModel.ReportTreeNode reportTreeNode) {
    // get feature line list to use
    FeatureLine[] featureLines = getBookmarks();

    // now remove the associated features from each wrapped model
    int size = bookmarksComboBoxModel.getSize();
    for (int i=0; i<size; i++) {
      if (featureLines[i].isFromReport(reportTreeNode)) {
        removeElement(featureLines[i]);
      }
    }
  }
}

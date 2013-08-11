/*
 * Provides bookmark services, specifically, management and selection of
 * a bookmark list, by wrapping DefaultListModel and DefaultComboBoxModel
 * services.
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

//  public DefaultListModel defaultListModel = new DefaultListModel();
  public BookmarksComboBoxModel bookmarksComboBoxModel = new BookmarksComboBoxModel();

@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public void addElement(FeatureLine featureLine) {
//    defaultListModel.addElement((Object)featureLine);
    bookmarksComboBoxModel.addElement((Object)featureLine);
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
    return (bookmarksComboBoxModel.getIndexOf((Object)featureLine) >= 0);
//    return bookmarksComboBoxModel.contains((Object)featureLine);
  }
  public void removeElement(FeatureLine featureLine) {
//    defaultListModel.removeElement((Object)featureLine);
    bookmarksComboBoxModel.removeElement((Object)featureLine);
  }
  public void clear() {
//    defaultListModel.clear();
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

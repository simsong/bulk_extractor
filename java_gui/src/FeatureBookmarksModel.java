import java.util.Observer;
import java.util.Enumeration;
import java.io.File;
import javax.swing.DefaultListModel;

/**
 * The <code>FeatureBookmarksModel</code> class supports bookmarking
 * and provides a <code>DefaultListModel</code> list of bookmarks.
 */
public class FeatureBookmarksModel {

  private final ModelChangedNotifier<Object> bookmarksModelChangedNotifier
                = new ModelChangedNotifier<Object>();
//  private DefaultListModel<FeatureLine> defaultListModel = new DefaultListModel<FeatureLine>();
  private DefaultListModel defaultListModel = new DefaultListModel();

  /**
   * Returns the <code>DefaultListModel</code> associated with this model
   * @return the bookmark list model
   */
//  public DefaultListModel<FeatureLine> getListModel() {
  public DefaultListModel getListModel() {
    return defaultListModel;
  }

  /**
   * Returns the number of feature line bookmarks in the model.
   * @return the number of feature line bookmarks in the model
   */
  public int size() {
    return defaultListModel.getSize();
  }

  /**
   * Returns the feature line bookmark at the given index.
   * @return the feature line bookmark at the given index
   */
  public FeatureLine get(int index) {
    return (FeatureLine)defaultListModel.get(index);
  }

  /*
   * Adds a FeatureLine to the bookmarks list unless it is already there.
   */
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  public void addBookmark(FeatureLine featureLine) {
    // add the featureLine if it is not already there
    if (!hasFeatureLine(featureLine)) {
      defaultListModel.addElement(featureLine);
      bookmarksModelChangedNotifier.fireModelChanged(null);
    }
  }

  /*
   * Removes a FeatureLine from the bookmarks list.
   */
  public void removeBookmark(FeatureLine featureLine) {
    // remove the feature line
    defaultListModel.removeElement(featureLine);
    bookmarksModelChangedNotifier.fireModelChanged(null);
  }

  private boolean hasFeatureLine(FeatureLine featureLine) {
    if (featureLine == null) {
      return false;
    }

    // check each element for match
    for (Enumeration e = defaultListModel.elements(); e.hasMoreElements();) {
      if (((FeatureLine)e.nextElement()).equals(featureLine)) {
        return true;
      }
    }
    return false;
  }

  /*
   * Removes all FeatureLines from the bookmarks list.
   */
  public void removeAllBookmarks() {
    // remove all feature lines
    defaultListModel.removeAllElements();
    bookmarksModelChangedNotifier.fireModelChanged(null);
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param bookmarksModelChangedListener the <code>Observer</code> to be added
   */
  public void addBookmarksModelChangedListener(Observer bookmarksModelChangedListener) {
    bookmarksModelChangedNotifier.addObserver(bookmarksModelChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param bookmarksModelChangedListener the <code>Observer</code> to be removed
   */
  public void removeBookmarksModelChangedListener(Observer bookmarksModelChangedListener) {
    bookmarksModelChangedNotifier.deleteObserver(bookmarksModelChangedListener);
  }

  /**
   * Clear the bookmarks of features associated with the given Report.
   */
  public void removeAssociatedBookmarks(ReportsModel.ReportTreeNode reportTreeNode) {
    // DefaultComboBoxModel doesn't provide an iterator so copy to an array and use the array
    FeatureLine[] featureLines = new FeatureLine[size()];
    for (int i=0; i<featureLines.length; i++) {
      featureLines[i] = get(i);
    }

    // now remove the associated features from the model
    for (int i=0; i<featureLines.length; i++) {
      if (featureLines[i].isFromReport(reportTreeNode)) {
        removeBookmark(featureLines[i]);
      }
    }
  }
}


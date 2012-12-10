import java.util.Observer;
import java.util.Observable;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Dimension;
import java.awt.Rectangle;
import javax.swing.JComponent;
import javax.swing.UIManager;
import javax.swing.Scrollable;
import javax.swing.JViewport;

/**
 * A display area for the Feature listing.
 */
public class FeaturesComponent extends JComponent implements Scrollable {
  private static final long serialVersionUID = 1;

  private FeaturesModel featuresModel;
  private RangeSelectionManager rangeSelectionManager;
  private int rowHeight = 0;	// cache

  /**
   * Don't use setFont.  Font is entirely managed by BasicFeturesUI and fontSize from FeaturesModel.
   */
  public void setFont(Font f) {
    throw new RuntimeException("unexpected setFont interface used");
  }

  /**
   * Constructs a new <code>FeaturesComponent</code> with the given features model.
   * @param featuresModel the features model to use
   */
  public FeaturesComponent(FeaturesModel featuresModel, RangeSelectionManager rangeSelectionManager) {
    this.featuresModel = featuresModel;
    this.rangeSelectionManager = rangeSelectionManager;

    // initialize UI
    updateUI();

    // set background to opaque with List background color
    setOpaque(true);
    setBackground(UIManager.getColor("TextPane.background"));
  }

  /**
   * Sets the component size, note that the component will still need to be revalidated and painted.
   */
  public void setComponentSize(Dimension viewDimension) {
    setPreferredSize(viewDimension);
  }

  /**
   * Sets the L&F object that renders this component.
   * @param featuresUI  the <code>FeaturesUI</code> L&F object
   */
  public void setUI(FeaturesUI featuresUI) {
    super.setUI(featuresUI);
  }

  /**
   * Notification from the <code>UIFactory</code> that the L&F has changed.
   * Called to replace the UI with the latest version from the
   * <code>UIFactory</code>.
   */
  public void updateUI() {
    setUI((FeaturesUI)UIManager.getUI(this));
    invalidate();
  }

  /**
   * Returns the class ID for the UI.
   * @return the ID ("FeaturesUI")
   */
  public String getUIClassID() {
    return FeaturesUI.UI_CLASS_ID;
  }

  /**
   * Returns the <code>FeaturesModel</code> that is providing the data.
   */
  public FeaturesModel getFeaturesModel() {
    return featuresModel;
  }

  /**
   * Returns the <code>RangeSelectionManager</code> that is providing the range selection.
   */
  public RangeSelectionManager getRangeSelectionManager() {
    return rangeSelectionManager;
  }

  // Scrollable, adapted from javax.swing.text.JTextArea and JTextComponent
  /**
   * Returns the preferred size of the viewport for this features component.
   * @return a <code>Dimension</code> object containing the <code>preferredSize</code> of the <code>JViewport</code>
   *         which displays this features component
   * @see Scrollable#getPreferredScrollableViewportSize
   */
  public Dimension getPreferredScrollableViewportSize() {
    return getPreferredSize();
  }

  /**
   * Returns the scroll increment (in pixels) that completely exposes one new row.
   * <p>This method is called each time the user requests a unit scroll.
   * @param visibleRect the view area visible within the viewport
   * @param orientation (not used)
   * @param direction (not used)
   * @return the "unit" increment for scrolling vertically
   * @see Scrollable#getScrollableUnitIncrement
   */
  public int getScrollableUnitIncrement(Rectangle visibleRect, int orientation, int direction) {
    if (rowHeight == 0) {
      // recalculate cached rowHeight from font
      FontMetrics metrics = getFontMetrics(getFont());
      rowHeight = metrics.getHeight();
    }
    return rowHeight;
  }

  /**
   * Returns <code>visibleRect.height</code>.
   * @return <code>visibleRect.height</code>
   * @see Scrollable#getScrollableBlockIncrement
   */
  public int getScrollableBlockIncrement(Rectangle visibleRect, int orientation, int direction) {
    return visibleRect.height;
  }  
 
  /**
   * Returns true if the viewport is larger than the preferred width of the features component.
   * Otherwise returns false.
   * @return true if the viewport is larger than the preferred width of the features component,
   * otherwise returns false
   * @see Scrollable#getScrollableTracksViewportWidth
   */
  public boolean getScrollableTracksViewportWidth() {
    if (getParent() instanceof JViewport) {
      return (((JViewport)getParent()).getWidth() > getPreferredSize().width);
    }
    return false;
  }

  /**
   * Returns true if the viewport is larger than the preferred height of the features component.
   * Otherwise returns false.
   * @return true if the viewport is larger than the preferred height of the features component,
   * otherwise returns false
   * @see Scrollable#getScrollableTracksViewportHeight
   */
  public boolean getScrollableTracksViewportHeight() {
    if (getParent() instanceof JViewport) {
      return (((JViewport)getParent()).getHeight() > getPreferredSize().height);
    }
    return false;
  }
}


import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.Observable;
import java.util.Observer;
import java.util.Vector;
import java.util.Enumeration;
import java.io.File;
import javax.swing.plaf.ComponentUI;

/**
 * A basic implementation of {@code FeaturesUI}.
 */
public class BasicFeaturesUI extends FeaturesUI implements MouseListener, MouseMotionListener {
  private FeaturesComponent featuresComponent;
  private FeaturesModel featuresModel;	// FeaturesComponent's data model
  private FeaturesModelChangedListener featuresModelChangedListener;
  private RangeSelectionManager rangeSelectionManager; // FeaturesComponent's range selection manager
  private RangeSelectionManagerChangedListener rangeSelectionManagerChangedListener;
  private UserHighlightModelChangedListener userHighlightModelChangedListener;
  private FeatureLineSelectionManagerChangedListener featureLineSelectionManagerChangedListener;
  private int mouseDownLine;
  private int selectedLine = -1;

  // cached values set on each entry to paint()
  private FontMetrics textMetrics;
  private FontMetrics prefixMetrics;

  // cached values set on mouse drag and cleared on model change
  private boolean makingRangeSelection = false;
  private int minSelectionIndex = -1;
  private int maxSelectionIndex = -1;

  /**
   * Returns a new instance of <code>BasicFeaturesUI</code>.
   * 
   * @param c the <code>FeaturesComponent</code> object (not used)
   * @see ComponentUI#createUI
   * @return a new <code>BasicFeaturesUI</code> object
   */
  public static ComponentUI createUI(JComponent c) {
    return new BasicFeaturesUI();
  }

  /**
   * Binds <code>BasicFeaturesUI</code> with <code>FeaturesComponent</code>.
   * @param c the FeaturesComponent
   */
  public void installUI(JComponent c) {
    featuresComponent = (FeaturesComponent)c;
    featuresModel = ((FeaturesComponent)c).getFeaturesModel();
    rangeSelectionManager = ((FeaturesComponent)c).getRangeSelectionManager();
    rangeSelectionManagerChangedListener = new RangeSelectionManagerChangedListener();
    rangeSelectionManager.addRangeSelectionManagerChangedListener(rangeSelectionManagerChangedListener);
    featuresComponent.addMouseListener(this);
    featuresComponent.addMouseMotionListener(this);
    featuresModelChangedListener = new FeaturesModelChangedListener();
    featuresModel.addFeaturesModelChangedListener(featuresModelChangedListener);
    userHighlightModelChangedListener = new UserHighlightModelChangedListener();
    BEViewer.userHighlightModel.addUserHighlightModelChangedListener(userHighlightModelChangedListener);
    featureLineSelectionManagerChangedListener = new FeatureLineSelectionManagerChangedListener();
    BEViewer.featureLineSelectionManager.addFeatureLineSelectionManagerChangedListener(
                                           featureLineSelectionManagerChangedListener);

    // NOTE: do not use wheel listener in this component
    // because then wheel events won't go to jscrollpane.

    // respond to keystrokes
    setKeyboardListener();
   }

  private void setKeyboardListener() {
    featuresComponent.addKeyListener(new KeyListener() {
      public void keyPressed(KeyEvent e) {
        // no action
        // ignore request if not active
        if (selectedLine <0) {
          // no action
        }

        if (e.getKeyText(e.getKeyCode()) == "Up") {
          selectLine(selectedLine - 1);
        } else if (e.getKeyText(e.getKeyCode()) == "Down") {
          selectLine(selectedLine + 1);
        }
      }

      // arror keys are action keys and thus do not generate keyTyped events
      public void keyReleased(KeyEvent e) {
        // no action
      }

      // actions for keystrokes
      public void keyTyped(KeyEvent e) {
        // escape
        if (e.getKeyChar() == KeyEvent.VK_ESCAPE) {
          // for ESCAPE deselect range if range is selected
          if (rangeSelectionManager.getProvider() == featuresModel) {
            rangeSelectionManager.clear();

            // clear range else clear selection
            return;
          }

          // also, if a histogram selection is in effect, deselect it and clear its filter
          FeaturesModel.ModelRole modelRole = featuresModel.getModelRole();
          if (selectedLine != -1
           && modelRole == FeaturesModel.ModelRole.HISTOGRAM_ROLE) {
            selectedLine = -1;
            BEViewer.referencedFeaturesModel.setFilterBytes(new byte[0]);

            // repaint to show deselection change
            featuresComponent.repaint();
          }
        }
      }
    });
  }

   /**
   * Unbinds <code>BasicFeaturesUI</code> from <code>FeaturesComponent</code>.
   * @param c the <code>FeaturesComponent</code> object (not used)
   */
  public void uninstallUI(JComponent c) {
    featuresComponent.removeMouseListener(this);
    featuresComponent.removeMouseMotionListener(this);
    featuresModel.removeFeaturesModelChangedListener(featuresModelChangedListener);
    BEViewer.userHighlightModel.removeUserHighlightModelChangedListener(userHighlightModelChangedListener);
    BEViewer.featureLineSelectionManager.removeFeatureLineSelectionManagerChangedListener(
                                              featureLineSelectionManagerChangedListener);
    featuresModelChangedListener = null;
    userHighlightModelChangedListener = null;
    featureLineSelectionManagerChangedListener = null;
    featuresComponent = null;
    featuresModel = null;
    featuresComponent = null;
  }

  /**
   * Returns the dimension in points of the view space
   * required to render the view provided by <code>ImageView</code>.
   */
  private Dimension getViewDimension() {
    final int width;
    final int height;

    if (featuresModel.getTotalLines() == 0) {
      width = 0;
      height = 0;
    } else {
      int lineHeight = featuresModel.getTotalLines();
      FeatureLine featureLine = featuresModel.getFeatureLine(0);
      String printableForensicPath = ForensicPath.getPrintablePath(featureLine.forensicPath, featuresModel.getUseHexPath());
      FontMetrics tempFontMetrics = featuresComponent.getFontMetrics(getTextFont());
      width = tempFontMetrics.stringWidth("W")
              * ((printableForensicPath.length() + 4) // 4 is plenty to account for extra decimal places
              + featuresModel.getWidestLineLength());
      height = tempFontMetrics.getHeight() * lineHeight;
    }
    return new Dimension(width, height);
  }

  /**
   * Returns the text font.
   */
  public Font getTextFont() {
    // set monospaced plain for readability
    return new Font("serif", Font.PLAIN, featuresModel.getFontSize());
  }

  /**
   * Returns the prefix font.
   */
  private Font getPrefixFont() {
    // set monospaced plain for readability
    return new Font("monospaced", Font.PLAIN, featuresModel.getFontSize());
  }

  /**
   * Invoked by Swing to draw the lines of text for the Features Component.
   */
  public void paint(Graphics g, JComponent c) {
    // NOTE: Issue: bug in Utilities.drawTabbedText when using large values of y.
    //      Utilities.drawTabbedText(Segment text, int x, int y, Graphics g, null, 0);
    // Instead, use Graphics.drawString(String string, int x, int y).
    // NOTE: adapted from javax.swing.text.PlainView
    // NOTE: use SIMPLE_SCROLL_MODE in JViewport else clip gets overoptimized
    // and fails to render background color changes properly.
    // NOTE: for highlighting, we use setColor(Color.YELLOW) rather than take an L&F color
    // from UIManager because UIManager has no equivalent.
    // NOTE: this model does not support tabs.  Specifically, it manages the one expected
    // tab after the forensic path or historam but does not manage any other tabs.
    // If tab management is required in the future, either replace tab with space (simple)
    // or call g.drawString multiple times, once per tab, setting y for each tabstop width.
    // NOTE: Swing hangs if array is to large, so strings are truncated in FeatureFieldFormatter.

    Rectangle alloc;	// size of entire space
    Rectangle clip;	// size of view clip within entire space

    alloc = new Rectangle(c.getPreferredSize());
    clip = g.getClipBounds();

    // define geometry based on font
    Font textFont = getTextFont();
    Font prefixFont = getPrefixFont();
    textMetrics = g.getFontMetrics(textFont); // do not recalculate for duration of paint call
    prefixMetrics = g.getFontMetrics(prefixFont); // do not recalculate for duration of paint call
    int tabWidth = prefixMetrics.stringWidth("w") * 8;
    int fontHeight = textMetrics.getHeight();
    int heightBelow = (alloc.y + alloc.height) - (clip.y + clip.height);
    int linesBelow = Math.max(0, heightBelow / fontHeight);
    int heightAbove = clip.y - alloc.y;
    int linesAbove = Math.max(0, heightAbove / fontHeight);
    int linesTotal = alloc.height / fontHeight;
    int ascent = textMetrics.getAscent();

    // optimization: cache colors that are looked up
    final Color SELECTION_BACKGROUND_COLOR = UIManager.getColor("List.selectionBackground");

    // define the drawing area
    if (alloc.height % fontHeight != 0) {
      linesTotal++;
    }
    Rectangle lineArea = new Rectangle(alloc.x, alloc.y + (linesAbove * fontHeight),
                                       alloc.width, fontHeight);
    int y = lineArea.y;
    int x = lineArea.x;
    int lineCount = featuresModel.getTotalLines();
    int endLine = Math.min(lineCount, linesTotal - linesBelow);

    // NOTE: Swing bug: background rectangle fails to paint if it is too wide or too high, ~32K.
    // So: manually paint background if wider than arbitrary large amount:
    if (alloc.width > 20000 || alloc.height > 20000) {
      g.setColor(UIManager.getColor("List.background"));
      g.fillRect(alloc.x, alloc.y, 20000, 20000);
    }

    // render each line in the drawing area
    for (int line = linesAbove; line < endLine; line++) {

      // get FeatureLine
      FeatureLine featureLine = featuresModel.getFeatureLine(line);

      // a bad feature line can generate a warning, which interferes
      // with the UI in a cyclic way, which is fatal, so to cope, clear
      // the report selection.  This solution is jarring but is not fatal.
      if (featureLine.isBad()) {
        BEViewer.reportSelectionManager.setReportSelection(null, null);
        return;
      }

      // calculate text and geometry of the feature line's prefix
      String prefixString = ForensicPath.getPrintablePath(featureLine.forensicPath, featuresModel.getUseHexPath());
      int prefixWidth = prefixMetrics.stringWidth(prefixString);
      int tabbedPrefixWidth = prefixWidth + (tabWidth - (prefixWidth % tabWidth));

      // get the feature text into usable local variables
      final String featureString = featureLine.formattedFeature;
      final char[] featureCharArray = featureString.toCharArray();

      // get text width handy if needed
      int textWidth = textMetrics.charsWidth(featureCharArray, 0, featureCharArray.length);

      // draw the range selection background
      if (line >= minSelectionIndex && line <= maxSelectionIndex) {
        g.setColor(SELECTION_BACKGROUND_COLOR);
        g.fillRect(x, y, tabbedPrefixWidth + textWidth, fontHeight);
      }

      // draw the selected line's selection background
      if (line == selectedLine) {
        g.setColor(SELECTION_BACKGROUND_COLOR.darker());
//        g.fillRect(x, y, tabbedPrefixWidth + textWidth, fontHeight);
        g.fillRect(x, y, tabbedPrefixWidth, fontHeight);
      }

      // paint the highlight background for any highlights
      g.setColor(Color.YELLOW);	// desired color.
      Vector<FeatureHighlightIndex> highlightIndexes = FeatureHighlightIndex.getHighlightIndexes(featureCharArray);
      for (Enumeration<FeatureHighlightIndex> e = highlightIndexes.elements(); e.hasMoreElements(); ) {
        FeatureHighlightIndex highlightIndex = e.nextElement();
        int beginIndex = highlightIndex.beginIndex;
        int length = highlightIndex.length;
        int xHighlightStart = tabbedPrefixWidth + textMetrics.charsWidth(featureCharArray, 0, beginIndex);
        int xHighlightWidth = textMetrics.charsWidth(featureCharArray, beginIndex, length);
        g.fillRect(x + xHighlightStart, y, xHighlightWidth, fontHeight);
      }

      // set the color for drawing text
      g.setColor(c.getForeground());

      // draw the prefix of the feature line
      g.setFont(prefixFont);
      g.drawString(prefixString, x, y + ascent);

      // draw the Feature portion of the feature line
      g.setFont(textFont);
      g.drawString(featureString, x + tabbedPrefixWidth, y + ascent);
// NOTE: g.drawChars() has Y alignment issues on long lists while g.drawString() does not.

      // move to next line
      y += fontHeight;
    }
  }

  // mouse listener

  /**
   * Not used.
   * @param e the mouse event
   */
  public void mouseEntered(MouseEvent e) {
  }

  /**
   * Not used.
   * @param e the mouse event
   */
  public void mouseExited(MouseEvent e) {
  }

  /**
   * Repaints the component if the mouse movement has changed the selectable feature line.
   * Not used.
   * @param e the mouse event
   */
  public void mouseMoved(MouseEvent e) {
  }

  /**
   * Identify the hovered line number where the mouse goes down.
   * @param e the mouse event
   */
  public void mousePressed(MouseEvent e) {
    // give the window focus
    featuresComponent.requestFocusInWindow();

    // set the line index of mouse down
    int line = e.getY() / textMetrics.getHeight();
    if (line >= 0 && line < featuresModel.getTotalLines()) {
      mouseDownLine = line;
    } else {
      mouseDownLine = -1;
    }

    // indicate that a range selection is not in progress
    makingRangeSelection = false;
  }

  // MouseMotionListener
  /**
   * Track any range selection.
   * @param e the mouse event
   */
  public void mouseDragged(MouseEvent e) {
    // range selection happens when mouse drags
    if (mouseDownLine != -1) {

      // indicate that a range selection is in progress
      if (makingRangeSelection == false) {
        makingRangeSelection = true;
        // clear out any other selection when this selection starts
        rangeSelectionManager.setRange(featuresModel, mouseDownLine, mouseDownLine);
      }

      // get the line that the mouse is on, but keep the line within the valid line range
      int line = e.getY() / textMetrics.getHeight();
      if (line < 0) {
        line = 0;
      }
      if (line >= featuresModel.getTotalLines()) {
        line = featuresModel.getTotalLines() - 1;
      }

      // find the new range selection
      int newMinSelectionIndex;
      int newMaxSelectionIndex;
      if (mouseDownLine <= line) {
        newMinSelectionIndex = mouseDownLine;
        newMaxSelectionIndex = line;
      } else {
        newMinSelectionIndex = line;
        newMaxSelectionIndex = mouseDownLine;
      }

      // if the new range selection is different then take it and repaint
      if (newMinSelectionIndex != minSelectionIndex || newMaxSelectionIndex != maxSelectionIndex) {
        minSelectionIndex = newMinSelectionIndex;
        maxSelectionIndex = newMaxSelectionIndex;
        featuresComponent.repaint();
      }
    }
  }

  /**
   * Take the range selection
   * @param e the mouse event
   */
  public void mouseReleased(MouseEvent e) {
    if (makingRangeSelection) {
      rangeSelectionManager.setRange(featuresModel, minSelectionIndex, maxSelectionIndex);
    }
  }

  // select the specified line
  private void selectLine(int line) {

    // ignore request if out of range
    if (line < 0 || line >= featuresModel.getTotalLines()) {
      return;
    }

    // select the requested line
    selectedLine = line;

    // action depends on feature model type: FEATURES or HISTOGRAM
    FeaturesModel.ModelRole modelRole = featuresModel.getModelRole();
    if (modelRole == FeaturesModel.ModelRole.FEATURES_ROLE) {

      // set the feature selection
      BEViewer.featureLineSelectionManager.setFeatureLineSelection(
                                         featuresModel.getFeatureLine(selectedLine));

    } else if (modelRole == FeaturesModel.ModelRole.HISTOGRAM_ROLE) {

      // filter the histogram features model by the feature field
      byte[] matchableFeature = featuresModel.getFeatureLine(selectedLine).featureField;
      BEViewer.referencedFeaturesModel.setFilterBytes(matchableFeature);
    } else {
      throw new RuntimeException("Invalid type");
    }

    // repaint to show view of changed model
    featuresComponent.repaint();
  }
    
  /**
   * Selects the feature line if the cursor is hovering over a valid forensic path.
   * @param e the mouse event
   */
  public void mouseClicked(MouseEvent e) {
    // for mouse clicked deselect range if range is selected
    if (rangeSelectionManager.getProvider() == featuresModel) {
      rangeSelectionManager.clear();
    }

    // also perform a feature line selection operation if hovering over a selectable feature line
    if (mouseDownLine >= 0) {
      selectLine(mouseDownLine);
    }
  }

  private int getLineWidth(FeatureLine featureLine) {
    // calculate width of the feature line's prefix
      String prefixString = ForensicPath.getPrintablePath(featureLine.forensicPath, featuresModel.getUseHexPath());
    int prefixWidth = prefixMetrics.stringWidth(prefixString);

    // calculate width of the tab
    int tabWidth = prefixMetrics.stringWidth("w") * 8;
    int tabbedWidth = tabWidth - (prefixWidth % tabWidth);

    // calculate width of the displayed feature text
    String featureString = featureLine.formattedFeature;
    int featureWidth = textMetrics.stringWidth(featureString);

    // return the width of the line
    return prefixWidth + tabbedWidth + featureWidth;
  }

  private int getHoveredFeatureLineIndex(int pointX, int pointY) {
    int line;

    // determine line number based on pointY
    int fontHeight = textMetrics.getHeight();
    line = (pointY < 0) ? -1 : pointY / fontHeight;

    // pointed line must be within valid line range
    if (line >= 0 && line < featuresModel.getTotalLines()) {

      // get the line identified by x, y
      FeatureLine featureLine = featuresModel.getFeatureLine(line);

      // determine that x, y is within the rectangle of the paintable line
      int lineWidth = getLineWidth(featureLine);
      if (pointX >= 0 && pointX < lineWidth) {
        return line;
      }
    }
    return -1;
  }

  // The view changes when the features model changes
  private class FeaturesModelChangedListener implements Observer {
    // Observer
    public void update(Observable o, Object arg) {
      FeaturesModel.ChangeType changeType = featuresModel.getChangeType();
      if (changeType == FeaturesModel.ChangeType.FEATURES_CHANGED) {
        // the feature content changed so clear any selections and resize

        // if a histogram selection is in effect then clear the referenced feature
        // NOTE this may be obscure here but the deselection action is what clears the
        // referenced features model.
        if (selectedLine >= 0
         && featuresModel.getModelRole() == FeaturesModel.ModelRole.HISTOGRAM_ROLE) {
          BEViewer.referencedFeaturesModel.setFilterBytes(new byte[0]);
        }

        // undo the selected feature line, if any
        selectedLine = -1;

        // if a range selection is active then clear it
        if (rangeSelectionManager.getProvider() == featuresModel) {
          BEViewer.rangeSelectionManager.clear();
        }

        // resize since the lines changed
        featuresComponent.setComponentSize(getViewDimension());
        featuresComponent.revalidate();

        

      } else if (changeType == FeaturesModel.ChangeType.FORMAT_CHANGED) {
        // resize since the width changed
        featuresComponent.setComponentSize(getViewDimension());
        featuresComponent.revalidate();
      }

      // repaint to show view of changed model
      featuresComponent.repaint();
    }
  }

  // the view is redrawn when highlighting changes
  private class UserHighlightModelChangedListener implements Observer {
    // Observer
    public void update(Observable o, Object arg) {
      // redraw to render highlight changes
      featuresComponent.repaint();
    }
  }

  // this class listens for a feature line selection change to turn off the selected feature line
  // when the user navigates elsewhere
  private class FeatureLineSelectionManagerChangedListener implements Observer {
    // Observer
    public void update(Observable o, Object arg) {

      // deselect the selected line if the selection changes
      if (selectedLine >= 0
       && featuresModel.getModelRole() == FeaturesModel.ModelRole.FEATURES_ROLE) {
        if (!featuresModel.getFeatureLine(selectedLine).equals(
             BEViewer.featureLineSelectionManager.getFeatureLineSelection())) {

          // deselect the selected feature line if it is not a histogram line
          // and it no longer matches the selection in the image model
          selectedLine = -1;
        }
      }

      // redraw to render highlight change and also any selection change
      featuresComponent.repaint();
    }
  }

  // this listener is notified when the range selection changes
  private class RangeSelectionManagerChangedListener implements Observer {
    public void update(Observable o, Object arg) {
      // clear range if the change is from another provider
      if (rangeSelectionManager.getProvider() != featuresModel) {
        minSelectionIndex = -1;
        maxSelectionIndex = -1;
        featuresComponent.repaint();
      }
    }
  }
}


import java.awt.*;
import java.awt.event.*;
import java.util.Observable;
import java.util.Observer;
import javax.swing.*;
import javax.swing.plaf.ComponentUI;

/**
 * A basic implementation of {@code ImageUI}.
 */
public class BasicImageUI extends ImageUI implements MouseListener, MouseMotionListener {
  private ImageComponent imageComponent;
  private ImageView imageView;	// Image view's data model
  private ImageViewChangedListener imageViewChangedListener;
  private RangeSelectionManager rangeSelectionManager; // ImageView's selection manager
  private RangeSelectionManagerChangedListener rangeSelectionManagerChangedListener; // ImageView's selection manager
  private boolean makingRangeSelection;
  private int rangeMouseDownLine;

  private int fontHeight;		// set on each entry to paint()
  private int monospaceWidth;		// set on each entry to paint()
  private int lineCount;		// set on each entry to paint()

  private int mouseDownLine = -1;
  private int minSelectionIndex = -1;
  private int maxSelectionIndex = -1;
 
  /**
   * Returns a new instance of <code>BasicImageUI</code>.
   * 
   * @param c the <code>ImageComponent</code> object (not used)
   * @see ComponentUI#createUI
   * @return a new BasicImageUI object
   */
  public static ComponentUI createUI(JComponent c) {
    return new BasicImageUI();
  }

  /**
   * Binds <code>BasicImageUI</code> with <code>ImageComponent</code>.
   * @param c the <code>ImageComponent</code> object
   */
  public void installUI(JComponent c) {
    imageComponent = (ImageComponent)c;
    imageView = ((ImageComponent)c).getImageView();
    rangeSelectionManager = ((ImageComponent)c).getRangeSelectionManager();
    rangeSelectionManagerChangedListener = new RangeSelectionManagerChangedListener();
    rangeSelectionManager.addRangeSelectionManagerChangedListener(rangeSelectionManagerChangedListener);
    makingRangeSelection = false;
    imageComponent.addMouseListener(this);
    imageComponent.addMouseMotionListener(this);
    imageViewChangedListener = new ImageViewChangedListener();
    imageView.addImageViewChangedListener(imageViewChangedListener);

    // do not use wheel listener in this component
    // because then wheel events won't go to jscrollpane.

    // respond to keystrokes
    setKeyboardListener();
  }

  private void setKeyboardListener() {
    imageComponent.addKeyListener(new KeyListener() {
      public void keyPressed(KeyEvent e) {
        // no action
      }
      public void keyReleased(KeyEvent e) {
        // no action
      }
      public void keyTyped(KeyEvent e) {
        // actions are defined for keystrokes, currently juste ESCAPE
        if (e.getKeyChar() == KeyEvent.VK_ESCAPE) {
          // for ESCAPE deselect range if range is selected
          if (rangeSelectionManager.getProvider() == imageView) {
            rangeSelectionManager.clear();
          }
        }
      }
    });
  }

  /**
   * Unbinds <code>BasicImageUI</code> from <code>ImageComponent</code>.
   * @param c the <code>ImageComponent</code> object (not used)
   */
  public void uninstallUI(JComponent c) {
    imageComponent.removeMouseListener(this);
    imageComponent.removeMouseMotionListener(this);
    imageView.removeImageViewChangedListener(imageViewChangedListener);
    imageViewChangedListener = null;
    imageComponent = null;
    rangeSelectionManager.removeRangeSelectionManagerChangedListener(rangeSelectionManagerChangedListener);
  }

  /**
   * Returns the font that BasicImageUI uses.
   */
  public Font getImageFont() {
    // set monospaced plain for readability
    return new Font("monospaced", Font.PLAIN, imageView.getFontSize());
  }

  // scroll to path else scroll to top
  private void scrollToForensicPath() {
    Rectangle rectangle;

    // get the forensic path line index
    final int forensicPathLineIndex = imageView.getForensicPathLineIndex(imageView.getImagePage().featureLine.forensicPath);

    // scroll based on index
    if (forensicPathLineIndex == -1) {

      // the forensic path to be scrolled to is not in this page,
      // so scroll to top
      rectangle = new Rectangle(0, 0, 0, 0);

    } else {
      // the forensic path is in this page, so scroll to it
      // calculate the line's pixel address based on font height
      int y = forensicPathLineIndex * fontHeight;

      // scroll to the pixel address allowing a rectangular margin of three lines below
      rectangle = new Rectangle(0, y, 0, fontHeight * 3);
    }

    // scroll to the rectangle containing the feature
    imageComponent.scrollRectToVisible(rectangle);
    // NOTE: Swing scrolls to the top on the first request but scrolls correctly on the second
    // request, so we issue scroll twice.
    // Specifically, this is consistently repeatable by scrolling from a lower location
    // on a taller window to a higher location on a shorter window.
    imageComponent.scrollRectToVisible(rectangle);
  }

  /**
   * Returns the dimension in points of the view space
   * required to render the view provided by <code>ImageView</code>.
   */
  private Dimension getViewDimension() {
    int lineHeight = imageView.getNumLines();
    int lineWidth = (lineHeight == 0) ? 0 : imageView.getLine(0).text.length();

    // return the dimension of the page using font metrics
    FontMetrics tempFontMetrics = imageComponent.getFontMetrics(getImageFont());
    int width = tempFontMetrics.stringWidth(" ") * lineWidth;
    int height = tempFontMetrics.getHeight() * lineHeight;
    return new Dimension(width, height);
  }

  /**
   * Invoked by Swing to render the lines of text for the Image Component.
   * @param g the graphics object
   * @param c the component to render to
   */
  public void paint(Graphics g, JComponent c) {
    // NOTE: use SIMPLE_SCROLL_MODE in JViewport else clip gets overoptimized
    // and fails to render background color changes properly.
    // NOTE: paint code adapted from javax.swing.text.PlainView
    // NOTE: g.drawChars() has Y alignment issues on long lists while g.drawString() does not.

//System.out.println("BasicImageUI.paint");

    Rectangle alloc;	// size of entire space
    Rectangle clip;	// size of view clip within entire space

    alloc = new Rectangle(c.getPreferredSize());
    clip = g.getClipBounds();

    // define geometry based on font
    Font imageFont = getImageFont();
    FontMetrics fontMetrics = g.getFontMetrics(imageFont); // do not recalculate for duration of paint call
    monospaceWidth = fontMetrics.stringWidth(" ");
    fontHeight = fontMetrics.getHeight();
    int heightBelow = (alloc.y + alloc.height) - (clip.y + clip.height);
    int linesBelow = Math.max(0, heightBelow / fontHeight);
    int heightAbove = clip.y - alloc.y;
    int linesAbove = Math.max(0, heightAbove / fontHeight);
    int linesTotal = alloc.height / fontHeight;
    int ascent = fontMetrics.getAscent();

    // define the drawing area
    if (alloc.height % fontHeight != 0) {
      linesTotal++;
    }
    Rectangle lineArea = new Rectangle(alloc.x, alloc.y + (linesAbove * fontHeight),
                                       alloc.width, fontHeight);
    int y = lineArea.y;
    int x = lineArea.x;

    lineCount = imageView.getNumLines();
    int endLine = Math.min(lineCount, linesTotal - linesBelow);


    // render each line in the drawing area
    g.setFont(imageFont);
    for (int line = linesAbove; line < endLine; line++) {
      // prepare the line
      ImageLine imageLine = imageView.getLine(line);

      // draw the range selection background
      if (line >= minSelectionIndex && line <= maxSelectionIndex) {
        g.setColor(UIManager.getColor("List.selectionBackground"));
        int xWidth = fontMetrics.stringWidth(imageLine.text);
        g.fillRect(0, y, xWidth, fontHeight);
      }

      // draw the highlight background for each highlight index in the highlight array
      g.setColor(Color.YELLOW);
      for (int i =0; i< imageLine.numHighlights; i++) {
        int index = imageLine.highlightIndexes[i];
        g.fillRect(index * monospaceWidth, y, monospaceWidth, fontHeight);
      }

      // draw the line's text
      g.setColor(c.getForeground());
      g.drawString(imageLine.text, x, y + ascent);

      // move to the next line
      y += fontHeight;
    }
  }

  // mouse listener
  /**
   * Not used.
   * @param e the mouse event
   */
  public void mouseClicked(MouseEvent e) {
//WLog.log("BasicImageUI.MouseClicked");
    // for mouse click deselect range if range is selected
    if (rangeSelectionManager.getProvider() == imageView) {
      rangeSelectionManager.clear();
    }
  }
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
   * If the cursor is over a line, this starts a range selection
   * @param e the mouse event
   */
  public void mousePressed(MouseEvent e) {
//WLog.log("BasicImageUI.mousePressed");
    // give the window focus
    imageComponent.requestFocusInWindow();

    // if cursor is over a line, start a range selection
    int line = e.getY() / fontHeight;
    if (line >= 0 && line < lineCount) {
      mouseDownLine = line;
    } else {
      mouseDownLine = -1;
    }

    // indicate that a range selection is not in progress
    makingRangeSelection = false;
  }

  // MouseMotionListener
  /**
   * If a range selection is in progress,
   * this updates the selection view as the selection changes.
   * @param e the mouse event
   */
  public void mouseDragged(MouseEvent e) {
//WLog.log("BasicImageUI.mouseDragged");
    // range selection happens when mouse drags
    if (mouseDownLine != -1) {

      // indicate that a range selection is in progress
      if (makingRangeSelection == false) {
        makingRangeSelection = true;
        // clear out any other selection when this selection starts
        rangeSelectionManager.setRange(imageView, mouseDownLine, mouseDownLine);
      }

      // get the line that the mouse is on, but keep the line within the valid line range
      int line = e.getY() / fontHeight;
      if (line < 0) {
        line = 0;
      }
      if (line >= lineCount) {
        line = lineCount - 1;
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
        imageComponent.repaint();
      }
    }
  }

  /**
   * If a range selection is in progress, this accepts the selection.
   * @param e the mouse event
   */
  public void mouseReleased(MouseEvent e) {
    if (makingRangeSelection) {
//WLog.log("BasicImageUI.mouseReleased: " + minSelectionIndex);
      rangeSelectionManager.setRange(imageView, minSelectionIndex, maxSelectionIndex);
    }
  }

  /**
   * Not used.
   * @param e the mouse event
   */
  public void mouseMoved(MouseEvent e) {
  }

  // this listener is notified when the image view changes.
  private class ImageViewChangedListener implements Observer {
    public void update(Observable o, Object arg) {

      // action depends on change type
      ImageView.ChangeType changeType = imageView.getChangeType();
      if (changeType == ImageView.ChangeType.IMAGE_PAGE_CHANGED
       || changeType == ImageView.ChangeType.LINE_FORMAT_CHANGED) {

        // with these changes, the content of the lines change,
        // so resize and clear any range selection, then scroll to desired position

        // set the component's new size based on current image line provider content
        imageComponent.setComponentSize(getViewDimension());
        imageComponent.revalidate();

        // if a range selection was active then clear it
        if (rangeSelectionManager.getProvider() == imageView) {
          rangeSelectionManager.clear();
        }

        // scroll to the feature line else scroll to top
        scrollToForensicPath();

        // NOTE: scrollToForensicPath does not issue repaint if the scroll doesn't change
        // so we must repaint wheather scrollpane did so or not.
        imageComponent.repaint();

      } else if (changeType == ImageView.ChangeType.USER_HIGHLIGHT_CHANGED) {
        // although highlighting changed, geometry did not change.
        imageComponent.repaint();
      } else if (changeType == ImageView.ChangeType.FORENSIC_PATH_NUMERIC_BASE_CHANGED
              || changeType == ImageView.ChangeType.FONT_SIZE_CHANGED) {
        // line text changed but the addressing to the lines remains the same
        imageComponent.setComponentSize(getViewDimension());
        imageComponent.revalidate();
        imageComponent.repaint();
      } else {
        throw new RuntimeException("Unexpected change type.");
      }
    }
  }

  // this listener is notified when the range selection changes
  private class RangeSelectionManagerChangedListener implements Observer {
    public void update(Observable o, Object arg) {
      // clear range if the change is from another provider
      if (rangeSelectionManager.getProvider() != imageView) {
        minSelectionIndex = -1;
        maxSelectionIndex = -1;
        imageComponent.repaint();
      }
    }
  }
}


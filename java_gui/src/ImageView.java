import java.io.File;
import java.util.Vector;
import java.util.Observer;
import java.util.Observable;

/**
 * The <code>ImageView</code> class provides image view from page bytes
 * formatted according to <code>ImageLine.LineFormat</code>  and from user highlights.
 */

public class ImageView implements CopyableLineInterface {

  private final int HEX_BYTES_PER_LINE = 16;
  private final int TEXT_BYTES_PER_LINE = 64;

  // resources
  private final ImageModel imageModel;
  private final UserHighlightModel userHighlightModel;
  private final ImageHighlightProducer imageHighlightProducer;
  private final ModelChangedNotifier<Object> imageViewChangedNotifier = new ModelChangedNotifier<Object>();

  // cached state
  private ImageModel.ImagePage imagePage;
//  private Vector<byte[]> featureHighlights = new Vector<byte[]>();
  private Vector<byte[]> userHighlights = new Vector<byte[]>(); // managed by this model
  private boolean highlightMatchCase;

  // calculated state
  private final Vector<ImageLine> lines = new Vector<ImageLine>();

  // input used to generate image view
  private File pageFile = new File();
  private String pageForensicPath = "";
  private byte[] pageBytes = new byte[0];
  private boolean[] pageHighlightFlags = new boolean[0];
  private boolean useHexPath = false;
  private ImageLine.LineFormat lineFormat = ImageLine.LineFormat.HEX_FORMAT;
  private int fontSize = 12;
  
  // this output state allows listeners to know the type of the last change
  private ChangeType changeType = ChangeType.IMAGE_PAGE_CHANGED; // indicates fullest change

  /**
   * Constructs an Image view and registers it to listen to image model changes.
   */
  public ImageView(ImageModel _imageModel, UserHighlightModel _userHighlightModel) {
    // resources
    this.imageModel = _imageModel;
    this.userHighlightModel = _userHighlightModel;
    imageHighlightProducer = new ImageHighlightProducer();

    // image model listener
    imageModel.addImageModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        ImageModel.ImagePage imagePage = imageModel.getImagePage();
        setImagePage(imagePage);
      }
    });

    // user highlight listener
    userHighlightModel.addUserHighlightModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        Vector<byte[]> userHighlights = userHighlightModel.getUserHighlightByteVector();
        boolean highlightMatchCase = userHighlightModel.isHighlightMatchCase();
        setUserHighlights(userHighlights, highlightMatchCase);
      }
    });
  }

  /**
   * Constructs an Image view that is not wired to other models, useful for generating Reports.
   */
  public ImageView() {
    imageModel = null;
    userHighlightModel = null;
    imageHighlightProducer = new ImageHighlightProducer();
  }

  /**
   * The <code>ChangeType</code> class identifies the type of the change that was last requested.
   */
  public static final class ChangeType {
    public static final ChangeType IMAGE_PAGE_CHANGED = new ChangeType("Image page changed");
    public static final ChangeType USER_HIGHLIGHT_CHANGED = new ChangeType("User highlight changed");
    public static final ChangeType ADDRESS_FORMAT_CHANGED = new ChangeType("Address format changed");
    public static final ChangeType LINE_FORMAT_CHANGED = new ChangeType("Line format changed");
    public static final ChangeType FONT_SIZE_CHANGED = new ChangeType("Font size changed");
    private final String name;
    private ChangeType(String name) {
      this.name = name;
    }
    public String toString() {
      return name;
    }
  }

  /**
   * Sets the image view for the given image page.
   */
  public void setImagePage(ImageModel.ImagePage imagePage) {
    this.imagePage = imagePage; // save for use with imageHighlightProducer.getPageHighlightFlags

    // set state that changes for this call
    if (imagePage == null) {
      // clear page values
      pageForensicPath = "";
      pageBytes = new byte[0];
      pageHighlightFlags = new boolean[0];
    } else {
      // set page values
      pageForensicPath = imagePage.pageForensicPath;
      pageBytes = imagePage.pageBytes;
      pageHighlightFlags = imageHighlightProducer.getPageHighlightFlags(
                              imagePage, userHighlights, highlightMatchCase);
    }

    // make change
    setLines(ChangeType.IMAGE_PAGE_CHANGED);
  }

  /**
   * Returns the image page that is currently set in the image view.
   */
  public ImageModel.ImagePage getImagePage() {
    return imagePage;
  }

  /**
   * Sets the user highlight values for the image view.
   */
  public void setUserHighlights(Vector<byte[]> userHighlights, boolean highlightMatchCase) {
    // set state changes for this call
    this.userHighlights = userHighlights;
    this.highlightMatchCase = highlightMatchCase;
    if (imagePage == null) {
      // clear highlights
      pageHighlightFlags = new boolean[0];
    } else {
      pageHighlightFlags = imageHighlightProducer.getPageHighlightFlags(
                              imagePage, userHighlights, highlightMatchCase);
    }

    // make the changes
    setLines(ChangeType.USER_HIGHLIGHT_CHANGED);
  }

  /**
   * Sets the address format associated with the currently active image.
   */
  public void setUseHexPath(boolean useHexPath) {
    this.useHexPath = useHexPath;
    setLines(ChangeType.ADDRESS_FORMAT_CHANGED);
  }

  /**
   * Returns the address format associated with the currently active image.
   */
  public boolean getUseHexPath() {
    return useHexPath;
  }

  /**
   * Sets the line format associated with the currently active image.
   */
  public void setLineFormat(ImageLine.LineFormat lineFormat) {
    this.lineFormat = lineFormat;
    setLines(ChangeType.LINE_FORMAT_CHANGED);
  }

  /**
   * Returns the change type that resulted in the currently active image.
   */
  public ImageView.ChangeType getChangeType() {
    return changeType;
  }

  /**
   * Returns the line format associated with the currently active image.
   */
  public ImageLine.LineFormat getLineFormat() {
    return lineFormat;
  }

  /**
   * Sets font size.
   */
  public void setFontSize(int fontSize) {
    this.fontSize = fontSize;
    setLines(ChangeType.FONT_SIZE_CHANGED);
  }

  /**
   * Returns the font size.
   */
  public int getFontSize() {
    return fontSize;
  }

  /**
   * Returns the requested paintable line.
   * @param line the line number of the line to return
   */
  public ImageLine getLine(int line) {
    if (line >= lines.size()) {
      throw new IndexOutOfBoundsException();
    }
    return lines.get(line);
  }

  /**
   * Returns the number of paintable lines.
   */
  public int getNumLines() {
    return lines.size();
  }

  /**
   * Sets the image line
   * @param pageStartAddress the start address for generated feature lines
   * @param pageBytes the bytes of the page
   * @param pageHighlightFlags the highlight flags associated with the page bytes
   * @param useHexPath print address in decimal or hex
   * @param lineFormat the line format to use for creating the line text
   */
  private void setLines(ChangeType changeType) {

    this.changeType = changeType;

    // clear any old lines
    lines.clear();

    // set image lines based on line format
    if (lineFormat == ImageLine.LineFormat.HEX_FORMAT) {
      setHexLines();
    } else if (lineFormat == ImageLine.LineFormat.TEXT_FORMAT) {
      setTextLines();
    } else {
      throw new IllegalArgumentException();
    }
    imageViewChangedNotifier.fireModelChanged(null);
  }

  // generate lines in hex view format
  private void setHexLines() {
    final int MAX_CHARS = 80;

    // work through bytes, preparing image lines
    int pageOffset;
    for (pageOffset = 0; pageOffset < pageBytes.length; pageOffset += HEX_BYTES_PER_LINE) {

      // image line attributes to be prepared
      final String lineForensicPath;
      final String text;
      final int[] highlightIndexes = new int[MAX_CHARS];
      int numHighlights = 0;

      // prepare this image line
      StringBuffer textBuffer = new StringBuffer(MAX_CHARS);

      // determine the line's absolute byte offset from the start of the image
      // determine the line's forensic path based on the page offset
      lineForensicPath = ForensicPath.getAdjustedPath(pageForensicPath, pageOffset);

      // format: long address, binary values where legal, hex values
      // format: [00000000]00000000  ..fslfejl.......  00000000 00000000 00000000 00000000

      // add the long address
      String printablePath = ForensicPath.getPrintablePath(lineForensicPath, useHexPath);
      textBuffer.append(printablePath);

      // add space
      textBuffer.append("  ");

      // add the ascii values
      for (int i=0; i<HEX_BYTES_PER_LINE; i++) {

        if (pageOffset + i < pageBytes.length) {
          // there are more bytes

          // if the byte is to be highlighted, record the byte highlight index
          if (pageHighlightFlags[pageOffset + i]) {
            highlightIndexes[numHighlights++] = textBuffer.length();
          }

          // add byte text
          byte b = pageBytes[pageOffset + i];
          if (b > 31 && b < 127) {
            textBuffer.append((char)b);	// printable
          } else {
            textBuffer.append(".");	// not printable
          }

        } else {
          textBuffer.append(" ");	// past EOF
        }
      }
      
      // add the HEX_BYTES_PER_LINE hex values
      for (int i=0; i<HEX_BYTES_PER_LINE; i++) {

        // pre-space each hex byte
        if (i % 4 == 0) {
          // double-space bytes between word boundaries
          textBuffer.append("  ");
        } else {
          // single-space bytes within word boundaries
          textBuffer.append(" ");
        }

        // check for EOF
        if (pageOffset + i < pageBytes.length) {
          // there are more bytes

          // if the byte is to be highlighted, record the byte highlight index
          // for the high and low hex characters that represent the byte
          if (pageHighlightFlags[pageOffset + i]) {
            highlightIndexes[numHighlights++] = textBuffer.length();
            highlightIndexes[numHighlights++] = textBuffer.length() + 1;
          }
        
          // add the hex byte
          try {
            textBuffer.append(String.format("%1$02x", pageBytes[pageOffset + i]));
          } catch (Exception e) {
            throw new RuntimeException(e);
          }

        } else {
          // there are no more bytes
          textBuffer.append("  ");	// past EOF
        }
      }

      // create the line's text from the temporary text buffer
      text = textBuffer.toString();

      // create the image line
      ImageLine imageLine = new ImageLine(lineForensicPath, text, highlightIndexes, numHighlights);
      lines.add(imageLine);
    }
  }

  /**
   * Returns the image line number containing the feature line given the line format.
   */
  public int getForensicPathLineIndex(String forensicPath) {
    if (featureLine == null) {
      return -1;
    }

    // get bytes per line
    int bytesPerLine;
    if (lineFormat == ImageLine.LineFormat.HEX_FORMAT) {
      bytesPerLine = HEX_BYTES_PER_LINE;
    } else if (lineFormat == ImageLine.LineFormat.TEXT_FORMAT) {
      bytesPerLine = TEXT_BYTES_PER_LINE;
    } else {
      throw new IllegalArgumentException();
    }
    // get index based on line format
    long pathOffset = ForensicPath.getOffset(forensicPath);
    long pageOffset = ForensicPath.getOffset(pageForensicPath);
    long imageLine = (pathOffset - pageOffset) / bytesPerLine;

    // normalize
    if (imageLine < 0 || imageLine > lines.size()) {
      return -1;
    } else {
      return imageLine;
    }
  }

  // generate lines in text view format
  private void setTextLines() {

    // work through bytes, preparing image lines
    int pageOffset;
    for (pageOffset = 0; pageOffset < pageBytes.length; pageOffset += TEXT_BYTES_PER_LINE) {

      // image line attributes to be prepared
      final String lineForensicPath;
      final String text;
      final int[] highlightIndexes = new int[TEXT_BYTES_PER_LINE];
      int numHighlights = 0;

      // prepare this image line
      StringBuffer textBuffer = new StringBuffer(TEXT_BYTES_PER_LINE);

      // determine the line's absolute byte offset from the start of the image
      lineStartAddress = pageStartAddress + pageOffset;

      // format: long address, binary values where legal
      // format: [00000000]00000000  ..fslfejl.............................

      // add the long address
      String printablePath = ForensicPath.getPrintablePath(lineForensicPath, useHexPath);
      textBuffer.append(printablePath);

      // add space
      textBuffer.append("  ");

      // add the ascii values
      for (int i=0; i<TEXT_BYTES_PER_LINE; i++) {

        if (pageOffset + i < pageBytes.length) {
          // there are more bytes

          // if the byte is to be highlighted, record the byte highlight index
          if (pageHighlightFlags[pageOffset + i]) {
            highlightIndexes[numHighlights++] = textBuffer.length();
          }

          // add byte text
          byte b = pageBytes[pageOffset + i];
          if (b > 31 && b < 127) {
            textBuffer.append((char)b);	// printable
          } else {
            textBuffer.append(".");	// not printable
          }
        }
      }
      
      // create the line's text from the temporary text buffer
      text = textBuffer.toString();

      // create the image line
      ImageLine imageLine = new ImageLine(lineForensicPath, text, highlightIndexes, numHighlights);
      lines.add(imageLine);
    }
  }

  /**
   * Implements CopyableLineInterface to provide a copyable line as a String
   */
  public String getCopyableLine(int line) {
    return getLine(line).text;
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param imageViewChangedListener the <code>Observer</code> to be added
   */
  public void addImageViewChangedListener(Observer imageViewChangedListener) {
    imageViewChangedNotifier.addObserver(imageViewChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param imageViewChangedListener the <code>Observer</code> to be removed
   */
  public void removeImageViewChangedListener(Observer imageViewChangedListener) {
    imageViewChangedNotifier.deleteObserver(imageViewChangedListener);
  }
}


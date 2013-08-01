import java.awt.print.*;
import java.awt.Font;
import java.awt.Color;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;

public class ImageRangePrinter implements Printable {

  // global working values valid for duration of print call
  ImageView imageView = null;
  int minSelectionIndex = -1;
  int maxSelectionIndex = -1;
  ImageModel.ImagePage imagePage;
  Graphics g;
  Font textFont;
  Font monoFont;
  FontMetrics fontMetrics;
  int fontHeight;
  int ascent;
  int monospaceWidth;
  int x;
  int y;
  int linesPerPage;
  int headerSize;

  /**
   * Print the feature associated with this image view and range.
   */
  public void printImageRange(ImageView imageView, int minSelectionIndex, int maxSelectionIndex) {
    this.imageView = imageView;
    this.minSelectionIndex = minSelectionIndex;
    this.maxSelectionIndex = maxSelectionIndex;
    PrinterJob printerJob = PrinterJob.getPrinterJob();
    printerJob.setPrintable(this);
    boolean userSaysOK = printerJob.printDialog();
    if (userSaysOK) {
      try {
        printerJob.print();
      } catch (PrinterException e) {
        WError.showError("Image range print request failed", "Print failure", e);
      }
    }
  }

  /**
   * Java's PrinterJob calls print to print indexed pages to Graphics g.
   * This code has been adapted from BasicImageUI.
   */
  public int print(Graphics g, PageFormat pageFormat, int page) throws PrinterException {

    // validate that the image page is printable
    imagePage = imageView.getImagePage();
    if (imagePage.featureLine.actualImageFile == null || imagePage.featureLine.featuresFile == null) {
      WError.showError("Invalid Image content", "Print Error", null);
      return NO_SUCH_PAGE;
    }

    // establish working values
    this.g = g;
    textFont = new Font("serif", Font.PLAIN, 10);
    monoFont = new Font("monospaced", Font.PLAIN, 10);
    fontMetrics = g.getFontMetrics(monoFont);
    fontHeight = fontMetrics.getHeight();
    ascent = fontMetrics.getAscent();
    monospaceWidth = fontMetrics.stringWidth(" ");
    x = (int)(pageFormat.getImageableX());
    y = (int)(pageFormat.getImageableY());
    linesPerPage = (int)(pageFormat.getImageableHeight() / fontHeight);
    headerSize = 4 + 1;

    // calculate the start line and count value for this page
    int startLine;
    int count;
    if (page == 0) {
      // start line
      startLine = minSelectionIndex;

      // count
      int countAvailable = maxSelectionIndex - minSelectionIndex + 1;
      int maxCount = linesPerPage - headerSize;
      count = (countAvailable > maxCount) ? maxCount : countAvailable;

    } else {
      // start line
      int pageOffset = linesPerPage * page - headerSize;
      startLine = pageOffset + minSelectionIndex;

      // count
      int countAvailable = maxSelectionIndex - startLine + 1;
      int maxCount = linesPerPage;
      count = (countAvailable > maxCount) ? maxCount : countAvailable;
    }

    // paint this page
    if (page == 0) {
      // if this is page 0 then print header information and the first lines of the image that fit
      // paint header
      paintHeader();

      // paint first page of image lines
      paintImageLines(startLine, count);

      // page 0 always exists
      return PAGE_EXISTS;
    } else {

      // only paint additional pages if they have content
      if (count > 0) {
        paintImageLines(startLine, count);
        return PAGE_EXISTS;
      } else {
        return NO_SUCH_PAGE;
      }
    }
  }

  private void paintImageLines(int startLine, int count) {
    // paint the formatted Image lines
    g.setFont(monoFont);
    for (int i=0; i<count; i++) {
      ImageLine imageLine = imageView.getLine(startLine + i);

      // draw the highlight background for each highlight index in the highlight array
      g.setColor(Color.YELLOW);
      for (int j=0; j< imageLine.numHighlights; j++) {
        int index = imageLine.highlightIndexes[j];
        g.fillRect(x + index * monospaceWidth, y, monospaceWidth, fontHeight);
      }

      // draw the line's text
      g.setColor(Color.BLACK);
      g.drawString(imageLine.text, x, y + ascent);

      // move to the next line
      y += fontHeight;
    }
  }

  private void paintHeader() {
    g.setColor(Color.BLACK);

    // NOTE: Graphics paint bug: A larger font is used when text includes ellipses ("...").
    g.setFont(textFont);

    // get header information
    String actualImageFile = FileTools.getAbsolutePath(imagePage.featureLine.actualImageFile);
    String featuresFile = imagePage.featureLine.featuresFile.getAbsolutePath();
    String forensicPath = ForensicPath.getPrintablePath(imagePage.featureLine.forensicPath, imageView.getUseHexPath());

    // paint header information
    paintPair("Image File", actualImageFile);
    paintPair("Feature File", featuresFile);
    paintPair("Forensic Path", forensicPath);

    // allow a space between header fields and image lines
    y += fontHeight;
  }

  private void paintPair(String key, String value) {
    // NOTE: Graphics paint bug: A default font is used when text includes ellipses ("...")
    // so don't specify any font.

    // paint pair and increment y
    g.drawString(key, x, y + ascent);
    g.drawString(value, x + monospaceWidth * 16, y + ascent);
    y += fontHeight;
  }
}


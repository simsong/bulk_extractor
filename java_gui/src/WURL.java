import java.awt.*;
import javax.swing.*;
import java.io.IOException;
import java.net.URL;

/**
 * A pop-up window for displaying a URL from a File path.
 */
public class WURL extends JDialog {
  private static final long serialVersionUID = 1;

  /**
   * The <code>WURL</code> class opens a window and displays the file at the given URL.
   * @param title The title of the window
   * @param url The URL to display as the contents of the window
   */
  public WURL(String title, URL url) {
//    super(BEViewer.getBEWindow(), "Bulk Extractor Viewer file " + url, false);
    super(BEViewer.getBEWindow(), title, false);
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    pane.setLayout(new GridBagLayout());

    // (0,0) JEditorPane
    GridBagConstraints c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // create the text area that holds the URL file's information
    JEditorPane editorPane;
    try {
      // create the pane and load the URL File
      editorPane = new JEditorPane(url);
    } catch (IOException e) {
      WLog.log("URL: " + url);
      throw new RuntimeException(e);
    }
    editorPane.setEditable(false);

    // put the text area in a scroll pane
    JScrollPane scrollPane= new JScrollPane(editorPane);
    scrollPane.setPreferredSize(new Dimension(800,600));

    // add the scroll pane containing the text area that holds the information
    pane.add(scrollPane, c);

    // pack the window
    pack();

    setLocationRelativeTo(BEViewer.getBEWindow());
    setVisible(true);
  }
}


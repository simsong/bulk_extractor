import java.awt.*;
import javax.swing.*;

/**
 * A pop-up window for displaying information about Bulk Extractor Viewer.
 */
public class WAbout extends JDialog {
  private static final long serialVersionUID = 1;

  private static WAbout wAbout;

  /**
   * Opens the pop-up window for displaying information about Bulk Extractor Viewer.
   */
  public static void openWindow() {
    // create the window if this is the first time
    if (wAbout == null) {
      wAbout = new WAbout();
    }

    // position and show the window
    wAbout.setLocationRelativeTo(BEViewer.getBEWindow());
    wAbout.setVisible(true);
  }

  // create the dialog window
  private WAbout() {
    super(BEViewer.getBEWindow(), "About Bulk Extractor Viewer", false);
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    pane.setLayout(new GridBagLayout());

    // (0,0) JTextArea
    GridBagConstraints c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // create the text area that holds the About information
    JTextArea textArea = new JTextArea();
    textArea.setEditable(false);
    textArea.setLineWrap(true);
    textArea.setWrapStyleWord(true);

    // set the text area's text
    textArea.setText("Bulk Extractor Viewer Version " + Config.VERSION
                 + "\n\n"
                 + "Please visit https://github.com/simsong/bulk_extractor/wiki/BEViewer."
                 + "\n\n"
    );

    // put the text area in a scroll pane
    JScrollPane scrollPane= new JScrollPane(textArea,
                 ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
                 ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
    scrollPane.setPreferredSize(new Dimension(500,200));

    // add the scroll pane containing the text area that holds the About information
    pane.add(scrollPane, c);

    // pack the window
    pack();
  }
}


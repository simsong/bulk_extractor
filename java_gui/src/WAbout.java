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

                 + "Bulk Extractor Viewer (BEViewer) is a User Interface for browsing "
                 + "features that have been extracted via the bulk_extractor feature "
                 + "extraction tool. BEViewer supports browsing multiple images and "
                 + "bookmarking and exporting features. BEViewer also provides a User "
                 + "Interface for launching bulk_extractor scans."
                 + "\n\n"
                 + "BEViewer downloads and documentation are available on github "
                 + "at https://github.com/simsong/bulk_extractor/wiki."
                 + "\n\n"
                 + "License:\n"
                 + "The software provided here is released by the Naval Postgraduate "
                 + "School, an agency of the U.S. Department of Navy.  The software "
                 + "bears no warranty, either expressed or implied. NPS does not assume "
                 + "legal liability nor responsibility for a User's use of the software "
                 + "or the results of such use. "
                 + "\n\n"
                 + "Please note that within the United States, copyright protection, "
                 + "under Section 105 of the United States Code, Title 17, is not "
                 + "available for any work of the United States Government and/or for "
                 + "any works created by United States Government employees. User "
                 + "acknowledges that this software contains work which was created by "
                 + "NPS government employees and is therefore in the public domain and "
                 + "not subject to copyright. "
                 + "\n\n"
                 + "Released into the public domain on June 1, 2011 by Bruce Allen."
    );

    // put the text area in a scroll pane
    JScrollPane scrollPane= new JScrollPane(textArea,
                 ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
                 ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
    scrollPane.setPreferredSize(new Dimension(600,400));

    // add the scroll pane containing the text area that holds the About information
    pane.add(scrollPane, c);

    // pack the window
    pack();
  }
}


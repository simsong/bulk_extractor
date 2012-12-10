import java.awt.*;
import javax.swing.*;

/**
 * A pop-up window for displaying information
 */
public class WInfo {
  private static JDialog window;
  private static JTextArea textArea;

  private WInfo() {
  }

  /**
   * Displays the text in the information window.
   * @param title the title of the information window
   * @param text the informational text to be displayed
   */
  public static void showInfo(String title, String text) {
    // create the JDialog window if it does not exist yet
    if (window == null) {
      // create the window component
//      window = new JDialog((Frame)null, "Bulk Extractor Viewer Runtime Log Report", false);
      window = new JDialog((Frame)null, null, false);
      Container pane = window.getContentPane();

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

      // create the text area that holds the Log information
      textArea = new JTextArea();
      textArea.setEditable(false);

      // put the text area in a scroll pane
      JScrollPane scrollPane= new JScrollPane(textArea,
                   ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
                   ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
      scrollPane.setPreferredSize(new Dimension(700,700));

      // add the scroll pane containing the text area that holds the information
      pane.add(scrollPane, c);

      // pack and position the window
      window.pack();
    }

    // update the title to indicate the current information title
    window.setTitle(title);

    // update the text area to contain the current content
    textArea.setText(text);

    // show the window
    // note: this should be delayed since it activates before setText, resulting in a visual flash.
    window.setLocationRelativeTo(BEViewer.getBEWindow());
    window.setVisible(true);
  }
}


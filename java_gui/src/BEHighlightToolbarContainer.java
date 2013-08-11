import java.awt.Dimension;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.Color;
import java.awt.Container;
import java.awt.Component;
import java.util.Observer;
import java.util.Observable;
import javax.swing.BoxLayout;
import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JToolBar;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.event.DocumentListener;
import javax.swing.event.DocumentEvent;

public class BEHighlightToolbar extends JToolBar {

//  private final JButton closeToolbarB = new JButton(BEIcons.CLOSE_24);
  private final JButton closeToolbarB = new JButton(BEIcons.CLOSE_16);
  private final JTextField highlightTF = new JTextField();
  private final JCheckBox matchCaseCB = new JCheckBox("Match case");

  // resources
  private final ModelChangedNotifier<Object> highlightToolbarChangedNotifier = new ModelChangedNotifier<Object>();

  private Color defaultHighlightTFBackgroundColor = null;

  /**
   * This notifies when visibility changes.
   */
  public void setVisible(boolean visible) {
    super.setVisible(visible);
    highlightToolbarChangedNotifier.fireModelChanged(null);
  }

  // this simply gets this interface to appear in javadocs
  public boolean isVisible() {
    return super.isVisible();
  }
 
  public BEHighlightToolbar() {
    super ("BEViewer Highlight Toolbar", JToolBar.HORIZONTAL);
    setFloatable(false);

    // close Toolbar
    closeToolbarB.setFocusable(false);
    closeToolbarB.setOpaque(false);
    closeToolbarB.setBorderPainted(false);
    closeToolbarB.setToolTipText("Close highlight toolbar");
    closeToolbarB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        setVisible(false);
      }
    });
    add(closeToolbarB);

    // separator
    addSeparator();

    // highlight control
    add(getHighlightControl());
  }

  private Container getHighlightControl() {
    Container container = new Container();
    container.setLayout(new BoxLayout(container, BoxLayout.X_AXIS));

    // ************************************************************
    // "Highlight:" label
    // ************************************************************
    container.add(new JLabel("Highlight:"));

    // separator
    container.add(Box.createRigidArea(new Dimension(BEViewer.GUI_X_PADDING, 0)));

    // ************************************************************
    // highlight input text field
    // ************************************************************
    defaultHighlightTFBackgroundColor = highlightTF.getBackground();

    // require a fixed size
    Dimension highlightDimension = new Dimension(220, highlightTF.getPreferredSize().height);
    highlightTF.setMinimumSize(highlightDimension);
    highlightTF.setPreferredSize(highlightDimension);
    highlightTF.setMaximumSize(highlightDimension);

    // create the JTextField highlightTF for containing the highlight text
    highlightTF.setToolTipText("Type Text to Highlight, type \"|\" between words to enter multiple Highlights");

    // change the user highlight model when highlightTF changes
    highlightTF.getDocument().addDocumentListener(new DocumentListener() {
      private void setHighlightString() {
        // and set the potentially escaped highlight text in the image model
        byte[] highlightBytes = highlightTF.getText().getBytes(UTF8Tools.UTF_8);
        BEViewer.userHighlightModel.setHighlightBytes(highlightBytes);

        // also make the background yellow while there is text in highlightTF
        if (highlightBytes.length > 0) {
          highlightTF.setBackground(Color.YELLOW);
        } else {
          highlightTF.setBackground(defaultHighlightTFBackgroundColor);
        }
      }
      // JTextField responds to Document change
      public void insertUpdate(DocumentEvent e) {
        setHighlightString();
      }
      public void removeUpdate(DocumentEvent e) {
        setHighlightString();
      }
      public void changedUpdate(DocumentEvent e) {
        setHighlightString();
      }
    });

    // add the highlight text textfield
    container.add(highlightTF);

    // separator
    container.add(Box.createRigidArea(new Dimension(10, 0)));

    // ************************************************************
    // Match Case checkbox
    // ************************************************************
    // set initial value
    matchCaseCB.setSelected(BEViewer.userHighlightModel.isHighlightMatchCase());

    // set up the highlight match case checkbox
    matchCaseCB.setFocusable(false);
    matchCaseCB.setOpaque(false); // this looks better with the ToolBar's gradient
    matchCaseCB.setRequestFocusEnabled(false);
    matchCaseCB.setToolTipText("Match case in highlight text");

    // match case action listener
    matchCaseCB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // toggle the case setting
        BEViewer.userHighlightModel.setHighlightMatchCase(matchCaseCB.isSelected());

        // as a convenience, change focus to highlightTF
        highlightTF.requestFocusInWindow();
      }
    });

    // wire listener to keep view in sync with the model
    BEViewer.userHighlightModel.addUserHighlightModelChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        // checkbox
        matchCaseCB.setSelected(BEViewer.userHighlightModel.isHighlightMatchCase());

        // text field, which can include user escape codes
        byte[] fieldBytes = highlightTF.getText().getBytes(UTF8Tools.UTF_8);
        byte[] modelBytes = BEViewer.userHighlightModel.getHighlightBytes();
        if (!UTF8Tools.bytesMatch(fieldBytes, modelBytes)) {
          highlightTF.setText(new String(modelBytes));
        }
      }
    });

    // add the match case button
    container.add(matchCaseCB);

    return container;
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param highlightToolbarChangedListener the <code>Observer</code> to be added
   */
  public void addHighlightToolbarChangedListener(Observer highlightToolbarChangedListener) {
    highlightToolbarChangedNotifier.addObserver(highlightToolbarChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param highlightToolbarChangedListener the <code>Observer</code> to be removed
   */
  public void removeHighlightToolbarChangedListener(Observer highlightToolbarChangedListener) {
    highlightToolbarChangedNotifier.deleteObserver(highlightToolbarChangedListener);
  }
}

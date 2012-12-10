import java.awt.Container;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.Component;
import java.io.IOException;
import javax.swing.*;
import javax.swing.event.*;
import java.util.Observable;
import java.util.Observer;
import javax.swing.JRadioButton;
import javax.swing.ButtonGroup;

/**
 * The <code>NavigationPane</code> class provides the navigation view
 * which includes navigation controls, selected feature information,
 * and the image dump listing.
 */
public final class NavigationPane extends Container {
  private static final int EDGE_PADDING = BEViewer.GUI_EDGE_PADDING;
  private static final int Y_PADDING = BEViewer.GUI_Y_PADDING;
  private static final long serialVersionUID = 1;
  private JButton bookmarkB;
  private JButton deleteB;
//  private JComboBox<FeatureLine> navigationComboBox;
  private JComboBox navigationComboBox;
  private FileComponent imageFileLabel;
  private FileComponent featuresFileLabel;
  private TextComponent featurePathLabel;
  private TextComponent featureLabel;
  private ImageComponent imageComponent;
  private JRadioButton textViewRB;
  private JRadioButton hexViewRB;
  private final ButtonGroup viewGroup = new ButtonGroup();
  private JButton reverseB;
  private JButton homeB;
  private JButton forwardB;

  private static final String NULL_LABEL = "<Not selected>";

  /**
   * Constructs navigation interfaces and wires actions.
   */
  public NavigationPane() {
    // build the UI
    setComponents();

    // wire actions
    wireActions();
  }

  public void wireActions() {

    // update views when the feature line selection manager changes
    BEViewer.featureLineSelectionManager.addFeatureLineSelectionManagerChangedListener(new Observer() {
      public void update(Observable o, Object arg) {

        // set fields based on the selected feature line
        final FeatureLine featureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();

        // set navigation view based on the FeatureLine
        if (featureLine != null
            && (featureLine.getType() == FeatureLine.FeatureLineType.ADDRESS_LINE
             || featureLine.getType() == FeatureLine.FeatureLineType.PATH_LINE)) {
 
          // set fields based on feature
          imageFileLabel.setFile(featureLine.getImageFile());
          featuresFileLabel.setFile(featureLine.getFeaturesFile());
          featurePathLabel.setComponentText(featureLine.getFormattedFirstField(
                                            BEViewer.imageView.getAddressFormat()));
          featureLabel.setComponentText(featureLine.getFormattedFeatureText());
          deleteB.setEnabled(true);
          bookmarkB.setEnabled(true);

        } else {

          // clear fields
          imageFileLabel.setFile(null);
          featuresFileLabel.setFile(null);
          featurePathLabel.setComponentText(null);
          featureLabel.setComponentText(null);
          deleteB.setEnabled(false);
          bookmarkB.setEnabled(false);
        }
      }
    });

    // update views when the image view changes
    BEViewer.imageView.addImageViewChangedListener(new Observer() {
      public void update(Observable o, Object arg) {
        // update UI based on type of change
        ImageView.ChangeType changeType = BEViewer.imageView.getChangeType();
        if (changeType == ImageView.ChangeType.IMAGE_PAGE_CHANGED) {
          // set enabled state of the image pan buttons
          setPanState(BEViewer.imageView.getImagePage());

        } else if (changeType == ImageView.ChangeType.LINE_FORMAT_CHANGED) {
          // set line format selection to match the image view
          ImageLine.LineFormat lineFormat = BEViewer.imageView.getLineFormat();
          if (lineFormat == ImageLine.LineFormat.TEXT_FORMAT) {
            textViewRB.setSelected(true);
          } else if (lineFormat == ImageLine.LineFormat.HEX_FORMAT) {
            hexViewRB.setSelected(true);
          } else {
            throw new RuntimeException("Invalid state");
          }

        } else if (changeType == ImageView.ChangeType.ADDRESS_FORMAT_CHANGED) {
          if (BEViewer.imageView.getImagePage() != null && BEViewer.imageView.getImagePage().featureLine != null) {
            FeatureLine featureLine = BEViewer.imageView.getImagePage().featureLine;
            featurePathLabel.setComponentText(
                   featureLine.getFormattedFirstField(BEViewer.imageView.getAddressFormat()));
          }
        } else {
          // no action for other change types
        }
      }
    });
  }

  private void setPanState(ImageModel.ImagePage imagePage) {
    // set forward, reverse, and home button state based on values from imageView's imagePage
    if (imagePage == null || imagePage.pageBytes.length == 0) {
      // there is no image to pan
      homeB.setEnabled(false);
      reverseB.setEnabled(false);
      forwardB.setEnabled(false);
    } else {
      long pageStartAddress = BEViewer.imageView.getImagePage().pageStartAddress;

      // set home button state
      homeB.setEnabled(true);

      // set reverse button state
      reverseB.setEnabled(imagePage.pageStartAddress > 0);

      // set forward button state
      forwardB.setEnabled(pageStartAddress + ImageModel.PAGE_SIZE < imagePage.imageSize);
    }
  }

  /**
   * Constructs navigation interfaces
   */
  private void setComponents() {
    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    setLayout(new GridBagLayout());
    int y = 0;

    // ************************************************************
    // (0,0) title "Navigation"
    // ************************************************************
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 5, 0, 5);
    c.insets = new Insets(EDGE_PADDING, EDGE_PADDING, 0, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the navigation title
    add(new JLabel("Navigation"), c);

    // ************************************************************
    // (0,1) Navigation control
    // ************************************************************
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 5, 0, 5);
    c.insets = new Insets(0, EDGE_PADDING, 0, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.weightx = 1;
    c.weighty = 0;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the navigation control container
    add(getNavigationControl(), c);

    // ************************************************************
    // (0,2) Feature line statistics
    // ************************************************************
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 5, 0, 5);
    c.insets = new Insets(0, EDGE_PADDING, Y_PADDING, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.weightx = 1;
    c.weighty = 0;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the Feature line statistics
    add(getFeatureStats(), c);

    // ************************************************************
    // (0,3) title "Image"
    // ************************************************************
    c = new GridBagConstraints();
    c.insets = new Insets(0, EDGE_PADDING, 0, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the image title
    add(new JLabel("Image"), c);

    // ************************************************************
    // (0,4) Image table
    // ************************************************************
    c = new GridBagConstraints();
    c.insets = new Insets(0, EDGE_PADDING, 0, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // add the image table
    add(getImageTable(), c);

    // ************************************************************
    // (0,5) Image table controls
    // ************************************************************
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 5, 0, 5);
    c.insets = new Insets(0, EDGE_PADDING, EDGE_PADDING, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.weightx = 1;
    c.weighty = 0;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the image table
    add(getImageTableControls(), c);
  }

  // ************************************************************
  // navigation control
  // ************************************************************
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  private Container getNavigationControl() {
    Container container = new Container();
    container.setLayout(new GridBagLayout());
    GridBagConstraints c;

    // (0,0) JButton <bookmark>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, BEViewer.GUI_X_PADDING);
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = 0;
    c.anchor = GridBagConstraints.LINE_START;

    // create bookmark button
    bookmarkB = new JButton(BEIcons.BOOKMARK_16);
    bookmarkB.setFocusable(false);
    bookmarkB.setRequestFocusEnabled(false);
    bookmarkB.setMinimumSize(BEViewer.BUTTON_SIZE);
    bookmarkB.setPreferredSize(BEViewer.BUTTON_SIZE);
    bookmarkB.setToolTipText("Bookmark this Feature");
    bookmarkB.setEnabled(false);

    // add the bookmark button
    container.add(bookmarkB, c);

    // clicking the bookmark button adds this feature entry to the bookmark list
    bookmarkB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.featureBookmarksModel.addBookmark(
                   BEViewer.featureLineSelectionManager.getFeatureLineSelection());
      }
    });

/*
    // (1,0) vertical separator
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 0;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.VERTICAL;
    container.add(new JSeparator(SwingConstants.VERTICAL), c);
*/

    // (2,0) JButton <delete>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, BEViewer.GUI_X_PADDING);
    c.gridx = 2;
    c.gridy = 0;
    c.weightx = 0;
    c.anchor = GridBagConstraints.LINE_START;

    // Delete Feature button
    deleteB = new JButton(BEIcons.DELETE_16);
    deleteB.setFocusable(false);
    deleteB.setRequestFocusEnabled(false);
    deleteB.setMinimumSize(BEViewer.BUTTON_SIZE);
    deleteB.setPreferredSize(BEViewer.BUTTON_SIZE);
    deleteB.setToolTipText("Remove this Feature");
    deleteB.setEnabled(false);

    // add the delete button
    container.add(deleteB, c);

    // clicking the delete button removes the feature from the feature list
    deleteB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        FeatureLine featureLine = (FeatureLine)BEViewer.featureNavigationComboBoxModel.getSelectedItem();

        // the button should be disabled when there is no selected feature line
        if (featureLine == null) {
          throw new NullPointerException();
        }

        // the selection should match the selection manager
        if (!featureLine.equals(BEViewer.featureLineSelectionManager.getFeatureLineSelection())) {
          throw new RuntimeException("invalid state");
        }

        // remove the current selection
        // NOTE: setSelectedItem fires clearing the selection in featureLineSelectionManager
        BEViewer.featureNavigationComboBoxModel.setSelectedItem(null);
        BEViewer.featureNavigationComboBoxModel.removeElement(featureLine);
      }
    });

    // (3,0) navigation comboBox
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 3;
    c.gridy = 0;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;

//    navigationComboBox = new JComboBox<FeatureLine>(BEViewer.featureNavigationComboBoxModel);
    navigationComboBox = new JComboBox(BEViewer.featureNavigationComboBoxModel);
    navigationComboBox.setFocusable(false);
//    navigationComboBox.requestFocusInWindow();  // as a user convenience, focus this item first.
    navigationComboBox.setRenderer(new FeatureListCellRenderer());

    // set the prototype value so Swing knows the height of JComboBox when it is empty
    navigationComboBox.setPrototypeDisplayValue(new FeatureLine());

    navigationComboBox.setMaximumRowCount(16);
    navigationComboBox.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        // The feature line selection manager changes the combo box selection.
        // The combo box selection changes the feature line selection manager
        // but only if the feature line selection manager had a different feature line.
        // So: no event loop.

        // determine equivalency, allowing null
        FeatureLine featureLine = (FeatureLine)navigationComboBox.getSelectedItem();
        BEViewer.featureLineSelectionManager.setFeatureLineSelection(featureLine);
      }
    });
    navigationComboBox.setToolTipText("Navigate to this Feature");

    container.add(navigationComboBox, c);

    return container;
  }

  // ************************************************************
  // feature statistics container
  // ************************************************************
  private Container getFeatureStats() {
    Container container = new Container();
    container.setLayout(new GridBagLayout());
    GridBagConstraints c;

    // (0,0) "Image File"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(new JLabel("Image File"), c);

    // (1,0) <image file>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;

    // create a file component for showing the selected image file
    imageFileLabel = new FileComponent();

    // add the image file label
    container.add(imageFileLabel, c);

    // (0,1) "Feature File"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(new JLabel("Feature File"), c);

    // (1,1) <feature file>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 1;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;

    // create a file component for showing the selected features file
    featuresFileLabel = new FileComponent();

    // add the features file label
    container.add(featuresFileLabel, c);

    // (0,2) Feature Path
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 2;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(new JLabel("Feature Path"), c);

    // (1,2) <feature path>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 2;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    featurePathLabel = new TextComponent();
    container.add(featurePathLabel, c);

    // (0,3) Feature text
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 3;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(new JLabel("Feature"), c);

    // (1,3) <feature text>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 3;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    featureLabel = new TextComponent();
    container.add(featureLabel, c);

    return container;
  }

  // ************************************************************
  // image table
  // ************************************************************
  private Component getImageTable() {

    // create ImageComponent imageComponent for showing media view image pages
    imageComponent = new ImageComponent(BEViewer.imageView, BEViewer.rangeSelectionManager);

    // create the scrollpane with the image component inside it
    JScrollPane imageViewScrollPane = new JScrollPane(imageComponent,
                 ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
                 ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
    imageViewScrollPane.setPreferredSize(new Dimension(550,600));

    return imageViewScrollPane;
  }

  // ************************************************************
  // image table controls
  // ************************************************************
  private Container getImageTableControls() {
    Container container = new Container();
    container.setLayout(new GridBagLayout());
    GridBagConstraints c;

    // (0,0) text radiobutton
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 0;

    // create the text view button for paging backward
    textViewRB = new JRadioButton("Text");
    textViewRB.setFocusable(false);
    textViewRB.setRequestFocusEnabled(false);
    textViewRB.setToolTipText("Text View");
    viewGroup.add(textViewRB);

    // add the text view button
    container.add(textViewRB, c);

    // clicking the text view button selects the "text" view
    textViewRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.imageView.setLineFormat(ImageLine.LineFormat.TEXT_FORMAT);
      }
    });

    // (1,0) hex radiobutton
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 30);
    c.gridx = 1;
    c.gridy = 0;

    // create the hex view button for paging backward
    hexViewRB = new JRadioButton("Hex");
    hexViewRB.setFocusable(false);
    hexViewRB.setRequestFocusEnabled(false);
    hexViewRB.setToolTipText("Hex View");
    viewGroup.add(hexViewRB);

    // add the hex view button
    container.add(hexViewRB, c);

    // clicking the hex view button selects the "hex" view
    hexViewRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        BEViewer.imageView.setLineFormat(ImageLine.LineFormat.HEX_FORMAT);
      }
    });

    // (2,0) JButton Reverse
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 0, 0, 0);
    c.insets = new Insets(0, 0, 0, BEViewer.GUI_X_PADDING);
    c.gridx = 2;
    c.gridy = 0;

    // create the Reverse button for paging backward
    reverseB = new JButton(BEIcons.REVERSE_16);
    reverseB.setFocusable(false);
    reverseB.setRequestFocusEnabled(false);
    reverseB.setMinimumSize(BEViewer.BUTTON_SIZE);
    reverseB.setPreferredSize(BEViewer.BUTTON_SIZE);
    reverseB.setToolTipText("Scroll Backward");
    reverseB.setEnabled(false);

    // add the Reverse button
    container.add(reverseB, c);

    // clicking the reverse button scrolls back one page
    reverseB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // get the exising page start address
        long pageStartAddress = BEViewer.imageView.getImagePage().pageStartAddress;

        // page back one page unless the new address is out of range
        if (pageStartAddress >= ImageModel.PAGE_SIZE) {
          pageStartAddress -= ImageModel.PAGE_SIZE;
          BEViewer.imageModel.setPageStartAddress(pageStartAddress);
        } else {
          WError.showError(
                      "Already at beginning of file.",
                      "BEViewer Image File address error", null);
        }
      }
    });

    // (3,0) JButton Home
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 0, 0, 0);
    c.insets = new Insets(0, 0, 0, BEViewer.GUI_X_PADDING);
    c.gridx = 3;
    c.gridy = 0;

    // create the home button for paging back to the page containing the selected address
    homeB = new JButton(BEIcons.HOME_16);
    homeB.setFocusable(false);
    homeB.setRequestFocusEnabled(false);
    homeB.setMinimumSize(BEViewer.BUTTON_SIZE);
    homeB.setPreferredSize(BEViewer.BUTTON_SIZE);
    homeB.setToolTipText("Return to Start Point");
    homeB.setEnabled(false);

    // add the Home button
    container.add(homeB, c);

    // clicking the Home button pages back to the page containing the selected address
    homeB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // return to the home page
        FeatureLine featureLine = BEViewer.imageView.getImagePage().featureLine;
        BEViewer.imageModel.setPageStartAddress(ImageModel.getAlignedAddress(featureLine));
      }
    });

    // (4,0) JButton Forward
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 4;
    c.gridy = 0;

    // create the Forward button for paging forward
    forwardB = new JButton(BEIcons.FORWARD_16);
    forwardB.setFocusable(false);
    forwardB.setRequestFocusEnabled(false);
    forwardB.setMinimumSize(BEViewer.BUTTON_SIZE);
    forwardB.setPreferredSize(BEViewer.BUTTON_SIZE);
    forwardB.setToolTipText("Scroll Forward");
    forwardB.setEnabled(false);

    // add the Forward button
    container.add(forwardB, c);

    // clicking the forward button scrolls forward one page
    forwardB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {

        // get the exising page start address
        long pageStartAddress = BEViewer.imageView.getImagePage().pageStartAddress;

        // get the file size
        long imageSize = BEViewer.imageView.getImagePage().imageSize;

        // page forward one page unless the new address is out of range
        if (pageStartAddress + ImageModel.PAGE_SIZE < imageSize) {
          pageStartAddress += ImageModel.PAGE_SIZE;
          BEViewer.imageModel.setPageStartAddress(pageStartAddress);
        } else {
          String sizeString = String.format(BEViewer.LONG_FORMAT, imageSize);
          WError.showError(
                      "Already at end of path of size " + sizeString + ".",
                      "BEViewer Image File address error", null);
        }
      }
    });
    return container;
  }
}


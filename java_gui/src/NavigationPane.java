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
  private FileComponent imageFileLabel;
  private FileComponent featuresFileLabel;
  private TextComponent forensicPathLabel;
  private TextComponent featureLabel;
  private ImageComponent imageComponent;
  private JRadioButton textViewRB;
  private JRadioButton hexViewRB;
  private final ButtonGroup viewGroup = new ButtonGroup();
  private JButton reverseB;
  private JButton homeB;
  private JButton forwardB;

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

        // get the selected feature line
        final FeatureLine featureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();

        // set navigation text and button state based on the FeatureLine
        if (!featureLine.isBlank() && !ForensicPath.isHistogram(featureLine.firstField)) {
 
          // set navigation fields
          imageFileLabel.setFile(featureLine.actualImageFile);
          featuresFileLabel.setFile(featureLine.featuresFile);
          forensicPathLabel.setComponentText(ForensicPath.getPrintablePath(featureLine.forensicPath, BEViewer.imageView.getUseHexPath()));
          featureLabel.setComponentText(featureLine.formattedFeature);

        } else {

          // clear navigation fields
          imageFileLabel.setFile(null);
          featuresFileLabel.setFile(null);
          forensicPathLabel.setComponentText(null);
          featureLabel.setComponentText(null);
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

        } else if (changeType == ImageView.ChangeType.FORENSIC_PATH_NUMERIC_BASE_CHANGED) {
          if (BEViewer.imageView.getImagePage() != null) {
            ImageModel.ImagePage imagePage = BEViewer.imageView.getImagePage();
            forensicPathLabel.setComponentText(ForensicPath.getPrintablePath(imagePage.featureLine.forensicPath, BEViewer.imageView.getUseHexPath()));
          }

        } else {
          // no action for other change types
        }
      }
    });
  }

  private void setPanState(ImageModel.ImagePage imagePage) {
    // set forward, reverse, and home button state based on the forensic path of the active page
    if (imagePage.pageBytes.length == 0) {
      // there is no image to pan
      homeB.setEnabled(false);
      reverseB.setEnabled(false);
      forwardB.setEnabled(false);
    } else {
      long offset = ForensicPath.getOffset(imagePage.pageForensicPath);

      // set home button state
      homeB.setEnabled(true);

      // set reverse button state
      reverseB.setEnabled(offset > 0);

      // set forward button state
      forwardB.setEnabled(offset + ImageModel.PAGE_SIZE < imagePage.imageSize);
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
    // (0,y++) Feature line statistics
    // ************************************************************
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 5, 0, 5);
    c.insets = new Insets(EDGE_PADDING, EDGE_PADDING, Y_PADDING, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.weightx = 1;
    c.weighty = 0;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the Feature line statistics
    add(getFeatureStats(), c);

    // ************************************************************
    // (0,y++) title "Image"
    // ************************************************************
    c = new GridBagConstraints();
    c.insets = new Insets(0, EDGE_PADDING, 0, EDGE_PADDING);
    c.gridx = 0;
    c.gridy = y++;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the image title
    add(new JLabel("Image"), c);

    // ************************************************************
    // (0,y++) Image table
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
    // (0,y++) Image table controls
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

    // (0,2) Forensic Path
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 2;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(new JLabel("Forensic Path"), c);

    // (1,2) <forensic path>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 2;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    forensicPathLabel = new TextComponent();
    container.add(forensicPathLabel, c);

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
        // get the exising page offset
        ImageModel.ImagePage imagePage = BEViewer.imageView.getImagePage();
        long offset = ForensicPath.getOffset(imagePage.pageForensicPath);

        // page back one page unless the new forensic path is out of range
        if (offset >= ImageModel.PAGE_SIZE) {
          offset -= ImageModel.PAGE_SIZE;
          BEViewer.imageModel.setImageSelection(ForensicPath.getAdjustedPath(imagePage.pageForensicPath, offset));
        } else {
          WError.showError(
                      "Already at beginning of file.",
                      "BEViewer Image File pan error", null);
        }
      }
    });

    // (3,0) JButton Home
    c = new GridBagConstraints();
//    c.insets = new Insets(0, 0, 0, 0);
    c.insets = new Insets(0, 0, 0, BEViewer.GUI_X_PADDING);
    c.gridx = 3;
    c.gridy = 0;

    // create the home button for paging back to the page containing the selected forensic path
    homeB = new JButton(BEIcons.HOME_16);
    homeB.setFocusable(false);
    homeB.setRequestFocusEnabled(false);
    homeB.setMinimumSize(BEViewer.BUTTON_SIZE);
    homeB.setPreferredSize(BEViewer.BUTTON_SIZE);
    homeB.setToolTipText("Return to Start Point");
    homeB.setEnabled(false);

    // add the Home button
    container.add(homeB, c);

    // clicking the Home button pages back to the page containing the selected forensic path
    homeB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // return to the home page
        ImageModel.ImagePage imagePage = BEViewer.imageView.getImagePage();
        BEViewer.imageModel.setImageSelection(ForensicPath.getAlignedPath(imagePage.featureLine.forensicPath));
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

        // get the exising page offset
        ImageModel.ImagePage imagePage = BEViewer.imageView.getImagePage();
        long offset = ForensicPath.getOffset(imagePage.pageForensicPath);

        // get the image size
        long imageSize = imagePage.imageSize;

        // page forward one page unless the new offset is out of range
        if (offset + ImageModel.PAGE_SIZE < imagePage.imageSize) {
          offset += ImageModel.PAGE_SIZE;
          BEViewer.imageModel.setImageSelection(ForensicPath.getAdjustedPath(imagePage.pageForensicPath, offset));
        } else {
          WError.showError(
                      "Already at end of path, " + imageSize + ".",
                      "BEViewer Image File pan error", null);
        }
      }
    });
    return container;
  }
}


import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.io.File;
import javax.swing.*;

/**
 * Manage Required parameter values.
 */

public class WScanBoxedRequired {

  public final Component component;

  private final JRadioButton imageFileChooserRB = new JRadioButton("Image File");
  private final JRadioButton rawDeviceChooserRB = new JRadioButton("Raw Device");
  private final JRadioButton directoryOfFilesChooserRB = new JRadioButton("Directory of Files");
  private final ButtonGroup imageChooserGroup = new ButtonGroup();

  private final JLabel inputImageL = new JLabel();
  private final JTextField inputImageTF = new JTextField();
  private final JButton inputImageChooserB = new JButton("\u2026");	// ...

  private final JTextField outdirTF = new JTextField();
  private final JButton outdirChooserB = new JButton("\u2026");	// ...

  public WScanBoxedRequired() {
    component = buildContainer();
    wireActions();
  }

  private Component buildContainer() {
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();
    container.setBorder(BorderFactory.createTitledBorder("Required Parameters"));

    container.setLayout(new GridBagLayout());

    int y = 0;
    // scan type
    GridBagConstraints c;
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = y++;
    c.gridwidth= 3;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(buildScanTypeContainer(), c);

    addFileLine(container, y++, inputImageL, inputImageTF, inputImageChooserB);
    inputImageChooserB.setToolTipText("Select the image path");
    addFileLine(container, y++, new JLabel("Output Feature Directory"), outdirTF, outdirChooserB);
    outdirChooserB.setToolTipText("Select the Output Feature Directory");

    // group the JRadioButtons together
    imageChooserGroup.add(imageFileChooserRB);
    imageChooserGroup.add(rawDeviceChooserRB);
    imageChooserGroup.add(directoryOfFilesChooserRB);

    return container;
  }

  private Component buildScanTypeContainer() {
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();

    container.setLayout(new GridBagLayout());
    GridBagConstraints c;

    // "Scan:" label
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor  = GridBagConstraints.LINE_START;
    container.add(new JLabel("Scan:"), c);

    // Image File radio button
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 1;
    c.gridy = 0;
    c.anchor  = GridBagConstraints.LINE_START;
    container.add(imageFileChooserRB, c);

    // raw device radio button
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 2;
    c.gridy = 0;
    c.anchor  = GridBagConstraints.LINE_START;
    if (BEViewer.isWindows || BEViewer.isMac) {  // sorry, can't select raw device
      rawDeviceChooserRB.setEnabled(false);
      rawDeviceChooserRB.setToolTipText("(not available on Windows or Mac)");
    }
    container.add(rawDeviceChooserRB, c);

    // Directory of files radio button
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 3;
    c.gridy = 0;
    c.anchor  = GridBagConstraints.LINE_START;
    container.add(directoryOfFilesChooserRB, c);

    return container;
  }

  public void setScanSettings(ScanSettings scanSettings) {
    setImageSourceType(scanSettings.imageSourceType);
    inputImageTF.setText(scanSettings.inputImage);
    outdirTF.setText(scanSettings.outdir);
  }

  public void getScanSettings(ScanSettings scanSettings) {
    if (imageFileChooserRB.isSelected()) {
      scanSettings.imageSourceType = ImageSourceType.IMAGE_FILE;
    } else if (rawDeviceChooserRB.isSelected()) {
      scanSettings.imageSourceType = ImageSourceType.RAW_DEVICE;
    } else if (directoryOfFilesChooserRB.isSelected()) {
      scanSettings.imageSourceType = ImageSourceType.DIRECTORY_OF_FILES;
    } else {
      throw new RuntimeException("bad setting");
    }
    scanSettings.inputImage = inputImageTF.getText();
    scanSettings.outdir = outdirTF.getText();
  }

  private void setImageSourceType(ImageSourceType imageSourceType) {
    // set usage variable and UI text
    inputImageL.setText(imageSourceType.toString());
    inputImageTF.setText("");
    if (imageSourceType == ImageSourceType.IMAGE_FILE) {
      imageFileChooserRB.setSelected(true);
      inputImageChooserB.setToolTipText("select the Image File to scan");
    } else if (imageSourceType == ImageSourceType.RAW_DEVICE) {
      rawDeviceChooserRB.setSelected(true);
      inputImageChooserB.setToolTipText("select the Raw Device to scan");
    } else if (imageSourceType == ImageSourceType.DIRECTORY_OF_FILES) {
      directoryOfFilesChooserRB.setSelected(true);
      inputImageChooserB.setToolTipText("select the directory of files to scan");
    } else {
      throw new RuntimeException("bad image source");
    }
  }

  private void wireActions() {

    // on image file radio button selection set for selecting default image file
    imageFileChooserRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // set to use image file as source
        setImageSourceType(ImageSourceType.IMAGE_FILE);
      }
    });

    // on image file radio button selection set for selecting default image file
    rawDeviceChooserRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // set to use image file as source
        setImageSourceType(ImageSourceType.RAW_DEVICE);
      }
    });

    // on file directory radio button selection set for selecting file directory
    directoryOfFilesChooserRB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // set to use file directory as source
        setImageSourceType(ImageSourceType.DIRECTORY_OF_FILES);
      }
    });
 
    // input image source, one of IMAGE_FILE, DIRECTORY_OF_FILES, or RAW_DEVICE
    inputImageChooserB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        // open a chooser based on the selected type
        if (imageFileChooserRB.isSelected()) { // IMAGE_FILE
          // set up image chooser
          JFileChooser imageFileChooser = new JFileChooser();
          imageFileChooser.setDialogTitle("Image File to Extract Features From");
          imageFileChooser.setDialogType(JFileChooser.OPEN_DIALOG);
          imageFileChooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
          imageFileChooser.setSelectedFile(new File(inputImageTF.getText()));
          imageFileChooser.setFileFilter(ImageFileType.imageFileFilter);

          // choose the image
          // if the user selects APPROVE then take the text
          if (imageFileChooser.showOpenDialog(WScan.getWScanWindow()) == JFileChooser.APPROVE_OPTION) {
            // put the text in the text field
            String pathString = imageFileChooser.getSelectedFile().getAbsolutePath();
            inputImageTF.setText(pathString);
          }

        } else if (directoryOfFilesChooserRB.isSelected()) { // DIRECTORY_OF_FILES
          // set up input directory chooser
          JFileChooser imageFileChooser = new JFileChooser();
          imageFileChooser.setDialogTitle("Directory of Files to Recursively Extract Features From");
          imageFileChooser.setDialogType(JFileChooser.OPEN_DIALOG);
          imageFileChooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
          imageFileChooser.setSelectedFile(new File(inputImageTF.getText()));

          // choose the directory
          // if the user selects APPROVE then take the text
          if (imageFileChooser.showOpenDialog(WScan.getWScanWindow()) == JFileChooser.APPROVE_OPTION) {
            // put the text in the text field
            String pathString = imageFileChooser.getSelectedFile().getAbsolutePath();
            inputImageTF.setText(pathString);
          }
        } else if (rawDeviceChooserRB.isSelected()) { // RAW_DEVICE
          WRawDeviceChooser.openWindow();
          // continue here on closure
          String selection = WRawDeviceChooser.getSelection();
          if (selection != null) {
            // put the text in the text field
            inputImageTF.setText(selection);
          }
        }
      }
    });

    // output feature directory
    outdirChooserB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {

/* via awt.FileDialog fails too
        FileDialog fileDialog = new FileDialog((Dialog)WScan.getWScanWindow(), "Output Feature Directory", FileDialog.SAVE);
        fileDialog.setDirectory(outdirTF.getText());
        fileDialog.setVisible(true);
        outdirTF.setText(fileDialog.getFile());
*/

        // open directory chooser for selecting the output feature directory
        // The output feature is typically new and is to be created using
        // the file chooser's "create new directory" button.

        // set up image chooser
        JFileChooser outputFeatureDirectoryChooser = new JFileChooser();
        outputFeatureDirectoryChooser.setDialogTitle("Output Feature Directory");

        // we need OPEN_DIALOG but we want the "create new directory" button,
        // so we use SAVE_DIALOG and hack it to say "open".
        outputFeatureDirectoryChooser.setDialogType(JFileChooser.SAVE_DIALOG);
//        outputFeatureDirectoryChooser.setApproveButtonText("Select");

        outputFeatureDirectoryChooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
        outputFeatureDirectoryChooser.setSelectedFile(new File(outdirTF.getText()));
System.setProperty("apple.awt.fileDialogForDirectories", "true");

        // choose the image
        // if the user selects it then take the text
        if (outputFeatureDirectoryChooser.showDialog(WScan.getWScanWindow(), "Select") == JFileChooser.APPROVE_OPTION) {
          // put the text in the text field
          String pathString = outputFeatureDirectoryChooser.getSelectedFile().getAbsolutePath();
          outdirTF.setText(pathString);
        }
      }
    });
  }

  // add file line
  private void addFileLine(Container container, int y,
             JComponent fileComponent, JTextField textField, JButton button) {
    GridBagConstraints c;

    // file radio button(0,y)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(fileComponent, c);

    // file textField (1,y)
    textField.setMinimumSize(new Dimension(WScan.EXTRA_WIDE_FIELD_WIDTH, textField.getPreferredSize().height));
    textField.setPreferredSize(new Dimension(WScan.EXTRA_WIDE_FIELD_WIDTH, textField.getPreferredSize().height));
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = y;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(textField, c);

    // file chooser (2,y)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 2;
    c.gridy = y;
    container.add(button, c);
  }
}


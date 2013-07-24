import java.awt.*;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.*;
import javax.swing.filechooser.FileFilter;  // not java.io.FileFilter
import java.io.File;

/**
 * This class provides a button that displays "...", has a tooltip
 * and responds to selection by activating a file chooser
 * and loading the file chooser's selection to a destination text field.
 */

public class FileChooserButton extends JButton implements ActionListener {
  private final Component parent;
  private final String toolTip;
  private final JTextField textField;

  private final int dialogType;
  private final int fileSelectionMode;
  private final FileFilter preferredFileFilter;

  // provide visibility so listeners can register
  public JFileChooser chooser = new JFileChooser();

  // read/write file/directory/image
  public static final int READ_FILE = 0;
  public static final int READ_DIRECTORY = 1;
  public static final int WRITE_FILE = 2;
  public static final int WRITE_DIRECTORY = 3;
  public static final int READ_IMAGE_FILE = 4;
  public static final int READ_REPORT_FILE = 5;

  public FileChooserButton(Component parent, String toolTip, int mode, JTextField destTextField) {
    super("\u2026");	// ...
    setToolTipText(toolTip);
    addActionListener(this);

    this.parent = parent;
    this.toolTip = toolTip;
    this.textField = destTextField;

    // set properties for chooser based on mode type
    if (mode == READ_FILE) {
      dialogType = JFileChooser.OPEN_DIALOG;
      fileSelectionMode = JFileChooser.FILES_ONLY;
      preferredFileFilter = null;
    } else if (mode == READ_DIRECTORY) {
      dialogType = JFileChooser.OPEN_DIALOG;
      fileSelectionMode = JFileChooser.DIRECTORIES_ONLY;
      preferredFileFilter = null;
    } else if (mode == WRITE_FILE) {
      dialogType = JFileChooser.SAVE_DIALOG;
      fileSelectionMode = JFileChooser.FILES_AND_DIRECTORIES;
      preferredFileFilter = null;
    } else if (mode == WRITE_DIRECTORY) {
      dialogType = JFileChooser.SAVE_DIALOG;
      fileSelectionMode = JFileChooser.FILES_AND_DIRECTORIES;
      preferredFileFilter = null;
    } else if (mode == READ_IMAGE_FILE) {
      dialogType = JFileChooser.OPEN_DIALOG;
      fileSelectionMode = JFileChooser.FILES_ONLY;
      preferredFileFilter = ImageFileType.imageFileFilter;
    } else if (mode == READ_REPORT_FILE) {
      dialogType = JFileChooser.OPEN_DIALOG;
      fileSelectionMode = JFileChooser.FILES_ONLY;
      preferredFileFilter = new ReportFileFilter();
    } else {
      throw new RuntimeException("Invalid mode");
    }
  }

  public void actionPerformed(ActionEvent e) {
    chooser.setDialogTitle(toolTip);
    chooser.setDialogType(dialogType);
    chooser.setFileSelectionMode(fileSelectionMode);
    chooser.setSelectedFile(new File(textField.getText()));
    chooser.setFileFilter(preferredFileFilter);

    // if the user selects APPROVE then take the text
    if (chooser.showOpenDialog(parent) == JFileChooser.APPROVE_OPTION) {
      // put the text in the text field
      String pathString = chooser.getSelectedFile().getAbsolutePath();
      textField.setText(pathString);
    }
  }
}


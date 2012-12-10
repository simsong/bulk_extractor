import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.io.File;
import java.io.IOException;

/**
 * The dialog window for selecting an output directory by selecting the parent directory path
 * and the new directory name.
 */

public class WSelectOutputDirectory extends JDialog {
  private static final long serialVersionUID = 1;

  private final Component parentWindow;

  // indicate selected
  private boolean isSelected;

  // create UI parts
  private final JTextField parentDirectoryTF;
  private final FileChooserButton parentDirectoryChooserB;
  private final JTextField newDirectoryNameTF;
  private final JButton selectB;
  private final JButton cancelB;

  // reset defaults
  public void setDefaultValues() {
    parentDirectoryTF.setText("");
    newDirectoryNameTF.setText("");
  }

  public String getOutputDirectory() {
    // set selected output directory value
    File parentDirectory = new File(parentDirectoryTF.getText());
    String newDirectoryName = newDirectoryNameTF.getText();
    File outputDirectory = new File(parentDirectory, newDirectoryName);
    return outputDirectory.getAbsolutePath();
  }

  /**
   * Opens this window.
   */
  public boolean showSelectionDialog() {
    isSelected = false;

    // set window attributes
    setLocationRelativeTo(parentWindow);

    // show the dialog window
    setVisible(true);

    // being modal, flow suspends until visibility is false, so return composite file now
    return isSelected;
  }

  public WSelectOutputDirectory(Component parentWindow, String title) {

    parentDirectoryTF = new JTextField();
    parentDirectoryChooserB = new FileChooserButton(this, "Select Directory to Contain the New Directory", FileChooserButton.READ_DIRECTORY, parentDirectoryTF);
    newDirectoryNameTF = new JTextField();
    selectB = new JButton("Select");
    cancelB = new JButton("Cancel");

    // build this window
    this.parentWindow = parentWindow;
    setTitle(title);
    buildInterface();
    wireActions();
    getRootPane().setDefaultButton(selectB);
    pack();
    setModal(true);
  }

  private void buildInterface() {
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,2) add the directory input
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildDirectoryPath(), c);

    // (0,3) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 3;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildControls(), c);
  }

  private Component buildDirectoryPath() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // parent directory path label (0,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Directory to Contain the New Directory"), c);

    // parentDirectoryTF (1,0)
    parentDirectoryTF.setMinimumSize(new Dimension(250, parentDirectoryTF.getPreferredSize().height));
    parentDirectoryTF.setPreferredSize(new Dimension(250, parentDirectoryTF.getPreferredSize().height));
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(parentDirectoryTF, c);

    // parent directory chooser (2,0)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 2;
    c.gridy = 0;
    container.add(parentDirectoryChooserB, c);

    // new directory name label (0,1)
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("New Directory Name"), c);

    // newDirectoryNameTF (1,1)
    newDirectoryNameTF.setMinimumSize(new Dimension(250, newDirectoryNameTF.getPreferredSize().height));
    newDirectoryNameTF.setPreferredSize(new Dimension(250, newDirectoryNameTF.getPreferredSize().height));
    c = new GridBagConstraints();
    c.insets = new Insets(0, 5, 0, 5);
    c.gridx = 1;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(newDirectoryNameTF, c);

    return container;
  }

  private Component buildControls() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) selectB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 0;
    container.add(selectB, c);

    // (1,0) cancelB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 1;
    c.gridy = 0;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(cancelB, c);

    return container;
  }

  private void wireActions() {

    // clicking selectB indicates isSelected and closes this window
    selectB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        // set selected
        isSelected = true;
        setVisible(false);
      };
    });

    // clicking cancelB closes this window
    cancelB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setVisible(false);
      }
    });
  }
}


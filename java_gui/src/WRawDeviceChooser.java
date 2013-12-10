import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.*;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.ListSelectionEvent;
import java.io.File;
import java.io.FileFilter;

/**
 * The dialog window for choosing a raw linux device under /dev
 */

public class WRawDeviceChooser extends JDialog {
  private static final long serialVersionUID = 1;

  // resources
//  private static final DefaultListModel<String> listModel = new DefaultListModel<String>();
  private static final DefaultListModel listModel = new DefaultListModel();
  private static final FileFilter fileFilter = new RawDeviceFileFilter();
  private static final File DEV_FILE = new File("/dev");

  private static WRawDeviceChooser wRawDeviceChooser;
//  private static JList rawDeviceL = new JList<String>(listModel);
@SuppressWarnings("unchecked") // hacked until we don't require javac6
  private static JList rawDeviceL = new JList(listModel);
  private static JButton selectB = new JButton("Select");
  private static JButton cancelB = new JButton("Cancel");
  private static JButton refreshB = new JButton("Refresh");

  public static String getSelection() {
    return (String)rawDeviceL.getSelectedValue();
  }

@SuppressWarnings("unchecked") // hacked until we don't require javac6
  private void refreshList() {
    // reload list
    // NOTE: we may wish to replace this with more code in order to provide device size and SN.
    // For SN, exec "hdparm -i /dev/sdb", repeating for each sd*.
    // For size, exec or use binary search of reading one byte as is done in RawFileReader.
    listModel.clear();
    File[] files = DEV_FILE.listFiles(fileFilter);
    if (files == null) {
      files = new File[0];
    }
    for (int i=0; i<files.length; i++) {
      listModel.addElement(files[i].getAbsolutePath());
    }

    if (files.length == 0) {
      // warn if no devices
      WError.showMessage("There are no devices at /dev/sd*.",
                         "Raw device list read failure");
    }
  }

  private static class RawDeviceFileFilter implements FileFilter {
    // accept filenames for valid image files
    /**
     * Whether the given file is accepted by this filter.
     * @param file The file to check
     * @return true if accepted, false if not
     */
    public boolean accept(File file) {
      return (file.getAbsolutePath().startsWith("/dev/sd"));
    }

    /**
     * The description of this filter.
     * @return the description of this filter
     */
    public String getDescription() {
      return "Raw Devices (/dev/sd*)";
    }
  }

/**
 * Opens this window.
 */
  public static void openWindow() {
    if (wRawDeviceChooser == null) {
      // this is the first invocation
      // create the window
      wRawDeviceChooser = new WRawDeviceChooser();
    }

    // show the dialog window
    wRawDeviceChooser.setLocationRelativeTo(BEViewer.getBEWindow());
    wRawDeviceChooser.refreshList();
    wRawDeviceChooser.setVisible(true);
  }

  private static void closeWindow() {
    // the window is not instantiated in test mode
    if (wRawDeviceChooser != null) {
      WRawDeviceChooser.wRawDeviceChooser.setVisible(false);
    }
  }

  private WRawDeviceChooser() {
    // set parent window, title, and modality

    buildInterface();
    wireActions();
    getRootPane().setDefaultButton(selectB);
    pack();
  }

  private void buildInterface() {
    setTitle("Select Raw Device");
    setModal(true);
    setAlwaysOnTop(true);
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    // (0,0) "Raw Devices (/dev)"
    c = new GridBagConstraints();
    c.insets = new Insets(10, 10, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.BOTH;
    pane.add(new JLabel("Raw Devices (/dev)"), c);

    // (0,1) scroll pane containing the raw device table
    c = new GridBagConstraints();
    c.insets = new Insets(0, 10, 10, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // put the raw device list in a scroll pane
    JScrollPane scrollPane = new JScrollPane(rawDeviceL);
    scrollPane.setPreferredSize(new Dimension(500, 300));

     // add the scroll pane containing the raw device list
    pane.add(scrollPane, c);

    // (0,2) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(buildControls(), c);
  }

  private Component buildControls() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,1)selectB 
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 1;
    selectB.setEnabled(false);
    container.add(selectB, c);

    // (1,1) cancelB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 1;
    c.gridy = 1;
    container.add(cancelB, c);

    // (2,1) vertical separator
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 2;
    c.gridy = 1;
    container.add(new JSeparator(SwingConstants.VERTICAL), c);

    // (3,1) refreshB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 3;
    c.gridy = 1;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(refreshB, c);

    return container;
  }

  private void wireActions() {
    // rawDeviceL selection change changes selectB enabled state
    rawDeviceL.addListSelectionListener(new ListSelectionListener() {
      public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting() == false) {
          selectB.setEnabled(rawDeviceL.getSelectedValue() != null);
        }
      }
    });

    // select
    selectB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setVisible(false);
      }
    });

    // cancel
    cancelB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        rawDeviceL.setSelectedValue(null, false);
        setVisible(false);
      }
    });

    // refresh
    refreshB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        rawDeviceL.setSelectedValue(null, false);
        refreshList();
      }
    });
  }
}


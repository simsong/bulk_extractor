import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.event.*;
import java.io.File;
import java.io.IOException;
import java.util.Observable;
import java.util.Observer;

/**
 * The dialog window for managing the bulk_extractor run queue scheduler
 */

@SuppressWarnings("unchecked") // hacked until we don't require javac6
public class WScanSettingsRunQueue extends JDialog {
  private static final long serialVersionUID = 1;

  private static WScanSettingsRunQueue wScanSettingsRunQueue;
  private static JList runQueueL = new JList(BEViewer.scanSettingsListModel);
  private static ScanSettingsToolBar toolbar = new ScanSettingsToolBar(runQueueL);
  private static JButton closeB = new JButton("Close");

  static {
    runQueueL.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    runQueueL.setCellRenderer(new ScanSettingsListCellRenderer());
  }

  /**
   * Opens this window.
   */
  public static void openWindow() {
    if (wScanSettingsRunQueue == null) {
      // this is the first invocation
      // create the window
      wScanSettingsRunQueue = new WScanSettingsRunQueue();
    }

    // show the dialog window
    wScanSettingsRunQueue.setLocationRelativeTo(BEViewer.getBEWindow());
    wScanSettingsRunQueue.setVisible(true);
  }

  private static void closeWindow() {
    WScanSettingsRunQueue.wScanSettingsRunQueue.setVisible(false);
  }

  private WScanSettingsRunQueue() {
    // set parent window, title, and modality

    buildInterface();
    wireActions();
    getRootPane().setDefaultButton(closeB);
    pack();
  }

  private void buildInterface() {
    setTitle("bulk_extractor Run Queue");
    Container pane = getContentPane();

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    pane.setLayout(new GridBagLayout());

    int y=0;
    // toolbar
    c = new GridBagConstraints();
    c.gridx = 0;
    c.gridy = y;
//    c.weightx = 1;
//    c.weighty = 1;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.BOTH;
    pane.add(toolbar, c);

    // "Run Queue" label
    y++;
    c = new GridBagConstraints();
    c.insets = new Insets(10, 10, 0, 10);
    c.gridx = 0;
    c.gridy = y;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.BOTH;
    pane.add(new JLabel("Run Queue"), c);

    // scroll pane containing the Scan Settings jobs
    y++;
    c = new GridBagConstraints();
    c.insets = new Insets(0, 10, 10, 10);
    c.gridx = 0;
    c.gridy = y;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // put the Scan Settings job list in the scroll pane
    JScrollPane scrollPane = new JScrollPane(runQueueL);
    scrollPane.setPreferredSize(new Dimension(600, 400));

     // add the scroll pane containing the Scan Settings job list
    pane.add(scrollPane, c);

    // add controls
    y++;
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = y;
    pane.add(buildControls(), c);
  }

  private Component buildControls() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    int x=0;
    int y=0;

    // closeB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
//    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(closeB, c);

    return container;
  }

  private void wireActions() {

    // clicking closeB closes this window
    closeB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setVisible(false);
      }
    });
  }

/*
  private void wireActions() {
    // JList selection state changes toolbar states
    runQueueL.addListSelectionListener(new ListSelectionListener() {
      public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting() == false) {
          toolbar.setEnabledStates();
        }
      }
    });

    // changes to list data can change the list selection model
    BEViewer.scanSettingsListModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        toolbar.setEnabledStates();
      }
      public void intervalAdded(ListDataEvent e) {
        // do this later on the swing queue because we already are on the
        // swing queue and the ListModel isn't stable until later.
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            runQueueL.setSelectedValue(
                     BEViewer.scanSettingsListModel.getElementAt(
                     BEViewer.scanSettingsListModel.getSize() - 1)
                     , true);
          }
        });
      }
      public void intervalRemoved(ListDataEvent e) {
        toolbar.setEnabledStates();
      }
    });
  }
*/
}


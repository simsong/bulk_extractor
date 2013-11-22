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
  private static JButton deleteB = new JButton(BEIcons.DELETE_16);
  private static JButton editRedoB = new JButton(BEIcons.EDIT_REDO_16);
  private static JButton upB = new JButton(BEIcons.UP_16);
  private static JButton downB = new JButton(BEIcons.DOWN_16);
  private static JButton closeB = new JButton("Close");

  static {
    runQueueL.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    runQueueL.setCellRenderer(new ScanSettingsListCellRenderer());
    deleteB.setToolTipText("Delete selection from queue");
    deleteB.setMinimumSize(BEViewer.BUTTON_SIZE);
    deleteB.setPreferredSize(BEViewer.BUTTON_SIZE);
    editRedoB.setToolTipText("Edit selection");
    editRedoB.setMinimumSize(BEViewer.BUTTON_SIZE);
    editRedoB.setPreferredSize(BEViewer.BUTTON_SIZE);
    upB.setToolTipText("Move selection up queue to be run sooner");
    upB.setMinimumSize(BEViewer.BUTTON_SIZE);
    upB.setPreferredSize(BEViewer.BUTTON_SIZE);
    downB.setToolTipText("Move selection down queue to be run later");
    downB.setMinimumSize(BEViewer.BUTTON_SIZE);
    downB.setPreferredSize(BEViewer.BUTTON_SIZE);
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
    setButtonStates();
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

    // (0,0) "Run Queue" label
    c = new GridBagConstraints();
    c.insets = new Insets(10, 10, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    c.fill = GridBagConstraints.BOTH;
    pane.add(new JLabel("Run Queue"), c);

    // (0,1) scroll pane containing the Scan Settings jobs
    c = new GridBagConstraints();
    c.insets = new Insets(0, 10, 10, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // put the Scan Settings job list in the scroll pane
    JScrollPane scrollPane = new JScrollPane(runQueueL);
    scrollPane.setPreferredSize(new Dimension(600, 400));

     // add the scroll pane containing the Scan Settings job list
    pane.add(scrollPane, c);

    // (0,2) add the controls
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = 0;
    c.gridy = 2;
    pane.add(buildControls(), c);
  }

  private Component buildControls() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    int x=0;
    int y=0;
    // deleteB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 25);
    c.gridx = x++;
    c.gridy = y;
    container.add(deleteB, c);

    // editRedoB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(editRedoB, c);

    // upB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(upB, c);

    // downB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 5, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    container.add(downB, c);

    // closeB
    c = new GridBagConstraints();
    c.insets = new Insets(5, 25, 5, 5);
    c.gridx = x++;
    c.gridy = y;
    c.anchor = GridBagConstraints.FIRST_LINE_START;
    container.add(closeB, c);

    return container;
  }

  private void setButtonStates() {

    // set states for buttons
    deleteB.setEnabled(runQueueL.getSelectedIndex() >= 0);
    editRedoB.setEnabled(runQueueL.getSelectedIndex() >= 0);
    upB.setEnabled(runQueueL.getSelectedIndex() > 0);
    downB.setEnabled(runQueueL.getSelectedIndex() != -1
                && runQueueL.getSelectedIndex()
                     < BEViewer.scanSettingsListModel.getSize() - 1);
  }

  private void wireActions() {
    // JList selection state changes button states
    runQueueL.addListSelectionListener(new ListSelectionListener() {
      public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting() == false) {
          setButtonStates();
        }
      }
    });

    // changes to list data can change the list selection model
    BEViewer.scanSettingsListModel.addListDataListener(new ListDataListener() {
      public void contentsChanged(ListDataEvent e) {
        setButtonStates();
      }
      public void intervalAdded(ListDataEvent e) {
        // an added list item gets is selected
WLog.log("WSRQ.ia");
int size = BEViewer.scanSettingsListModel.getSize();
WLog.log("WSRQ.ia. size " + size);
ScanSettings scanSettings = BEViewer.scanSettingsListModel.getElementAt(size-1);
WLog.log("WSRQ.ia.b " + size + ", " + scanSettings);
runQueueL.setSelectedValue(scanSettings, true);
WLog.log("WSRQ.ia.c");
scanSettings = (ScanSettings)runQueueL.getSelectedValue();


        runQueueL.setSelectedValue(
                 BEViewer.scanSettingsListModel.getElementAt(
                 BEViewer.scanSettingsListModel.getSize() - 1)
                 , true);
WLog.log("WSRQ.ia.d");
scanSettings = (ScanSettings)runQueueL.getSelectedValue();
WLog.log("WSRQ.ia.e");
      }
      public void intervalRemoved(ListDataEvent e) {
        setButtonStates();
      }
    });

    // clicking deleteB deletes the scan settings job
    deleteB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettings scanSettings = (ScanSettings)runQueueL.getSelectedValue();
        BEViewer.scanSettingsListModel.remove(scanSettings);
      }
    });

    // clicking editRedoB 
    editRedoB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettings scanSettings = (ScanSettings)runQueueL.getSelectedValue();
        BEViewer.scanSettingsListModel.remove(scanSettings);
        WScan.openWindow(scanSettings);
      }
    });

    // clicking upB moves job up
    upB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettings scanSettings = (ScanSettings)runQueueL.getSelectedValue();
        BEViewer.scanSettingsListModel.moveUp(scanSettings);
        runQueueL.setSelectedIndex(runQueueL.getSelectedIndex() - 1);
      }
    });

    // clicking downB moves job down
    downB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettings scanSettings = (ScanSettings)runQueueL.getSelectedValue();
        BEViewer.scanSettingsListModel.moveDown(scanSettings);
        runQueueL.setSelectedIndex(runQueueL.getSelectedIndex() + 1);
      }
    });

    // clicking closeB closes this window
    closeB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        setVisible(false);
      }
    });
  }
}


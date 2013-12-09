import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.event.*;
import java.io.File;
import java.io.IOException;
import java.util.Observable;
import java.util.Observer;

/**
 * The toolbar for the run queue scheduler.
 */

public class ScanSettingsToolBar extends JToolBar {

  private static class FormedJButton extends JButton {
    FormedJButton(Icon icon, String tooltip) {
      setIcon(icon);
      setToolTipText(tooltip);
      setMinimumSize(BEViewer.BUTTON_SIZE);
      setPreferredSize(BEViewer.BUTTON_SIZE);
      setFocusable(false);
      setOpaque(false);
      setBorderPainted(false);
    }
  }

  // the list that this toolbar works with
  private final JList runQueueL;

  // the toolbar's buttons and checkbox
  private JButton runB;
  private JButton deleteB;
  private JButton editRedoB;
  private JButton upB;
  private JButton downB;
  private JCheckBox pauseCB;

  public ScanSettingsToolBar (JList runQueueL) {
    super("Scan Settings Toolbar", JToolBar.HORIZONTAL);
    setFloatable(false);
    this.runQueueL = runQueueL;

    // run button
    runB = new FormedJButton(BEIcons.RUN_BULK_EXTRACTOR_24, "Generate a new report");
    this.add(runB);

    // delete button
    deleteB = new FormedJButton(BEIcons.DELETE_24, "Delete selection from queue");
    this.add(deleteB);

    // editRedo button
    editRedoB = new FormedJButton(BEIcons.EDIT_REDO_24, "Edit selection");
    this.add(editRedoB);

    // up button
    upB = new FormedJButton(BEIcons.UP_24, "Move selection up queue to be run sooner");
    this.add(upB);

    // down button
    downB = new FormedJButton(BEIcons.DOWN_24, "Move selection down queue to be run later");
    this.add(downB);

    // separator
    addSeparator(new Dimension(20, 0));
 
    // pause checkbox
    pauseCB = new JCheckBox("Pause");
    pauseCB.setToolTipText("Delay starting the next bulk_extractor run");

    pauseCB.setFocusable(false);
    pauseCB.setOpaque(false); // this looks better with the ToolBar's gradient
    pauseCB.setRequestFocusEnabled(false);

    this.add(pauseCB);

    // set enabled states
    setEnabledStates();

    // wire listeners
    wireListeners();
  }

  private void setEnabledStates() {
//WLog.log("ScanSettingsToolBar.setEnabledStates set states for buttons, selected index "+ runQueueL.getSelectedIndex());

    // set states for buttons
    deleteB.setEnabled(runQueueL.getSelectedIndex() >= 0);
    editRedoB.setEnabled(runQueueL.getSelectedIndex() >= 0);
    upB.setEnabled(runQueueL.getSelectedIndex() > 0);
    downB.setEnabled(runQueueL.getSelectedIndex() != -1
                && runQueueL.getSelectedIndex()
                     < BEViewer.scanSettingsListModel.getSize() - 1);
  }

  private void wireListeners() {
    // on JList selection change, set button states
    runQueueL.addListSelectionListener(new ListSelectionListener() {
      public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting() == false) {
          setEnabledStates();
        }
      }
    });

    // on JList data change, change button states and, for add, change the list selection
    BEViewer.scanSettingsListModel.addListDataListener(new ListDataListener() {

      // contentsChanged
      public void contentsChanged(ListDataEvent e) {
        setEnabledStates();
      }

      // intervalAdded
      public void intervalAdded(ListDataEvent e) {
        setEnabledStates();
      }

      // intervalRemoved
      public void intervalRemoved(ListDataEvent e) {
        setEnabledStates();
      }
    });

    // clicking runB starts to define a new job
    runB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        WScan.openWindow(new ScanSettings());
      }
    });

    // clicking deleteB deletes the scan settings job
    deleteB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettings scanSettings = (ScanSettings)runQueueL.getSelectedValue();
        BEViewer.scanSettingsListModel.remove(scanSettings);
      }
    });

    // pause or un-pause the consumer
    pauseCB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettingsConsumer.pauseConsumer(pauseCB.isSelected());
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
      }
    });

    // clicking downB moves job down
    downB.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ScanSettings scanSettings = (ScanSettings)runQueueL.getSelectedValue();
        BEViewer.scanSettingsListModel.moveDown(scanSettings);
      }
    });
  }
}


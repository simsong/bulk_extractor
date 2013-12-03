import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.lang.Runtime;
import java.lang.Process;
import java.util.Observable;
import java.util.Observer;
import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * The dialog window for showing progress on a started bulk_extractor scan process.
 * Multiple windows and scans may be active.  This window is not modal.
 */

public class WScanProgress extends JDialog {
  private static final long serialVersionUID = 1;

  private final FileComponent imageFileLabel = new FileComponent();
  private final FileComponent featureDirectoryLabel = new FileComponent();
  private final JTextField commandField = new JTextField();

  private final JProgressBar progressBar = new JProgressBar();
  private final JLabel statusL = new JLabel();
  private final JTextArea outputArea = new JTextArea();
  private final JButton cancelB = new JButton("Cancel");

  // runnable factory class for obtaining WScanProgress
  private static class RunnableGetWScanProgress implements Runnable {
    private WScanProgress wScanProgress;
    public void run() {
      wScanProgress = new WScanProgress();
    }
  }

  // runnable class for showStart
  private class RunnableShowStart implements Runnable {
    private final ScanSettings scanSettings;
    private RunnableShowStart(ScanSettings scanSettings) {
      this.scanSettings = scanSettings;
    }
    public void run() {
      // put scanSettings values into UI components
      imageFileLabel.setFile(new File(scanSettings.inputImage));
      featureDirectoryLabel.setFile(new File(scanSettings.outdir));

      // show the command string in the command field
      String commandString = scanSettings.getCommandString();
WLog.log("WScanProgress commandString to start with: " + commandString);
      commandField.setText(commandString);
      commandField.setToolTipText(commandString);
      commandField.setCaretPosition(0);
    }
  }

  // runnable class for showStdout
  private class RunnableShowStdout implements Runnable {
    private final String input;
    private RunnableShowStdout(String input) {
      this.input = input;
    }
    public void run() {
      // set progress % in progress bar
      // check input for progress identifier: "(.*%)"
      int leftParenIndex = input.indexOf("(");
      int rightParenIndex = input.indexOf("%)");
      if (leftParenIndex > 0 && rightParenIndex > leftParenIndex) {
        // this qualifies as a progress line

        // put progress line in the status label
        statusL.setText(input);

        // set % in progress bar
        String progress = input.substring(leftParenIndex + 1, rightParenIndex);
        try {
          float progressFloat = Float.parseFloat(progress);
          progressBar.setValue((int)progressFloat);
          progressBar.setString(Float.toString(progressFloat) + "%");
        } catch (NumberFormatException e) {
          WLog.log("WScanProgress.run: unexpected progress value '" + progress + "' in stdout: " + input);
        }

      } else {
        // forward everything else to outputArea
        outputArea.append(input);
        outputArea.append("\n");
      }
    }
  }

  // runnable class for showStderr
  private class RunnableShowStderr implements Runnable {
    private final String input;
    public RunnableShowStderr(String input) {
      this.input = input;
    }

    public void run() {
      // forward all stderr to outputArea and to log
      WLog.log("bulk_extractor scan error: '" + input + "'");
      outputArea.append(input);
      outputArea.append("\n");
    }
  }

  // This class integrates "done" information into the UI
  private class RunnableShowDone implements Runnable {
    private final ScanSettings scanSettings;
    private final int exitValue;
    public RunnableShowDone(ScanSettings scanSettings, int exitValue) {
      this.scanSettings = scanSettings;
      this.exitValue = exitValue;
    }

    public void run() {

      // change "cancel" button to say "close"
      cancelB.setText("Close");

      // respond to termination based on the process' exit value
      if (exitValue == 0) {
        // successful run
        statusL.setText("bulk_extractor scan completed.");
        progressBar.setValue(100);
        progressBar.setString("Done");

        // As a user convenience, add the report that has been generated
        // by this run
        BEViewer.reportsModel.addReport(new File(scanSettings.outdir),
                                        new File(scanSettings.inputImage));

        // alert completion
        WError.showMessage("bulk_extractor has completed.\nReport "
                  + scanSettings.outdir
                  + " is ready for viewing.", "Report is Ready");

      } else {
        // failed run
        statusL.setText("Error: bulk_extractor terminated with exit value "
                        + exitValue);
        WLog.log("bulk_extractor error exit value: " + exitValue);
        progressBar.setString("Error");
        WError.showError("bulk_extractor Scanner terminated.",
                         "bulk_extractor failure", null);
      }
    }
  }

  /**
   * Threadsafe factory method for obtaining a new WScanProgress.
   */
  public synchronized static WScanProgress getWScanProgress() {
    RunnableGetWScanProgress getWScanProgress = new RunnableGetWScanProgress();
    try {
      SwingUtilities.invokeAndWait(getWScanProgress);
    } catch (Exception e) {
      throw new RuntimeException("WScanProgress getWScanProgress thread failure");
    }
    WScanProgress wScanProgress = getWScanProgress.wScanProgress;
    return wScanProgress;
  }

  /**
   * Threadsafe method for showing startup information.
   */
  public void showStart(ScanSettings scanSettings) {
    SwingUtilities.invokeLater(new RunnableShowStart(scanSettings));
  }
 
  /**
   * Threadsafe method for showing stdout
   */
  public void showStdout(String stdoutString) {
    SwingUtilities.invokeLater(new RunnableShowStdout(stdoutString));
  }

  /**
   * Threadsafe method for showing stderr
   */
  public void showStderr(String stderrString) {
    SwingUtilities.invokeLater(new RunnableShowStderr(stderrString));
  }

  /**
   * Threadsafe method for showing closure information
   */
  public void showDone(ScanSettings scanSettings, int exitValue) {
    SwingUtilities.invokeLater(new RunnableShowDone(scanSettings, exitValue));
  }

  // note that this is run privately by factory on the Swing thread
  private WScanProgress() {
    
    buildInterface();
    pack();
//    commandField.setText(commandString); // set after pack
//    commandField.setToolTipText(commandString); // set after pack
//    commandField.setCaretPosition(0);
    setClosure();
    setLocationRelativeTo(BEViewer.getBEWindow());
    getRootPane().setDefaultButton(cancelB);
    wireActions();
    setVisible(true);
  }

  // closure
  private void setClosure() {
    setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);	// close via handler
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        doClose();
      }
    });
  }

  private void buildInterface() {
    // set the title to include the image filename
    setTitle("bulk_extractor Scan");

    // use GridBagLayout with GridBagConstraints
    GridBagConstraints c;
    Container pane = getContentPane();
    pane.setLayout(new GridBagLayout());

    // (0,0) File container
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 0;
    c.fill = GridBagConstraints.HORIZONTAL;
    pane.add(getFileContainer(), c);

    // (0,1) Command field container
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 1;
//    c.weightx= 1;
    c.fill = GridBagConstraints.HORIZONTAL;
//    c.anchor = GridBagConstraints.LINE_START;
    pane.add(getCommandContainer(), c);

    // (0,2) Progress container
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 2;
    c.anchor = GridBagConstraints.LINE_START;
    pane.add(getProgressContainer(), c);

    // (0,3) bulk_extractor output area container
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 0, 5);
    c.gridx = 0;
    c.gridy = 3;
    c.weightx= 1;
    c.weighty= 1;
    c.fill = GridBagConstraints.BOTH;
    pane.add(getOutputContainer(), c);

    // (0,4) Cancel
    c = new GridBagConstraints();
    c.insets = new Insets(15, 5, 15, 5);
    c.gridx = 0;
    c.gridy = 4;

    // add the cancel button
    pane.add(cancelB, c);
  }

  private void wireActions() {
    // service the cancel button
    cancelB.addActionListener(new ActionListener() {
      public void actionPerformed (ActionEvent e) {
        doClose();
      }
    });
  }

  private void doClose() {
    // cancel
    ScanSettingsConsumer.killBulkExtractorProcess();
    dispose();
  }

  // File container
  private Container getFileContainer() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) "Image File"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Image File"), c);

    // (1,0) <image file>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 0;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(imageFileLabel, c);

    // (0,1) "Feature Directory"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 1;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Feature Directory"), c);

    // (1,1) <feature directory>
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 1;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;
    container.add(featureDirectoryLabel, c);

    return container;
  }

  // Command container
  private Container getCommandContainer() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) "command"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Command"), c);

    // (0,1) command text field
    commandField.setEditable(false);
    commandField.setMinimumSize(new Dimension(0, commandField.getPreferredSize().height));

    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.fill = GridBagConstraints.HORIZONTAL;

    // add the command field
    container.add(commandField, c);

    return container;
  }

  // Progress container
  private Container getProgressContainer() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) "progress"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 10);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("Progress"), c);

    // (1,0) progress bar
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 1;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    progressBar.setPreferredSize(new Dimension(60, progressBar.getPreferredSize().height));
    progressBar.setMinimumSize(progressBar.getPreferredSize());
    progressBar.setStringPainted(true);
    progressBar.setMinimum(0);
    progressBar.setMaximum(100);
    progressBar.setValue(0);
    progressBar.setString("0%");
    container.add(progressBar, c);

    // (0,1) status text
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 1;
    c.gridwidth = 2;
    c.fill = GridBagConstraints.HORIZONTAL;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(statusL, c);

    return container;
  }

  // bulk_extractor output container
  private Container getOutputContainer() {
    GridBagConstraints c;
    Container container = new Container();
    container.setLayout(new GridBagLayout());

    // (0,0) "bulk_extractor output"
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 0;
    c.anchor = GridBagConstraints.LINE_START;
    container.add(new JLabel("bulk_extractor output"), c);

    // (0,1) output scrollpane for containing output from bulk_extractor
    outputArea.setEditable(false);
    JScrollPane outputScrollPane = new JScrollPane(outputArea,
                       ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
                       ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
    outputScrollPane.setPreferredSize(new Dimension(500, 200));
    c = new GridBagConstraints();
    c.insets = new Insets(0, 0, 0, 0);
    c.gridx = 0;
    c.gridy = 1;
    c.weightx = 1;
    c.weighty = 1;
    c.fill = GridBagConstraints.BOTH;

    // add the output scrollpane
    container.add(outputScrollPane, c);

    return container;
  }
}


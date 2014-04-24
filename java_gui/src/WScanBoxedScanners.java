import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.*;
import java.util.Vector;
import java.util.Iterator;
import java.io.IOException;
import java.util.HashMap;

/**
 * Manage the scanner selections (selected feature recorders).
 */

public class WScanBoxedScanners {

  /**
   * FeatureScanner contains UI and state for one scanner: its command name,
   * CheckBox, default value, and user's selected value.
   */
/*
  public static class FeatureScanner {
    final JCheckBox scannerCB;
    final boolean defaultUseScanner;
    final String command;
    boolean useScanner;
    FeatureScanner(String command, boolean defaultUseScanner) {
      scannerCB = new JCheckBox(command);
      this.command = command;
      this.defaultUseScanner = defaultUseScanner;
//      this.useScanner = defaultUseScanner;
      scannerCB.setSelected(useScanner);
    }
  }
*/

  public static JPanel container = new JPanel();
  public static Component component;
  private static HashMap<String, JCheckBox> scannerMap = new HashMap<String, JCheckBox>();

  public WScanBoxedScanners() {
    component = buildContainer();
    wireActions();
  }

  private Component buildContainer() {
    // container using GridBagLayout with GridBagConstraints
    container.setBorder(BorderFactory.createTitledBorder("Scanners"));
    container.setLayout(new GridBagLayout());
    setScannerList();
    return container;
  }

  // for each scanner from bulk_extractor, create the checkboxes
  // and add a map to the checkboxes for future access
  private static void setScannerList() {

    // get the scanner list from bulk_extractor
    Vector<BulkExtractorScanListReader.Scanner> scanners;
    try {
      scanners = BulkExtractorScanListReader.readScanList(
              WScanBoxedControls.usePluginDirectoriesCB.isSelected(),
              WScanBoxedControls.pluginDirectoriesTF.getText());
    } catch (IOException e) {
      WError.showError("Error in obtaining list of scanners from bulk_extractor."
                     + "\nBulk_extractor is not available during this session."
                     + "\nIs bulk_extractor installed?",
                         "bulk_extractor failure", e);
      scanners = new Vector<BulkExtractorScanListReader.Scanner>();
    }

    // add a JCheckBox to the container for each scanner
    int y=0;
    for (Iterator<BulkExtractorScanListReader.Scanner> it = scanners.iterator(); it.hasNext();) {
      BulkExtractorScanListReader.Scanner scanner = it.next();

      // add the checkbox to the container
      JCheckBox checkBox = new JCheckBox(scanner.name);
      WScan.addOptionLine(container, y++, checkBox);

      // also add the checkbox to the scanner map
      // for future access to its selection value
      scannerMap.put(scanner.name, checkBox);
    }
  }

  public void setScanSettings(ScanSettings scanSettings) {

    // iterate over scanSettings.scanners and set the corresponding
    // selection state in the associated JCheckBox
    for (Iterator<BulkExtractorScanListReader.Scanner> it = scanSettings.scanners.iterator(); it.hasNext();) {
      BulkExtractorScanListReader.Scanner scanner = it.next();
      JCheckBox checkbox = scannerMap.get(scanner.name);
      checkbox.setSelected(scanner.useScanner);
    }
  }

  public void getScanSettings(ScanSettings scanSettings) {

    // iterate over scanSettings.scanners and get the selection state
    // from the associated JCheckBox
    for (Iterator<BulkExtractorScanListReader.Scanner> it = scanSettings.scanners.iterator(); it.hasNext();) {
      BulkExtractorScanListReader.Scanner scanner = it.next();
      JCheckBox checkbox = scannerMap.get(scanner.name);
      scanner.useScanner = checkbox.isSelected();
    }
  }

  private void wireActions() {
    // no action
  }
}
 

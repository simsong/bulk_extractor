import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.*;
import java.util.Vector;
import java.util.Enumeration;
import java.io.IOException;

/**
 * Manage the scanner selections (selected feature recorders).
 */

public class WScanBoxedScanners {

  /**
   * FeatureScanner contains UI and state for one scanner: its command name,
   * CheckBox, default value, and user's selected value.
   */
  public static class FeatureScanner {
    final JCheckBox scannerCB;
    final boolean defaultUseScanner;
    final String command;
    boolean useScanner;
    FeatureScanner(String command, boolean defaultUseScanner) {
      scannerCB = new JCheckBox(command);
      this.command = command;
      this.defaultUseScanner = defaultUseScanner;
      this.useScanner = defaultUseScanner;
      scannerCB.setSelected(useScanner);
    }
  }

  public static JPanel container = new JPanel();
  public static Vector<BulkExtractorScanListReader.Scanner> scanners
                       = new Vector<BulkExtractorScanListReader.Scanner>();
  public static Vector<FeatureScanner> featureScanners
                       = new Vector<FeatureScanner>();
  public static Component component;

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

  // Rebuild the scanner list during runtime
  public static void setScannerList() {

    scanners.clear();
    container.removeAll();

    // get the scanners list from bulk_extractor
    try {
      BulkExtractorScanListReader.readScanList(
              WScanBoxedControls.usePluginDirectoryCB.isSelected(),
              WScanBoxedControls.pluginDirectoryTF.getText(),
              scanners);
    } catch (IOException e) {
      WError.showError("Error in obtaining list of scanners from bulk_extractor."
                     + "\nBulk_extractor is not available during this session."
                     + "\nIs bulk_extractor installed?",
                         "bulk_extractor failure", e);
    }

    // add scanner items
    int y=0;
    for (Enumeration e = scanners.elements(); e.hasMoreElements();) {
      // add the FeatureScanner objects to the Vector
      BulkExtractorScanListReader.Scanner scanner =
            (BulkExtractorScanListReader.Scanner)e.nextElement();
      FeatureScanner featureScanner = new FeatureScanner(scanner.command, scanner.defaultUseScanner);
      featureScanners.add(featureScanner);
      WScan.addOptionLine(container, y++, featureScanner.scannerCB);
    }
    container.revalidate();
  }

  public void setDefaultValues() {
    // Scanners (feature recorders)
//    for (Enumeration<FeatureScanner> e = (Enumeration<FeatureScanner>)(featureScanners.elements()); e.hasMoreElements();); {
    for (Enumeration e = featureScanners.elements(); e.hasMoreElements();) {
      FeatureScanner featureScanner = (FeatureScanner)e.nextElement();
      featureScanner.useScanner = featureScanner.defaultUseScanner;
    }
  }

  public void setUIValues() {
    // Scanners (feature recorders)
//    for (Enumeration<FeatureScanner> e = (Enumeration<FeatureScanner>)(featureScanners.elements()); e.hasMoreElements();); {
    for (Enumeration e = featureScanners.elements(); e.hasMoreElements();) {
      FeatureScanner featureScanner = (FeatureScanner)e.nextElement();
      featureScanner.scannerCB.setSelected(featureScanner.useScanner);
    }
  }

  public void getUIValues() {
    // Scanners (feature recorders)
//    for (Enumeration<FeatureScanner> e = (Enumeration<FeatureScanner>)(featureScanners.elements()); e.hasMoreElements();); {
    for (Enumeration e = featureScanners.elements(); e.hasMoreElements();) {
      FeatureScanner featureScanner = (FeatureScanner)e.nextElement();
      featureScanner.useScanner = featureScanner.scannerCB.isSelected();
    }
  }

  public boolean validateValues() {
    return true;
  }

  private void wireActions() {
    // Scanners (feature recorders)
    // no action
  }
}
 

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
   * this class contains a scanner: its command name, CheckBox, default value,
   * and user's selected value.
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
    }
  }
  public final static Vector<FeatureScanner> featureScanners = new Vector<FeatureScanner>();
  static {
    Vector<BulkExtractorScanListReader.Scanner> scanners;
    try {
      scanners = BulkExtractorScanListReader.readScanList();
    } catch (IOException e) {
      WError.showError("Error in obtaining list of scanners from bulk_extractor."
                       + "\nBulk_extractor is not available during this session.",
                         "bulk_extractor failure", e);
      scanners = new Vector<BulkExtractorScanListReader.Scanner>();
    }
    for (Enumeration e = scanners.elements(); e.hasMoreElements();) {
      BulkExtractorScanListReader.Scanner scanner =
            (BulkExtractorScanListReader.Scanner)e.nextElement();
      featureScanners.add(new FeatureScanner(scanner.command, scanner.defaultUseScanner));
    }
  }

  public final Component component;

  public WScanBoxedScanners() {
    component = buildContainer();
    wireActions();
  }

  private Component buildContainer() {
    // container using GridBagLayout with GridBagConstraints
    JPanel container = new JPanel();
    container.setBorder(BorderFactory.createTitledBorder("Scanners"));
    container.setLayout(new GridBagLayout());
    int y = 0;
//    for (Enumeration<FeatureScanner> e = (Enumeration<FeatureScanner>)(featureScanners.elements()); e.hasMoreElements();); {
    for (Enumeration e = featureScanners.elements(); e.hasMoreElements();) {
      FeatureScanner featureScanner = (FeatureScanner)e.nextElement();
      WScan.addOptionLine(container, y++, featureScanner.scannerCB);
    }

    return container;
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
 

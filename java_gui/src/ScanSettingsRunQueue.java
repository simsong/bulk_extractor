import java.util.Vector;
import java.util.Iterator;

/**
 * The <code>ScanSettingsRunQueue</code> class manages the bulk_extractor
 * run queue.
 *
 * The queue is a javax.swing.DefaultListModel so that it can integrate
 * with a JList.
 *
 * Locked interfaces add, remove, swap, and change queued ScanSettings objects.
 * 
 * Events are fired so:
 *   Lists can stay current.
 *   The consumer can run the next ScanSettings job using bulk_extractor.
 */

public class ScanSettingsRunQueue {
//  private static Vector<ScanSettings> jobs = new Vector<ScanSettings>();
  private static DefaultListModel<ScanSettings> jobs = new Vector<ScanSettings>();

  // singleton with no class
  private ScanSettingsRunQueue() {
  }

  /**
   * Add ScanSettings to tail (bottom) of LIFO job queue.
   */
  public synchronized static void add(ScanSettings scanSettings) {
    jobs.add(scanSettings);
  }

  /**
   * Remove ScanSettings from head (top) of LIFO job queue.
   */
  public synchronized static void remove(ScanSettings scanSettings) {
    jobs.remove(scanSettings);
  }

  /**
   * Swap the order of two elements
   */
  public synchronized static void swap(ScanSettings s1, ScanSettings s2) {
    // java.util.Vector does not have a swap command, so use this
    int n1 = job.indexOf(s1);
    int n2 = job.indexOf(s2);
    ScanSettings j1 = jobs.get(n1);
    ScanSettings j2 = jobs.get(n2);
    jobs.setElementAt(j2, n1);
    jobs.setElementAt(j1, n2);
  }






    // required parameters
    imageSourceType = ImageSourceType.IMAGE_FILE;
    inputImage = "";
    outdir = "";

    // general options
    useBannerFile = false;
    bannerFile = "";
    useAlertlistFile = false;
    alertlistFile = "";
    useStoplistFile = false;
    stoplistFile = "";
    useFindRegexTextFile = false;
    findRegexTextFile = "";
    useFindRegexText = false;
    findRegexText = "";
    useRandomSampling = false;
    randomSampling = "";

    // tuning parameters
    useContextWindowSize = false;
    contextWindowSize = "16";
    usePageSize = false;
    pageSize = "16777216";
    useMarginSize = false;
    marginSize = "4194304";
    useBlockSize = false;
    blockSize = "512";
    useNumThreads = false;
    numThreads = Integer.toString(Runtime.getRuntime().availableProcessors());
    useMaxRecursionDepth = false;
    maxRecursionDepth = "7";
    useMaxWait = false;
    maxWait = "60";

    // parallelizing
    useStartProcessingAt = false;
    startProcessingAt = "";
    useProcessRange = false;
    processRange = "";
    useAddOffset = false;
    addOffset = "";

    // Debugging options
    useStartOnPageNumber = false;
    startOnPageNumber = "0";
    useDebugNumber = false;
    debugNumber = "1";
    useEraseOutputDirectory = false;

    // Scanner controls
    usePluginDirectory = false;
    pluginDirectory = "";
    useSettableOptions = false;
    settableOptions = "";

    // scanners
    try {
      // get the default scanner list from bulk_extractor
      scanners = BulkExtractorScanListReader.readScanList(
                       WScanBoxedControls.usePluginDirectoryCB.isSelected(),
                       WScanBoxedControls.pluginDirectoryTF.getText());
    } catch (IOException e) {
      WError.showError("Error in obtaining list of scanners from bulk_extractor."
                     + "\nBulk_extractor is not available during this session."
                     + "\nIs bulk_extractor installed?",
                         "bulk_extractor failure", e);
      scanners = new Vector<BulkExtractorScanListReader.Scanner>();
    }
  }

  /**
   * instantiate by copying
   */
  public ScanSettings(ScanSettings sc) {

    // required parameters
    imageSourceType = sc.imageSourceType;
    inputImage = sc.inputImage;
    outdir = sc.outdir;

    // general options
    useBannerFile = sc.useBannerFile;
    bannerFile = sc.bannerFile;
    useAlertlistFile = sc.useAlertlistFile;
    alertlistFile = sc.alertlistFile;
    useStoplistFile = sc.useStoplistFile;
    stoplistFile = sc.stoplistFile;
    useFindRegexTextFile = sc.useFindRegexTextFile;
    findRegexTextFile = sc.findRegexTextFile;
    useFindRegexText = sc.useFindRegexText;
    findRegexText = sc.findRegexText;
    useRandomSampling = sc.useRandomSampling;
    randomSampling = sc.randomSampling;

    // tuning parameters
    useContextWindowSize = sc.useContextWindowSize;
    contextWindowSize = sc.contextWindowSize;
    usePageSize = sc.usePageSize;
    pageSize = sc.pageSize;
    useMarginSize = sc.useMarginSize;
    marginSize = sc.marginSize;
    useBlockSize = sc.useBlockSize;
    blockSize = sc.blockSize;
    useNumThreads = sc.useNumThreads;
    numThreads = sc.numThreads;
    useMaxRecursionDepth = sc.useMaxRecursionDepth;
    maxRecursionDepth = sc.maxRecursionDepth;
    useMaxWait = sc.useMaxWait;
    maxWait = sc.maxWait;

    // parallelizing
    useStartProcessingAt = sc.useStartProcessingAt;
    startProcessingAt = sc.startProcessingAt;
    useProcessRange = sc.useProcessRange;
    processRange = sc.processRange;
    useAddOffset = sc.useAddOffset;
    addOffset = sc.addOffset;

    // Debugging options
    useStartOnPageNumber = sc.useStartOnPageNumber;
    startOnPageNumber = sc.startOnPageNumber;
    useDebugNumber = sc.useDebugNumber;
    debugNumber = sc.debugNumber;
    useEraseOutputDirectory = sc.useEraseOutputDirectory;

    // Scanner controls
    usePluginDirectory = sc.usePluginDirectory;
    pluginDirectory = sc.pluginDirectory;
    useSettableOptions = sc.useSettableOptions;
    settableOptions = sc.settableOptions;

    // scanners
    scanners = new Vector<BulkExtractorScanListReader.Scanner>();
    for (Iterator<BulkExtractorScanListReader.Scanner> it = sc.scanners.iterator(); it.hasNext();) {
      BulkExtractorScanListReader.Scanner scanner = it.next();
      scanners.add(new BulkExtractorScanListReader.Scanner(scanner.name, scanner.defaultUseScanner, scanner.useScanner));
    }
  }

  public String[] getCommandArray() {
    Vector<String> cmd = new Vector<String>();

    // basic usage: bulk_extractor [options] imagefile
    // program name
    cmd.add("bulk_extractor");

    // options
    // required parameters
    cmd.add("-o");
    cmd.add(outdir);
    
    // general options
    if (useBannerFile) {
      cmd.add("-b");
      cmd.add(bannerFile);
    }
    if (useAlertlistFile) {
      cmd.add("-r");
      cmd.add(alertlistFile);
    }
    if (useStoplistFile) {
      cmd.add("-w");
      cmd.add(stoplistFile);
    }
    if (useFindRegexTextFile) {
      cmd.add("-F");
      cmd.add(findRegexTextFile);
    }
    if (useFindRegexText) {
      cmd.add("-f");
      cmd.add(findRegexText);
    }
    if (useRandomSampling) {
      cmd.add("-s");
      cmd.add(randomSampling);
    }

    // tuning parameters
    if (useContextWindowSize) {
      cmd.add("-C");
      cmd.add(contextWindowSize);
    }
    if (usePageSize) {
      cmd.add("-G");
      cmd.add(pageSize);
    }
    if (useMarginSize) {
      cmd.add("-g");
      cmd.add(marginSize);
    }
    if (useBlockSize) {
      cmd.add("-B");
      cmd.add(blockSize);
    }
    if (useNumThreads) {
      cmd.add("-j");
      cmd.add(numThreads);
    }
    if (useMaxRecursionDepth) {
      cmd.add("-M");
      cmd.add(maxRecursionDepth);
    }
    if (useMaxWait) {
      cmd.add("-m");
      cmd.add(maxWait);
    }

    // parallelizing
    if (useStartProcessingAt) {
      cmd.add("-Y");
      cmd.add(startProcessingAt);
    }
    if (useProcessRange) {
      cmd.add("-Y");
      cmd.add(processRange);
    }
    if (useAddOffset) {
      cmd.add("-A");
      cmd.add(addOffset);
    }

    // Debugging
    if (useStartOnPageNumber) {
      cmd.add("-z");
      cmd.add(startOnPageNumber);
    }
    if (useDebugNumber) {
      cmd.add("-d" + debugNumber);
    }
    if (useEraseOutputDirectory) {
      cmd.add("-Z");
    }
  
    // controls
    if (usePluginDirectory) {
      cmd.add("-P");
      cmd.add(pluginDirectory);
    }
    if (useSettableOptions
     && settableOptions.length() > 0) {
      String[] settableOptionsArray = settableOptions.split("\\|");
      for (String optionName : settableOptionsArray) {
        cmd.add("-S");
        cmd.add(optionName);
      }
    }

    // Scanners
    for (Iterator<BulkExtractorScanListReader.Scanner> it = scanners.iterator(); it.hasNext();) {
      BulkExtractorScanListReader.Scanner scanner = it.next();
      if (scanner.defaultUseScanner) {
        if (!scanner.useScanner) {
          // disable this scanner that is enabled by default
          cmd.add("-x");
          cmd.add(scanner.name);
        }
      } else {
        if (scanner.useScanner) {
          // enable this scanner that is disabled by default
          cmd.add("-e");
          cmd.add(scanner.name);
        }
      }
    }

    // required imagefile or directory of files
    if (imageSourceType == ImageSourceType.DIRECTORY_OF_FILES) {
      // recurse through a directory of files
      cmd.add("-R");
    }
    cmd.add(inputImage);

    // convert the cmd vector to string array
    String[] command = cmd.toArray(new String[0]);

    // return the command string array
    return command;
  }

  public String getCommandString() {
    String[] commandArray = getCommandArray();

    // build the string by concatenating tokens separated by space
    // and quoting any tokens containing space.
    StringBuffer buffer = new StringBuffer();
    for (int i=0; i<commandArray.length; i++) {
      // append space separator between command array parts
      if (i > 0) {
        buffer.append(" ");
      }

      // append command array part
      if (commandArray [i].indexOf(" ") >= 0) {
        // append with quotes
        buffer.append("\"" + commandArray[i] + "\"");
      } else {
        // append without quotes
        buffer.append(commandArray[i]);
      }
    }
    return buffer.toString();
  }

  // validator for integers
  private boolean isInt(String value, String name) {
    try {
      int i = Integer.valueOf(value).intValue();
      return true;
    } catch (NumberFormatException e) {
      WError.showError("Invalid input for " + name + ": " + value,
                       "Settings error", null);
      return false;
    }
  }

  /**
   * This is a courtesy function for providing some validation of the settings.
   */
  public boolean validateSomeSettings() {

    // required parameters

    // validate the input image
    File image = new File(inputImage);
    if (imageSourceType == ImageSourceType.IMAGE_FILE) {
      // validate the image file as readable and not a directory
      if (image.isDirectory() || !image.canRead()) {
        WError.showError("The image file provided,\n'" + inputImage + "', is not valid."
                         + "\nPlease verify that this path exists and is accessible.",
                         "Settings error", null);
        return false;
      }
    } else if (imageSourceType == ImageSourceType.RAW_DEVICE) {
      // validate the device as readable and not a directory
      if (image.isDirectory() || !image.canRead()) {
        WError.showError("The image device provided,\n'" + inputImage + "', is not valid."
                         + "\nPlease verify that this path exists and is accessible.",
                         "Settings error", null);
        return false;
      }
    } else if (imageSourceType == ImageSourceType.DIRECTORY_OF_FILES) {
      // validate the input directory
      if (!image.isDirectory() || !image.canRead()) {
        WError.showError("The input image directory provided,\n'" + inputImage + "', is not valid."
                         + "\nPlease verify that this path exists and is accessible.",
                         "Settings error", null);
        return false;
      }
    }

    // validate that the directory above the output feature directory exists
    File directory = new File(outdir);
    File parent = directory.getParentFile();
    if (parent == null || !parent.isDirectory()) {
      WError.showError("The folder to contain Output Feature directory\n'" + directory
                     + "' is not valid."
                     + "\nPlease verify that this folder exists and is accessible.",
                     "Settings error", null);
      return false;
    }

    // general options

    // banner file
    if (useBannerFile) {
      File bf = new File(bannerFile);
      if (!bf.isFile() || !bf.canRead()) {
        WError.showError("The banner file \n'" + bannerFile
                       + "' is not valid."
                       + "\nPlease verify that this file exists and is accessible.",
                       "Settings error", null);
        return false;
      }
    }
    // alert list file
    if (useAlertlistFile) {
      File alf = new File(alertlistFile);
      if (!alf.isFile() || !alf.canRead()) {
        WError.showError("The alert list file \n'" + alertlistFile
                       + "' is not valid."
                       + "\nPlease verify that this file exists and is accessible.",
                       "Settings error", null);
        return false;
      }
    }
    // stoplist file
    if (useStoplistFile) {
      File slf = new File(stoplistFile);
      if (!slf.isFile() || !slf.canRead()) {
        WError.showError("The alert list file \n'" + stoplistFile
                       + "' is not valid."
                       + "\nPlease verify that this file exists and is accessible.",
                       "Settings error", null);
        return false;
      }
    }
    // find regex text file
    if (useFindRegexTextFile) {
      File frtf = new File(findRegexTextFile);
      if (!frtf.isFile() || !frtf.canRead()) {
        WError.showError("The alert list file \n'" + findRegexTextFile
                       + "' is not valid."
                       + "\nPlease verify that this file exists and is accessible.",
                       "Settings error", null);
        return false;
      }
    }

    // tuning parameters
    if (useContextWindowSize
        && !isInt(contextWindowSize, "context window size")) {
      return false;
    }
    if (usePageSize
        && !isInt(pageSize, "page size")) {
      return false;
    }
    if (useMarginSize
        && !isInt(marginSize, "margin size")) {
      return false;
    }
    if (useBlockSize
        && !isInt(blockSize, "block size")) {
      return false;
    }
    if (useNumThreads
        && !isInt(numThreads, "number of threads")) {
      return false;
    }
    if (useMaxRecursionDepth
        && !isInt(maxRecursionDepth, "maximum recursion depth")) {
      return false;
    }

    // parallelizing
    // no checks at this time

    // debugging options
    if (useStartOnPageNumber
        && !isInt(startOnPageNumber, "start on page number")) {
      return false;
    }
    if (useDebugNumber
        && !isInt(debugNumber, "debug mode number")) {
      return false;
    }

    // scanner controls
    // no checks at this time

    // scanners
    // no checks

    return true;
  }
}


/**
 * This static class runs up to one command from user input.
 */
public class BEArgsParser {
  private BEArgsParser() {
  }

  private static final String usage = 
          "Usage: type up to one of the following commands:"
      + "\n        -s \"<scanner arguments>\""
      + "\n        -clear_preferences";

  private static void showUsage() {
    WError.showMessage(usage, "Command Line Usage");
  }

  /**
   * Parse arguments for zero or one command.
   */
  public static void parseArgs(String[] args) {
    int length = args.length;

    // only activate if there are args
    if (length == 0) {
      return;
    }

    // start bulk_extractor scan
    if (args[0].equals("-s")) {
      if (length == 2) {
        // good, create bulk_extractor arg array
        ScanSettings scanSettings = new ScanSettings(args[1]);
        BEViewer.scanSettingsListModel.add(scanSettings);
      } else {
        showUsage();
      }

    // clear Preferences
    } else if (args[0].equals("-clear_preferences")) {
      if (length == 1) {
        BEPreferences.clearPreferences();
      } else {
        showUsage();
      }

    // command not recognized
    } else {
      showUsage();
    }
  }
}


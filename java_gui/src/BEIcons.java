import java.util.HashMap;
import javax.swing.Icon;
import javax.swing.ImageIcon;

public final class BEIcons {
  private BEIcons() {
  }

  private static HashMap<String, String> map = new HashMap<String, String>();
  static {
    map.put("add_bookmark", "bookmark-new-2.png");
    map.put("manage_bookmarks", "bookmark-2.png");
    map.put("export_bookmarks", "archive-extract-3.png");
    map.put("delete", "edit-delete-6.png");
    map.put("edit_redo", "edit-redo-8.png");
    map.put("up", "go-up-4.png");
    map.put("down", "go-down-4.png");
    map.put("reverse", "go-previous-4.png");
    map.put("forward", "go-next-4.png");
    map.put("home", "go-home-4.png");
    map.put("open_report", "document-open-7.png");
    map.put("copy", "edit-copy-3.png");
    map.put("run_bulk_extractor", "run-build-install.png");
    map.put("print", "document-print.png");
    map.put("close", "window-close.png");
    map.put("help", "help-contents.png");
    map.put("help_about", "help-about.png");
    map.put("exit", "application-exit.png");
  }

  // icons
  private static final String PATH_16 = "icons/16/";
  private static final String PATH_24 = "icons/24/";
  private static final ClassLoader classLoader = Thread.currentThread().getContextClassLoader();

  private static Icon getIcon16(String name) {
    Icon icon;
    try {
      icon = new ImageIcon(classLoader.getResource(PATH_16 + map.get(name)));
    } catch (RuntimeException e) {
      WLog.log("Error loading image in getIcon16: " + name + ", " + map.get(name));
      throw e;
    }
    return icon;
  }
  private static Icon getIcon24(String name) {
    Icon icon;
    try {
      icon = new ImageIcon(classLoader.getResource(PATH_24 + map.get(name)));
    } catch (RuntimeException e) {
      WLog.log("Error loading image in getIcon24: " + name + ", " + map.get(name));
      throw e;
    }
    return icon;
  }

  public static final Icon CLOSE_16 = getIcon16("close");
  public static final Icon CLOSE_24 = getIcon24("close");
  public static final Icon MANAGE_BOOKMARKS_16 = getIcon16("manage_bookmarks");
  public static final Icon MANAGE_BOOKMARKS_24 = getIcon24("manage_bookmarks");
  public static final Icon ADD_BOOKMARK_16 = getIcon16("add_bookmark");
  public static final Icon ADD_BOOKMARK_24 = getIcon24("add_bookmark");
  public static final Icon EXPORT_BOOKMARKS_16 = getIcon16("export_bookmarks");
  public static final Icon EXPORT_BOOKMARKS_24 = getIcon24("export_bookmarks");
  public static final Icon DELETE_16 = getIcon16("delete");
  public static final Icon DELETE_24 = getIcon24("delete");
  public static final Icon EDIT_REDO_16 = getIcon16("edit_redo");
  public static final Icon EDIT_REDO_24 = getIcon24("edit_redo");
  public static final Icon UP_16 = getIcon16("up");
  public static final Icon UP_24 = getIcon24("up");
  public static final Icon DOWN_16 = getIcon16("down");
  public static final Icon DOWN_24 = getIcon24("down");
  public static final Icon REVERSE_16 = getIcon16("reverse");
  public static final Icon REVERSE_24 = getIcon24("reverse");
  public static final Icon FORWARD_16 = getIcon16("forward");
  public static final Icon FORWARD_24 = getIcon24("forward");
  public static final Icon HOME_16 = getIcon16("home");
  public static final Icon HOME_24 = getIcon24("home");
  public static final Icon OPEN_REPORT_16 = getIcon16("open_report");
  public static final Icon OPEN_REPORT_24 = getIcon24("open_report");
  public static final Icon COPY_16 = getIcon16("copy");
  public static final Icon COPY_24 = getIcon24("copy");
  public static final Icon RUN_BULK_EXTRACTOR_16 = getIcon16("run_bulk_extractor");
  public static final Icon RUN_BULK_EXTRACTOR_24 = getIcon24("run_bulk_extractor");
  public static final Icon PRINT_FEATURE_16 = getIcon16("print");
  public static final Icon PRINT_FEATURE_24 = getIcon24("print");
  public static final Icon HELP_16 = getIcon16("help");
  public static final Icon HELP_24 = getIcon24("help");
  public static final Icon HELP_ABOUT_16 = getIcon16("help_about");
  public static final Icon HELP_ABOUT_24 = getIcon24("help_about");
  public static final Icon EXIT_16 = getIcon16("exit");
  public static final Icon EXIT_24 = getIcon24("exit");
}


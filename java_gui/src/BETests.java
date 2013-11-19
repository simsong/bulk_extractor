import java.io.File;
/**
 * The <code>BETest</code> class provides test methods.
 * For some tests, output must be externally verified.
 * Obvious failures throw exceptions.
 * NOTE: tests that wait for thread completion must not run from the Swing thread
 * because the Swing thread cannot wait.

To invoke the test suite:
Start BEViewer and select menu command "Help Diagnostics Run Tests".
Adjust the inputs, as necessary, as required for each test suite scheduled to run.
Select which tests to run.
Current tests are:
\li Preferences
\li Bookmarks
\li Scan

See individual tests for output files and how tests are verified.

\par Tests
Each test identifies setup requirements, comments about what is tested,
and how to verify completion.

\par Preferences Test
This test verifies the user Preferences subsystem by copying loaded preferences,
saving preferences, then reloading saved preferences, and comparing that the results are equal.
\n
For setup, define some preferences: run BEViewer and open some Reports and bookmark some Features.
\n
This test demonstrates that the system can initialize while loading a set of preferences,
and also that preferences can be saved and reloaded without corruption.
This test does not verify that the system is configured correctly according to the preferences.

\par bookmarks_test Bookmarks Test
This test verifies many BEViewer and bulk_extractor subsystems
by loading a Case file and generating case reports
identified by bookmarked Features defined in the Case file.
\n
This test requires a Case File of bookmarks as input and produces Bookmark output
in the Test Output Directory.
\n
Each bookmarked feature includes the source of the bookmark feature (the
feature file and image file involved),
the feature text obtained from the Feature file,
and the extracted Media excerpt containing the feature.
\n
A successful Bookmarks test requires that a comprehensive set of bookmark definitions
be exercised.
The set of bookmarks to test is provided
in the casefile provided during test invocation.

\par Corner Cases
Bookmarks should test corner cases, each reader used, and each media type supported.
\n
Potential good bookmark tests include the following corner cases:
\li Reading across page boundaries.
\li Reading both Offset and compressed Path values.
\li Reading large Offset vlaues.

\par Media Readers
It is important to test each reader BEViewer uses to ensure that the reader functions properly:
\li Native Java BEViewer readers.  These readers provide native reads
for Offset values, but do not support reading compressed Path values.
\li bulk_extractor Readers in bulk_extractor provde offset and compressed Path readers
for all supported media types.

BEViewer contains a switch for selecting each reader available.
By defining bookmarks for testing each media type and by testing each reader,
all readers are exercised.
\n
Two significant types of image reads are available:
\li Offset reads which read media relative to the beginning of the media.
\li Compressed Path reads which read decompressed media based on a path
containing a compression identifier such as "ZIP", for example "1000-zip-250",
which returns decompressed data
including the decompressed region 250 bytes into the decompressed data,
where the decompressed data was obtained by performing a reverse zip of the media
starting at an offset of 1000 bytes into the Image media.

Only bulk_extractor supports reading Compresed Paths.
\par Media Formats
It is important to test each media format type supported for each reader.
Supported media types follow:
\li .E01
\li .001
\li .raw

\par output Output
Syntax of output is impacted
by 1) whether the Image is displayed in text or hex format,
and 2) whether image Offsets are displayed in Decimal or Hexidecimal.
\n
BEViewer contains switches for selecting its output formats.
It is not significant to regression-test each output format
with each Bookmark tested.
All outputs should include the same information.
It is important to verify that each output format is displayed properly.
\n

\par Verification of Results
This test only verifies that bookmarks can be exported.
Verify the correctness of output externally by comparing generated files against known good files.
Recall that not all readers can read all format types.
Specifically, bookmarks should fail when the reader is not supported for the given Feature.
\par Scan
This test verifies that BEViewer can run a bulk_extractor scan.
This test takes a Test Scan Directive as input.
This directive is simply the set of parameters used for a bulk_extractor invocation,
including the word bulk_extractor,
delimited by commas with no whitespace.
\n
This test verifies that bulk_extractor can be invoked correctly.
Validate this test by observing that Feature content is generated.
This test is not intended to be the means for running many bulk_extractor sessions
or validating the feature set generated.
If desired, BEViewer may be modified to serve this purpose by allowing it
to invoke many bulk_extractor runs.
Users may like this feature.
\par Design of the Test Aparatus
The test apparatus runs on its own separate testing thread.
This is required because some tests wait for child threads to complete,
and we cannot ask the Swing thread to wait.
It is also problematic to use the Swing thread for processing
because the Swing thread is required for rendering logs and errors.
Faulting on the Swing thread compromises UI output.
\n
The down side of using a separate thread is that tests aren't quite invoked the same way:
Tests are initiated on a separate thread rather than on the Swing thread.
This may not be an issue.
Logs and Error reporting are thread-safe and, if not running on the Swing thread,
delegate to the Swing thread.
 */


//public class BETests extends Thread {
public class BETests {

  private static final String BOOKMARKS_TEXT = "test_bookmarks_text";

  private BETests() {
  }

  // ************************************************************
  // Test: checkPreferences checks for consistency during clear and reload
  // ************************************************************
  public static void checkPreferences() {
    WLog.log("test: checkPreferences");
    try {
      // save preferences
      BEPreferences.savePreferences();

      // cache preferences
      String originalPreferencesString = BEPreferences.getPreferencesString();

      // clear reports
      BEViewer.closeAllReports();

      // reload preferences
      BEPreferences.loadPreferences();

      // cache reloaded preferences
      String newPreferencesString = BEPreferences.getPreferencesString();

      // compare saved preferences against reloaded preferences
      if (!originalPreferencesString.equals(newPreferencesString)) {
        WLog.log("Preferences mismatch");
        WLog.log("original: " + originalPreferencesString);
        WLog.log("new: " + newPreferencesString);
        WLog.log("Preferences mismatch");
        throw new RuntimeException("checkPreferences error");
      }
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
    WLog.log("test: checkPreferences completed");
  }

  // ************************************************************
  // Test: makeBookmarks
  // ************************************************************
  public static void makeBookmarks(File workSettingsFile, File outputDirectory) {
    WLog.log("test: makeBookmarks");

    // load work settings file
    boolean isSuccessful = BEPreferences.importWorkSettings(workSettingsFile, false);
    if (!isSuccessful) {
      throw new RuntimeException("Test failed: makeBookmarks");
    }

    // generate output bookmark filenames
    File bookmarkOutfile = new File(outputDirectory, BOOKMARKS_TEXT);

    // delete output bookmark filenames
    boolean dummy1 = bookmarkOutfile.delete();

    Thread thread1;
    Thread thread2;

    // run tests using the image reader
    WLog.log("BETests.makeBookmarks");
    thread1 = WManageBookmarks.startThread(bookmarkOutfile);
    waitForThread(thread1);

    // restore preferences
    BEViewer.closeAllReports();
    BEPreferences.loadPreferences();

    WLog.log("test: makeBookmarks completed");
  }

  // ************************************************************
  // Test: perform scan
  // ************************************************************
  public static void performScan(String[] scanCommand) {
    // an implementation of this test would consist of enqueueing multiple
    // scans and then waiting for the queue to become empty again.
    WLog.log("test: performScan");
    WLog.log("Test currently not implemented, se BETests.performScan.");
  }

  // ************************************************************
  // helper functions
  // ************************************************************
  private static void waitForThread(Thread thread) {
    try {
      thread.join();
    } catch (InterruptedException ie) {
      throw new RuntimeException(ie);
    }
  }
}


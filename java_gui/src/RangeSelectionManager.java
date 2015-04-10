import java.awt.datatransfer.ClipboardOwner;
//import java.awt.Toolkit;
import java.awt.datatransfer.Clipboard;
import java.awt.datatransfer.Transferable;
import java.awt.Toolkit;
import java.awt.datatransfer.StringSelection;
import java.util.Observer;

/**
 * The <code>RangeSelectionManager</code> class manages a single range selection
 * offered by a <code>CopyableLineInterface</code> provider
 * and tracks the data provider behind it.
 * Obtain the actual selection using the data provider.
 */
public class RangeSelectionManager {

  // resources
  private final ModelChangedNotifier<Object> selectionManagerChangedNotifier = new ModelChangedNotifier<Object>();
  private static final Clipboard selectionClipboard = Toolkit.getDefaultToolkit().getSystemSelection();
  private static final Clipboard systemClipboard = Toolkit.getDefaultToolkit().getSystemClipboard();

  // state
  private CopyableLineInterface provider = null;
  private int minSelectionIndex = -1;
  private int maxSelectionIndex = -1;

  // the selection as a Transferable object
  private Transferable transferableSelection = new StringSelection("");

  public RangeSelectionManager() {
  }

  /**
   * Selects the provider and range.
   */
  public void setRange(CopyableLineInterface provider,
                          int minSelectionIndex, int maxSelectionIndex) {
    if (provider == this.provider
     && minSelectionIndex == this.minSelectionIndex
     && maxSelectionIndex == this.maxSelectionIndex) {
      // no change
      return;
    }

    // change the selection provider and range
    this.provider = provider;
    this.minSelectionIndex = minSelectionIndex;
    this.maxSelectionIndex = maxSelectionIndex;

    // set the selection as a Transferable object
    transferableSelection = getTransferableSelection();

    // if supported, set the system selection clipboard
    // Note that the system selection clipboard is not the system clipboard.
    setSelectionClipboard(transferableSelection);

    // fire changed
    selectionManagerChangedNotifier.fireModelChanged(null);
  }

  /**
   * Clears any selected provider and range.
   */
  public void clear() {
    setRange(null, -1, -1);
  }

  /**
   * Returns the selected provider or null.
   */
  public CopyableLineInterface getProvider() {
    return provider;
  }

  /**
   * Returns the smallest selected index, inclusive.
   */
  public int getMinSelectionIndex() {
    return minSelectionIndex;
  }

  /**
   * Returns the largest selected index, inclusive.
   */
  public int getMaxSelectionIndex() {
    return maxSelectionIndex;
  }

//  /**
//   * Indicates whether the line of the provider is selected.
//   */
//  public boolean isSelected(CopyableLineInterface provider, int line) {
//    // the provider is the same and the line is in range
//    return (provider == this.provider && line >= minSelectionIndex && line <= maxSelectionIndex);
//  }

  /**
   * Indicates whether the selection manager has an active selection.
   */
  public boolean hasSelection() {
    // a provider is defined and the range is valid
    return (provider != null && minSelectionIndex > -1);
  }

  /**
   * Returns the range as a Transferable object created from strings delimited with newlines
   * suitable for copying to a Clipboard object.
   */
  public Transferable getSelection() {
    return transferableSelection;
  }

  // Returns the range as a Transferable delimited with newlines.
  private Transferable getTransferableSelection() {
    if (provider == null || minSelectionIndex == -1) {
      return new StringSelection("");
    }

    // create a string buffer for containing the selection
    StringBuffer stringBuffer = new StringBuffer();

    if (minSelectionIndex == maxSelectionIndex) {
      // no "\n" appended
      stringBuffer.append(provider.getCopyableLine(minSelectionIndex));
    } else {

      // loop through each selected line to compose the text
      for (int line = minSelectionIndex; line <= maxSelectionIndex; line++) {

        // append the composed line to the selection buffer
        stringBuffer.append(provider.getCopyableLine(line));
        stringBuffer.append("\n");
      }
    }

    String selection = stringBuffer.toString();
    Transferable transferable = new StringSelection(selection);
    return transferable;
  }

  /**
   * Static convenience function for setting the System clipboard with a Transferable object.
   */
  public static void setSystemClipboard(Transferable transferable) {
    // copy the log to the system clipboard
    try {
      // put text onto Clipboard
      systemClipboard.setContents(transferable, null);
    } catch (IllegalStateException exception) {
      WError.showError("Copy Log to System Clipboard failed.",
                       "BEViewer System Clipboard error", exception);
    }
  }

  /**
   * Static convenience function for setting the Selection clipboard with a Transferable object.
   */
  public static void setSelectionClipboard(Transferable transferable) {
    try {
      // put text onto Clipboard
      if (selectionClipboard != null) {
        selectionClipboard.setContents(transferable, null);
      }
    } catch (IllegalStateException exception) {
      WError.showError("Copy Log to Selection Clipboard failed.",
                       "BEViewer Selection Clipboard error", exception);
    }
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param selectionManagerChangedListener the <code>Observer</code> to be added
   */
  public void addRangeSelectionManagerChangedListener(Observer selectionManagerChangedListener) {
    selectionManagerChangedNotifier.addObserver(selectionManagerChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param selectionManagerChangedListener the <code>Observer</code> to be removed
   */
  public void removeRangeSelectionManagerChangedListener(Observer selectionManagerChangedListener) {
    selectionManagerChangedNotifier.deleteObserver(selectionManagerChangedListener);
  }
}


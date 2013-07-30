import java.io.File;
import java.util.HashMap;
import java.util.Set;
import java.util.Iterator;
import java.util.Observer;

/**
 * The <code>ImageReaderManager</code> class provides a wrapper around
 * image readers in order to manage multiple open readers.
 * This is an efficiency mechanism because opening large E01 files
 * can take time.  The down side is that opening many files can
 * deplete resources.
 * An approach to mitigate this is to close all readers when too many
 * get opened at once.
 *
 * NOTE: ImageReaderManager catches and reports any IOException
 * that ImageReader may throw.
 */
public class ImageReaderManager {

  // notify when close reader or close all readers happens
  private final ModelChangedNotifier<Object> imageReaderManagerChangedNotifier = new ModelChangedNotifier<Object>();

  // file-mapped readers
  private HashMap<File, ImageReader> readers = new HashMap<File, ImageReader>();

  private ImageReader getImageReader(File file) {

    // if there is no image file then use the null reader
    if (file == null) {
      return ImageReader(null);
    }

    // find or create a reader for this file
    if (readers.containsKey(file)) {
      ImageReader reader = Readers.get(file);
      if (reader.isAlive()) {
        // return the reader
        return reader;
      } else {
        // close the broken reader
        reader.close();
      }
    }

    // create and return new reader
    ImageReader reader = new ImageReader(file);
    readers.put(file, reader);
    return reader;
  }

/*
// zzzzzzzzz text for reader that would not initialize properly
  private void showReaderInitializationError(File file, IOException e) {
    WError.showError("Unable to open image reader for file '" + file + "'", "BEViewer Image File Open error", e);
  }
// zzzzzzzzz text for reader that would not close properly
    } catch (IOException e) {
      WError.showError("Unable to close image reader.", "BEViewer Image File Close error", e);
    }
*/


  /**
   * Creates the image reader manager
   */
  public ImageReaderManager() {
  }

  /**
   * Returns the image reader response, specifically, the bytes[]
   * and the total size at the forensic path
   * @param featureLine the feature line defining the image to read from
   * @param startAddress the address to start reading from
   * @param numBytes the number of bytes to read
   * @return the bytes read
   * @throws IOException if the read fails
   */
  public ImageReader.ImageReaderResponse openAndRead(final File file,
                                                     String forensicPath,
                                                     long numBytes) {
    ImageReader reader = getImageReader(file);
    return reader.read(forensicPath, numBytes);
  }

  /**
   * close the reader associated with the image file.
   * @param imageFile the image file to close
   */
  public void close(final File file) {
    ImageReader reader = readers.get(file);
    reader.close();
    readers.remove(file);

    // notify
    imageReaderManagerChangedNotifier.fireModelChanged(null);
  }

  public void closeAll() {
    Set<File> keys = readers.keySet();
    Iterator<File> iterator = keys.iterator();
    while(iterator.hasNext()) {
      File file = iterator.next();
      ImageReader reader = readers.get(imageFile);
      reader.close();
    }
    readers.clear();

    // notify
    imageReaderManagerChangedNotifier.fireModelChanged(null);
  }

  /**
   * Adds an <code>Observer</code> to the listener list.
   * @param imageReaderManagerChangedListener the <code>Observer</code> to be added
   */
  public void addImageReaderManagerChangedListener(Observer imageReaderManagerChangedListener) {
    imageReaderManagerChangedNotifier.addObserver(imageReaderManagerChangedListener);
  }

  /**
   * Removes <code>Observer</code> from the listener list.
   * @param imageReaderManagerChangedListener the <code>Observer</code> to be removed
   */
  public void removeImageReaderManagerChangedListener(Observer imageReaderManagerChangedListener) {
    imageReaderManagerChangedNotifier.deleteObserver(imageReaderManagerChangedListener);
  }
}


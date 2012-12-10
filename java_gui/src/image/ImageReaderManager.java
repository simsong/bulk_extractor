import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Set;
import java.util.Iterator;
import java.util.Observer;

/**
 * The <code>ImageReaderManager</code> class provides path reading services
 * based on the ImageLine provided and the image reader type selected.
 * Because Java readers do not support compressed paths,
 * all compressed paths are delegated to the bulk_extractor reader.
 * Fires image reader change events so that the UI selection menu can remain current.
 * NOTE: by design, ImageReaderManager catches and reports IOException
 * and all lower layers may throw IOException.
 */
public class ImageReaderManager {

  // signal resource
  private final ModelChangedNotifier<Object> imageReaderManagerChangedNotifier = new ModelChangedNotifier<Object>();

  private HashMap<File, ImageReaderInterface> rawReaders = new HashMap<File, ImageReaderInterface>();
  private HashMap<File, ImageReaderInterface> multipartReaders = new HashMap<File, ImageReaderInterface>();
  private HashMap<File, ImageReaderInterface> e01Readers = new HashMap<File, ImageReaderInterface>();
  private HashMap<File, ImageReaderInterface> affReaders = new HashMap<File, ImageReaderInterface>();
  private HashMap<File, ImageReaderInterface> bulkExtractorReaders = new HashMap<File, ImageReaderInterface>();
  private HashMap<File, ImageReaderInterface> libewfcsTesterReaders = new HashMap<File, ImageReaderInterface>();
  private NoReader noReader = new NoReader();
  private FailReader failReader = new FailReader();

  private ImageReaderType selectedReaderType = ImageReaderType.OPTIMAL;

  private ImageReaderInterface getImageReader(FeatureLine featureLine) {

    // if there is no image file then use the null reader
    if (featureLine == null || featureLine.getImageFile() == null) {
      return noReader;
    }

    // identify the file associated with the feature line
    File file = featureLine.getImageFile();


    // if any reader is allowed, pick the optimal reader
    if (selectedReaderType == ImageReaderType.OPTIMAL) {

      // only BULK_EXTRACTOR can read compressed path lines
      if (featureLine.getType() == FeatureLine.FeatureLineType.PATH_LINE) {
        // return a bulk_extractor reader for all compressed path lines
        return getBulkExtractorReader(file);
      } else {
        return getOptimalReader(featureLine);
      }

    } else if (selectedReaderType == ImageReaderType.JAVA_ONLY) {
      return getJavaReader(file);
    } else if (selectedReaderType == ImageReaderType.BULK_EXTRACTOR) {
      return getBulkExtractorReader(file);
    } else if (selectedReaderType == ImageReaderType.LIBEWFCS_TESTER) {
      return getLibewfcsTesterReader(file);
    } else {
      throw new RuntimeException("implementation error");
    }
  }

  private void showReaderInitializationError(File file, IOException e) {
    WError.showError("Unable to open image reader for file '" + file + "'", "BEViewer Image File Open error", e);
  }

  private ImageReaderInterface getRawReader(File file) {
    if (rawReaders.containsKey(file)) {
      return rawReaders.get(file);
    } else {
      ImageReaderInterface reader;
      try {
        reader = new JavaReader(new RawFileReader(file));
      } catch (IOException e) {
        showReaderInitializationError(file, e);
        reader = failReader;
      }
      rawReaders.put(file, reader);
      return reader;
    }
  }
  private ImageReaderInterface getMultipartReader(File file) {
    if (multipartReaders.containsKey(file)) {
      return multipartReaders.get(file);
    } else {
      ImageReaderInterface reader;
      try {
        reader = new JavaReader(new MultipartFileReader(file));
      } catch (IOException e) {
        showReaderInitializationError(file, e);
        reader = failReader;
      }
      multipartReaders.put(file, reader);
      return reader;
    }
  }
  private ImageReaderInterface getE01Reader(File file) {
    if (e01Readers.containsKey(file)) {
      return e01Readers.get(file);
    } else {
      ImageReaderInterface reader;
      try {
        reader = new JavaReader(new E01FileReader(file));
      } catch (IOException e) {
        showReaderInitializationError(file, e);
        reader = failReader;
      }
      e01Readers.put(file, reader);
      return reader;
    }
  }
  private ImageReaderInterface getAFFReader(File file) {
    if (affReaders.containsKey(file)) {
      return affReaders.get(file);
    } else {
      ImageReaderInterface reader;
      try {

// NOTE: there is no Java AFF reader, so use bulk_extractor
//        reader = new AFFReader(file);
        reader = new BulkExtractorReader(file);
      } catch (IOException e) {
        showReaderInitializationError(file, e);
        reader = failReader;
      }
      affReaders.put(file, reader);
      return reader;
    }
  }
  private ImageReaderInterface getBulkExtractorReader(File file) {
    if (bulkExtractorReaders.containsKey(file)) {
      return bulkExtractorReaders.get(file);
    } else {
      ImageReaderInterface reader;
      try {
        reader = new BulkExtractorReader(file);
      } catch (IOException e) {
        showReaderInitializationError(file, e);
        reader = failReader;
      }
      bulkExtractorReaders.put(file, reader);
      return reader;
    }
  }
  private ImageReaderInterface getLibewfcsTesterReader(File file) {
    if (libewfcsTesterReaders.containsKey(file)) {
      return libewfcsTesterReaders.get(file);
    } else {
      ImageReaderInterface reader;
      try {
        reader = new LibewfcsTesterReader(file);
      } catch (IOException e) {
        showReaderInitializationError(file, e);
        reader = failReader;
      }
      libewfcsTesterReaders.put(file, reader);
      return reader;
    }
  }

  private ImageReaderInterface getOptimalReader(FeatureLine featureLine) {

    File file = featureLine.getImageFile();

    // only BULK_EXTRACTOR can read compressed path lines
    if (featureLine.getType() == FeatureLine.FeatureLineType.PATH_LINE) {
      // return a bulk_extractor reader for all compressed path lines
      return getBulkExtractorReader(file);
    }

    // delegate based on the file type
    ImageFileType imageFileType = ImageFileType.getImageFileType(file);
    if (imageFileType == ImageFileType.RAW) {
      return getRawReader(file);
    } else if (imageFileType == ImageFileType.MULTIPART) {
      return getMultipartReader(file);
    } else if (imageFileType == ImageFileType.E01) {
      return getE01Reader(file);
    } else if (imageFileType == ImageFileType.AFF) {
      return getAFFReader(file);
    } else {
      throw new RuntimeException("missing type");
    }
  }

  private ImageReaderInterface getJavaReader(File file) {
    // return the Java reader that reads this file
    ImageFileType imageFileType = ImageFileType.getImageFileType(file);
    if (imageFileType == ImageFileType.RAW) {
      return getRawReader(file);
    } else if (imageFileType == ImageFileType.MULTIPART) {
      return getMultipartReader(file);
    } else if (imageFileType == ImageFileType.E01) {
      return getE01Reader(file);
    } else if (imageFileType == ImageFileType.AFF) {
      WError.showError("The Java AFF Image reader is not supported."
                       + "\nPlease select a valid reader for this Image.",
                       "BEViewer Image File Open error", null);
      // note that Java does not currently have an AFF reader
      return failReader;
    } else {
      throw new RuntimeException("missing type");
    }
  }

  /**
   * Creates the image reader manager
   */
  public ImageReaderManager() {
  }

  /**
   * Allows specific control over the reader to use.
   * @param readerType the reader type to use
   */
  public void setReaderTypeAllowed(ImageReaderType readerType) {
    selectedReaderType = readerType;

    // notify
    imageReaderManagerChangedNotifier.fireModelChanged(null);
  }

  /**
   * indicates the reader type being used.
   * @return the reader type
   */
  public ImageReaderType getReaderTypeAllowed() {
    return selectedReaderType;
  }

  /**
   * Returns the requested bytes or less if request passes EOF.
   * @param featureLine the feature line defining the image to read from
   * @param startAddress the address to start reading from
   * @param numBytes the number of bytes to read
   * @return the bytes read
   * @throws IOException if the read fails
   */
  public byte[] readImageBytes(final FeatureLine featureLine,
                               final long startAddress, final int numBytes)
                               throws IOException {

    // permit zero-byte read of null feature line
    if (startAddress == 0 && numBytes == 0) {
      return new byte[0];
    }

    // get the correct image reader
    ImageReaderInterface reader = getImageReader(featureLine);

    // read the bytes from the path
    byte[] bytes = reader.readPathBytes(featureLine, startAddress, numBytes);

    return bytes;
  }

  /**
   * Returns the size of the image identified by the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the size of the image identified by the feature line.
   * @throws IOException if the file access fails
   */
  public long getImageSize(final FeatureLine featureLine) throws IOException {

    // allow for no feature line
    if (featureLine == null) {
      return 0;

    } else {

      // get the correct image reader
      ImageReaderInterface reader = getImageReader(featureLine);

      return reader.getPathSize(featureLine);
    }
  }

  /**
   * Returns the metadata associated with the feature line.
   * @param featureLine the feature line defining the page to be read
   * @return the metadata associated with the feature line
   * @throws IOException if the file access fails
   */
  public String getImageMetadata(final FeatureLine featureLine) throws IOException {

    // allow for no feature line
    if (featureLine == null) {
      return "A feature must be selected in order to obtain its associated metadata.";

    } else {

      // get the correct image reader
      ImageReaderInterface reader = getImageReader(featureLine);

      return reader.getPathMetadata(featureLine);
    }
  }

  /**
   * close the reader associated with the image file.
   * @param imageFile the image file to close
   */
  public void closeReader(final File imageFile) {

    tryCloseReader(rawReaders, imageFile);
    tryCloseReader(multipartReaders, imageFile);
    tryCloseReader(e01Readers, imageFile);
    tryCloseReader(affReaders, imageFile);
    tryCloseReader(bulkExtractorReaders, imageFile);
    tryCloseReader(libewfcsTesterReaders, imageFile);

    // notify
    imageReaderManagerChangedNotifier.fireModelChanged(null);
  }

  private void tryCloseReader(HashMap<File, ImageReaderInterface> readers, File imageFile) {
    ImageReaderInterface reader = readers.get(imageFile);
    if (reader != null) {
      try {
        reader.close();
        readers.remove(imageFile);
      } catch (IOException e) {
        WError.showError("Unable to close image reader.", "BEViewer Image File Close error", e);
      }
    }
  }

  /**
   * close all readers.
   */
  public void closeAllReaders() {
    closeGroup(rawReaders);
    closeGroup(multipartReaders);
    closeGroup(e01Readers);
    closeGroup(affReaders);
    closeGroup(bulkExtractorReaders);
    closeGroup(libewfcsTesterReaders);

    // notify
    imageReaderManagerChangedNotifier.fireModelChanged(null);
  }

  private void closeGroup(HashMap<File, ImageReaderInterface> map) {
    Set<File> keys = map.keySet();
    Iterator<File> iterator = keys.iterator();
    while(iterator.hasNext()) {
      File file = iterator.next();
      ImageReaderInterface reader = map.get(file);
      tryCloseReader(map, file);
    }
    map.clear();
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


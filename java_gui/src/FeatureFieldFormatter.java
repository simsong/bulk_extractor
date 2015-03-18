import java.util.Vector;
import java.io.File;
import java.io.StringReader;
import java.io.ByteArrayInputStream;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.DocumentBuilder;
//import javax.xml.parsers.ParserConfigurationException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.w3c.dom.Node;
//import org.xml.sax.SAXException;

/**
 * The <code>FeatureFieldFormatter</code> formats a Feature's feature field
 * into a more presentable formatted form.
 * NOTE: returned values are truncated to avoid Swing hanging when working with large strings.
 */
public class FeatureFieldFormatter {
  public static final int MAX_CHAR_WIDTH = 1000;

  // for parsing input such as exif which is formatted in XML
  private static final DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();

  private FeatureFieldFormatter() {
  }

  /**
   * Provides feature text in a format suitable for reading.
   */
  public static String getFormattedFeatureText(File featuresFile, byte[] featureField, byte[] contextField) {
    String filename = featuresFile.getName();
    String formattedText;

    if (filename.equals("elf.txt")) {
      formattedText = getELFFormat(contextField);

    } else if (filename.equals("ether.txt")) {
      formattedText = getEtherFormat(featureField, contextField);

    } else if (filename.equals("exif.txt")) {
      formattedText = getEXIFFormat(contextField);

    } else if (filename.equals("gps.txt")) {
      formattedText = getGPSFormat(contextField);

    } else if (filename.equals("ip.txt")) {
      // feature field, which contains IP, plus context, which contains
      // IP metadata
      formattedText = getGenericFormatWithContext(featureField, contextField);

//      case "json.txt":
//      case "tcp.txt":

    } else if (filename.equals("winpe.txt")) {
      formattedText = getWINPEFormat(contextField);

    } else if (filename.indexOf("identified") >= 0) {
      // accept any filename starting with "identified_blocks"
      // feature field, which contains hexdigest plus context which indicates
      // information about the feature
      formattedText = getGenericFormatWithContext(featureField, contextField);

    } else {
      formattedText = getGenericFormat(featureField);
    }

    // NOTE: Swing bug: Swing fails when attemtping to render long strings,
    // so truncate long text to MAX_CHAR_WIDTH characters.
    if (formattedText.length() > MAX_CHAR_WIDTH) {
      // truncate and append ellipses ("...")
      formattedText = formattedText.substring(0, MAX_CHAR_WIDTH) + "\u2026";
    }
//WLog.log("FeatureFieldFormatter formattedText: '" + formattedText + "'");
    return formattedText;
  }

  // note: XML parsed output would look better.
  private static String getELFFormat(byte[] contextField) {
    byte[] elfContextField = UTF8Tools.unescapeEscape(contextField);
    return new String(elfContextField);
  }
  
  private static String getEtherFormat(byte[] featureField, byte[] contextField) {
    // feature field, which contains MAC, plus context, which indicates
    // the ethernet traffic direction
    return new String(featureField) + " " + new String(contextField);
  }
  
  private static String getEXIFFormat(byte[] contextField) {
    StringBuffer stringBuffer = new StringBuffer();

    // provide values as "<key>=<value>"
    try {
      final DocumentBuilder builder = builderFactory.newDocumentBuilder();

      // get string into document
      byte[] exifContextField = UTF8Tools.unescapeEscape(contextField);
      Document document = builder.parse(new ByteArrayInputStream(exifContextField));

      Element rootElement = document.getDocumentElement();
      NodeList nodeList = rootElement.getChildNodes();
      int length = nodeList.getLength();
      for (int i=0; i<length; i++) {
        Node node = nodeList.item(i);
        if (node.getFirstChild() != null) {
          String key = node.getNodeName();
          String value = node.getFirstChild().getNodeValue();

          // shorten the key by removing suffix before "."
          if (key == null) {
            key = "not defined";
          }
          int last = key.lastIndexOf('.');
          if (last > 0) {
            key = key.substring(last + 1);
          }

          // append the key and value
          stringBuffer.append(key);
          stringBuffer.append("=");
          stringBuffer.append(value);
          if (i != length - 1) {
            stringBuffer.append(", ");
          }
        }
      }
    } catch (Exception e) {
      // let it go.
      stringBuffer.append("<EXIF parser failed, please see log>");
      WLog.log("FeatureFieldFormatter.getEXIFFormat parser failure:");
      WLog.logThrowable(e);
      WLog.logBytes("on unescaped byte sequence", UTF8Tools.unescapeBytes(contextField));
      WLog.logBytes("From escaped byte sequence", contextField);
    }

    return stringBuffer.toString();
  }

  private static String getGPSFormat(byte[] contextField) {
    // show GPS data as-is, encoded in the context field
    return new String(contextField);
  }
  
  // note: XML parsed output would look better.
  private static String getWINPEFormat(byte[] contextField) {
    byte[] winPEContextField = UTF8Tools.unescapeEscape(contextField);
    return new String(winPEContextField);
  }
  
  private static String getGenericFormat(byte[] featureField) {
    // keep string pretty much as is
    byte[] escapedBytes = featureField;

    // if probable utf16, strip all NULLs, right or not.
    if (UTF8Tools.escapedLooksLikeUTF16(escapedBytes)) {
      escapedBytes = UTF8Tools.stripNulls(escapedBytes);
    }

    // unescape any escaped escape character so it looks better
    escapedBytes = UTF8Tools.unescapeEscape(escapedBytes);

    // use this as text
    return new String(escapedBytes, UTF8Tools.UTF_8);
  }

  private static String getGenericFormatWithContext(byte[] featureField, byte[] contextField) {
    return new String(featureField) + " " + new String(contextField);
  }
  
  // ************************************************************
  // Image highlight text
  // ************************************************************

  // possible EXIF headers
  private static final byte[] EXIF_0 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe0};
  private static final byte[] EXIF_1 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe1};
  private static final byte[] EXIF_2 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe2};
  private static final byte[] EXIF_3 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe3};
  private static final byte[] EXIF_4 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe4};
  private static final byte[] EXIF_5 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe5};
  private static final byte[] EXIF_6 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe6};
  private static final byte[] EXIF_7 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe7};
  private static final byte[] EXIF_8 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe8};
  private static final byte[] EXIF_9 = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe9};
  private static final byte[] EXIF_A = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xea};
  private static final byte[] EXIF_B = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xeb};
  private static final byte[] EXIF_C = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xec};
  private static final byte[] EXIF_D = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xed};
  private static final byte[] EXIF_E = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xee};
  private static final byte[] EXIF_F = {(byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xef};

  // photoshop PSD header
  private static final byte[] PSD = {'8', 'B', 'P', 'S', 0x00, 0x01};

  // TIFF marker
  private static final byte[] TIFF_MM = {'M', 'M', 0, 42};
  private static final byte[] TIFF_II = {'I', 'I', 42, 0};

  // EXIF, PSD, and TIFF define byte[][] EXIF
  private static final Vector<byte[]> EXIF = new Vector<byte[]>();
  static {
    EXIF.add(EXIF_0); EXIF.add(EXIF_1); EXIF.add(EXIF_2); EXIF.add(EXIF_3);
    EXIF.add(EXIF_4); EXIF.add(EXIF_5); EXIF.add(EXIF_6); EXIF.add(EXIF_7);
    EXIF.add(EXIF_8); EXIF.add(EXIF_9); EXIF.add(EXIF_A); EXIF.add(EXIF_B);
    EXIF.add(EXIF_C); EXIF.add(EXIF_D); EXIF.add(EXIF_E); EXIF.add(EXIF_F);
    EXIF.add(EXIF_C); EXIF.add(EXIF_D); EXIF.add(EXIF_E); EXIF.add(EXIF_F);
    EXIF.add(PSD); EXIF.add(TIFF_MM); EXIF.add(TIFF_II);
  }

  // ELF
  private static final byte[] ELF_MAGIC_NUMBER = {(byte)0x7f, 'E', 'L', 'F'};
  private static final Vector<byte[]> ELF = new Vector<byte[]>();
  static {
    ELF.add(ELF_MAGIC_NUMBER);
  }

  private static final byte[] WINPE_MAGIC_NUMBER = {'P', 'E', (byte)0, (byte)0};
  private static final Vector<byte[]> WINPE = new Vector<byte[]>();
  static {
    WINPE.add(WINPE_MAGIC_NUMBER);
  }

  /**
   * Provides a vector of possible matching features as they would appear in the Image.
   */
  public static Vector<byte[]> getImageHighlightVector(ImageModel.ImagePage imagePage) {
    if (imagePage.featureLine.featuresFile == null) {
      return new Vector<byte[]>();
    }

    String filename = imagePage.featureLine.featuresFile.getName();

    if (filename.equals("gps.txt")) {
      return EXIF;
    } else if (filename.equals("exif.txt")) {
      return EXIF;
    } else if (filename.equals("ip.txt")) {
      return getIPImageFormat(imagePage);
    } else if (filename.equals("tcp.txt")) {
      return getTCPImageFormat(imagePage);
    } else if (filename.equals("elf.txt")) {
      return ELF;
    } else if (filename.equals("winpe.txt")) {
      return WINPE;

    } else {
      // for all else, add various UTF encodings of the feature field
      Vector<byte[]> textVector = new Vector<byte[]>();

      byte[] utf8Feature;
      byte[] utf16Feature;
      // for all else, add UTF8 and UTF16 encodings based on the feature field
      if (UTF8Tools.escapedLooksLikeUTF16(imagePage.featureLine.featureField)) {
        // calculate UTF8 and set UTF16
        utf8Feature = UTF8Tools.utf16To8Basic(imagePage.featureLine.featureField);
        utf16Feature = imagePage.featureLine.featureField;
      } else {
        // set UTF8 and calculate UTF16
        utf8Feature = imagePage.featureLine.featureField;
        utf16Feature = UTF8Tools.utf8To16Basic(imagePage.featureLine.featureField);
      }

      // now add the two filters
      textVector.add(utf8Feature);
      textVector.add(utf16Feature);

      return textVector;
    }
  }

  private static byte[] getIPBytes(String ipString) {
    // returns byte[] from String containing D.D.D.D

    // strip off any port number
    int portMarkerIndex = ipString.lastIndexOf(':');
    if (portMarkerIndex >= 0) {
      ipString = ipString.substring(0, portMarkerIndex);
    }

    // get the four IP values as an array of four strings
    String[] ipArray = ipString.split("\\.");

    // fail if there are not four values
    if (ipArray.length != 4) {
      return null;
    }

    // try to put the four values into the byte array
    byte[] ipBytes = new byte[4];
    try {
      for (int i=0; i<4; i++) {
        int ipPart = Integer.parseInt(ipArray[i]);
        if (ipPart < 0 || ipPart > 255) {
          // the number needs to be valid
          return null;
        }
        ipBytes[i] = (byte)(ipPart);
      }
    } catch (NumberFormatException e) {
      return null;
    }

    // return the correctly parsed byte array of 4 IP values
    return ipBytes;
  }


  private static Vector<byte[]> getIPImageFormat(ImageModel.ImagePage imagePage) {
    // create the text vector to be returned
    Vector<byte[]> textVector = new Vector<byte[]>();

    // get the feature text which should contain IP as "D.D.D.D" where D is a decimal
    String featureString = new String(imagePage.featureLine.featureField);

    // get the IP bytes directly from the feature string
    byte[] ipBytes = getIPBytes(featureString);
    if (ipBytes == null) {
      // fail
      WLog.log("FeatureFieldFormatter.getIPImageFormat: Invalid IP in feature: '"
               + featureString + "'");
    } else {
      // add to Vector
      textVector.add(ipBytes);
    }

    return textVector;
  }

  private static Vector<byte[]> getTCPImageFormat(ImageModel.ImagePage imagePage) {
    // create the text vector to be returned
    Vector<byte[]> textVector = new Vector<byte[]>();

    // get the feature text which should contain source and destination IPs
    String featureString = new String(imagePage.featureLine.featureField);

    // split the feature string into parts, source D.D.D.D and destination D.D.D.D
    String[] featureFieldArray = featureString.split("\\ ");
    if (featureFieldArray.length < 3) {
      // fail
      WLog.log("FeatureFieldFormatter.getIPImageFormat: Invalid TCP in feature: '"
               + featureString + "'");
      return textVector;
    }

    // add source IP
    byte[] sourceIPBytes = getIPBytes(featureFieldArray[0]);
    if (sourceIPBytes == null) {
      // fail
      WLog.log("FeatureFieldFormatter.getIPImageFormat: Invalid source TCP in feature: '"
               + featureString + "'");
    } else {
      // add to Vector
      textVector.add(sourceIPBytes);
    }

    // add destination IP
    byte[] destinationIPBytes = getIPBytes(featureFieldArray[2]);
    if (destinationIPBytes == null) {
      // fail
      WLog.log("FeatureFieldFormatter.getIPImageFormat: Invalid destination TCP in feature: '"
               + featureString + "'");
    } else {
      // add to Vector
      textVector.add(destinationIPBytes);
    }

    return textVector;
  }
}


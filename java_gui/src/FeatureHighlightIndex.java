import java.util.Vector;
import java.util.Enumeration;

/**
 * A highlight index, composed of begin index and length.
 */
public class FeatureHighlightIndex {
  public final int beginIndex;
  public final int length;
  public FeatureHighlightIndex(int beginIndex, int length) {
    this.beginIndex = beginIndex;
    this.length = length;
  }


  /**
   * obtain character highlight indexes for character matches
   * in the selected feaature line and in user's text.
   */
  public static Vector<FeatureHighlightIndex> getHighlightIndexes(char[] featureChars) {

    Vector<FeatureHighlightIndex> highlightIndexes = new Vector<FeatureHighlightIndex>();

    // if the feature line matches the selected feature line, highlight the match
    final FeatureLine highlightFeatureLine = BEViewer.featureLineSelectionManager.getFeatureLineSelection();
    if (highlightFeatureLine != null) {
      String highlightText = highlightFeatureLine.formattedFeature;
      char[] highlightChars = highlightText.toCharArray();
      addHighlights(highlightIndexes, featureChars, highlightChars);
    }

    // add any highlights that match user's highlighting from the highlight model
    Vector<byte[]> highlightArray = BEViewer.userHighlightModel.getUserHighlightByteVector();
//WLog.log("FHI.prefor");
    for (Enumeration<byte[]> e = highlightArray.elements(); e.hasMoreElements(); ) {
      byte[] highlightBytes = UTF8Tools.unescapeBytes(e.nextElement());
//WLog.log("FHI.bytes: " + new String(highlightBytes));
//WLog.log("FHI.unescaped bytes: " + new String(UTF8Tools.unescapeBytes(highlightBytes)));
      char[] highlightChars = new String(highlightBytes).toCharArray();
      addHighlights(highlightIndexes, featureChars, highlightChars);
    }

    return highlightIndexes;
  }

  // add highlight indexes where highlightChars are in featureChars
  private static void addHighlights(Vector<FeatureHighlightIndex> highlightIndexes,
                        char[] featureChars, char[] highlightChars) {

    if (highlightChars.length == 0) {
      // do not generate 0-length highlight indexes
      return;
    }

    final boolean highlightMatchCase = BEViewer.userHighlightModel.isHighlightMatchCase();
    final int lastBeginIndex = featureChars.length - highlightChars.length;

    // scan featureChars for matching highlightChars
    for (int i=0; i<=lastBeginIndex; i++) {
      boolean match = true;
      for (int j=0; j<highlightChars.length; j++) {
        char featureChar = featureChars[i + j];
        char highlightChar = highlightChars[j];

        // manage case
        if (!highlightMatchCase) {
          if (featureChar >= 0x41 && featureChar <= 0x5a) {
            // change upper case to lower case
            featureChar += 0x20;
          }
          if (highlightChar >= 0x41 && highlightChar <= 0x5a) {
            // change upper case to lower case
            highlightChar += 0x20;
          }
        }

        // check for match between chars in position
        if (featureChar != highlightChar) {
          match = false;
          break;
        }
      }
      if (match) {
        highlightIndexes.add(new FeatureHighlightIndex(i, highlightChars.length));
      }
    }
  }
}


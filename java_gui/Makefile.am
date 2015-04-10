# NOTE: To avoid complications relating to Java, Makefile.am intentionally
# does not define a BEViewer primary or define a Java compiler.
# As a result, autoconf does not know what files are sources,
# what files go into the distribution, or what files to be installed.
# To compensate, source files are prefixed with $(srcdir)
# and are included manually into EXTRA_DIST and install and uninstall hooks are hardcoded.

BE_VIEWER_JAR = BEViewer.jar

# srcdir is specified because Makefile.am does not follow the _PROGRAMS primary convention,
# see NOTE above.
# Note: BEViewer intentionally surrounds Basic*UI.java for javac.
SOURCE_FILES = \
$(srcdir)/src/BEViewer.java \
$(srcdir)/src/BasicFeaturesUI.java \
$(srcdir)/src/BasicImageUI.java \
$(srcdir)/src/BEViewer.java \
$(srcdir)/src/BEArgsParser.java \
$(srcdir)/src/BEIcons.java \
$(srcdir)/src/BEMenus.java \
$(srcdir)/src/BEPreferences.java \
$(srcdir)/src/BETests.java \
$(srcdir)/src/BEToolbar.java \
$(srcdir)/src/BookmarksModel.java \
$(srcdir)/src/ClassificationLabel.java \
$(srcdir)/src/ClassificationManager.java \
$(srcdir)/src/Config.java \
$(srcdir)/src/CopyableLineInterface.java \
$(srcdir)/src/FeatureFieldFormatter.java \
$(srcdir)/src/FeatureHighlightIndex.java \
$(srcdir)/src/FeatureLine.java \
$(srcdir)/src/FeatureLineSelectionManager.java \
$(srcdir)/src/FeatureLineTable.java \
$(srcdir)/src/FeatureListCellRenderer.java \
$(srcdir)/src/FeatureRangePrinter.java \
$(srcdir)/src/FeaturesComponent.java \
$(srcdir)/src/FeaturesModel.java \
$(srcdir)/src/FeaturesPane.java \
$(srcdir)/src/FeaturesParserThread.java \
$(srcdir)/src/FeaturesUI.java \
$(srcdir)/src/FileChooserButton.java \
$(srcdir)/src/FileComponent.java \
$(srcdir)/src/FileTools.java \
$(srcdir)/src/ForensicPath.java \
$(srcdir)/src/ImageComponent.java \
$(srcdir)/src/ImageFileType.java \
$(srcdir)/src/ImageHighlightProducer.java \
$(srcdir)/src/ImageLine.java \
$(srcdir)/src/ImageModel.java \
$(srcdir)/src/ImageRangePrinter.java \
$(srcdir)/src/ImageReaderThread.java \
$(srcdir)/src/ImageSourceType.java \
$(srcdir)/src/ImageUI.java \
$(srcdir)/src/ImageView.java \
$(srcdir)/src/ModelChangedNotifier.java \
$(srcdir)/src/NavigationPane.java \
$(srcdir)/src/RangeSelectionManager.java \
$(srcdir)/src/ReportFileFilter.java \
$(srcdir)/src/ReportSelectionManager.java \
$(srcdir)/src/ReportsModel.java \
$(srcdir)/src/ReportsPane.java \
$(srcdir)/src/ReportsTreeCellRenderer.java \
$(srcdir)/src/ScanSettings.java \
$(srcdir)/src/ScanSettingsConsumer.java \
$(srcdir)/src/ScanSettingsListCellRenderer.java \
$(srcdir)/src/ScanSettingsListModel.java \
$(srcdir)/src/ScanSettingsToolBar.java \
$(srcdir)/src/TextComponent.java \
$(srcdir)/src/ThreadReaderModel.java \
$(srcdir)/src/ThreadsafeTreeModelManager.java \
$(srcdir)/src/UserHighlightModel.java \
$(srcdir)/src/UTF8Tools.java \
$(srcdir)/src/WAbout.java \
$(srcdir)/src/WError.java \
$(srcdir)/src/WExportWorkSettings.java \
$(srcdir)/src/WImportScanSettings.java \
$(srcdir)/src/WImportWorkSettings.java \
$(srcdir)/src/WIndeterminateProgress.java \
$(srcdir)/src/WInfo.java \
$(srcdir)/src/WLog.java \
$(srcdir)/src/WManageBookmarks.java \
$(srcdir)/src/WOpen.java \
$(srcdir)/src/WProgress.java \
$(srcdir)/src/WRawDeviceChooser.java \
$(srcdir)/src/WScanBoxedControls.java \
$(srcdir)/src/WScanBoxedDebugging.java \
$(srcdir)/src/WScanBoxedGeneral.java \
$(srcdir)/src/WScanBoxedParallelizing.java \
$(srcdir)/src/WScanBoxedRequired.java \
$(srcdir)/src/WScanBoxedScanners.java \
$(srcdir)/src/WScanBoxedTuning.java \
$(srcdir)/src/WScan.java \
$(srcdir)/src/WScanProgress.java \
$(srcdir)/src/WScanSettingsRunQueue.java \
$(srcdir)/src/WTest.java \
$(srcdir)/src/WURL.java \
$(srcdir)/src/image/BulkExtractorScanListReader.java \
$(srcdir)/src/image/BulkExtractorVersionReader.java \
$(srcdir)/src/image/ImageReader.java \
$(srcdir)/src/image/ThreadAborterTimer.java

ICON_FILES = \
$(srcdir)/icons/16/bookmark-new-2.png $(srcdir)/icons/24/bookmark-new-2.png \
$(srcdir)/icons/16/bookmark-2.png $(srcdir)/icons/24/bookmark-2.png \
$(srcdir)/icons/16/archive-extract-3.png $(srcdir)/icons/24/archive-extract-3.png \
$(srcdir)/icons/16/edit-delete-6.png $(srcdir)/icons/24/edit-delete-6.png \
$(srcdir)/icons/16/edit-redo-8.png $(srcdir)/icons/24/edit-redo-8.png \
$(srcdir)/icons/16/go-up-4.png $(srcdir)/icons/24/go-up-4.png \
$(srcdir)/icons/16/go-down-4.png $(srcdir)/icons/24/go-down-4.png \
$(srcdir)/icons/16/go-previous-4.png $(srcdir)/icons/24/go-previous-4.png \
$(srcdir)/icons/16/go-next-4.png $(srcdir)/icons/24/go-next-4.png \
$(srcdir)/icons/16/go-home-4.png $(srcdir)/icons/24/go-home-4.png \
$(srcdir)/icons/16/document-open-7.png $(srcdir)/icons/24/document-open-7.png \
$(srcdir)/icons/16/edit-copy-3.png $(srcdir)/icons/24/edit-copy-3.png \
$(srcdir)/icons/16/run-build-install.png $(srcdir)/icons/24/run-build-install.png \
$(srcdir)/icons/16/document-print.png $(srcdir)/icons/24/document-print.png \
$(srcdir)/icons/16/window-close.png $(srcdir)/icons/24/window-close.png \
$(srcdir)/icons/16/help-contents.png $(srcdir)/icons/24/help-contents.png \
$(srcdir)/icons/16/help-about.png $(srcdir)/icons/24/help-about.png \
$(srcdir)/icons/16/application-exit.png $(srcdir)/icons/24/application-exit.png

MANIFEST_FILE = $(srcdir)/BEViewer_manifest.txt

LAUNCHER_FILE = $(srcdir)/BEViewer

EXTRA_DIST = $(SOURCE_FILES) $(ICON_FILES) $(MANIFEST_FILE) $(LAUNCHER_FILE) doc/doxygen

DOXYGEN_PDF = doc/doxygen/latex/refman.pdf

all: $(BE_VIEWER_JAR)

# compile BEViewer to classes, extract jars to classes,
# then jar together all classes and other requisite files
# NOTE: for now, compile to Java6 until win, mac, and Linux people are all ready for Java7
$(BE_VIEWER_JAR): $(SOURCE_FILES)
	rm -rf classes
	mkdir -p classes
	echo $(SOURCE_FILES) > temp_generated_source_list
	javac -source 1.6 -target 1.6 -Xlint:unchecked -d classes -classpath @temp_generated_source_list
	rm -f temp_generated_source_list
	echo $(ICON_FILES) | sed s:$(srcdir)/:\ : > temp_generated_icon_list
	jar cfm $(BE_VIEWER_JAR) $(MANIFEST_FILE) -C $(srcdir) @temp_generated_icon_list -C classes .
	rm temp_generated_icon_list

doxygen: $(DOXYGEN_PDF)

$(DOXYGEN_PDF): $(SOURCE_FILES)
	cd doc/doxygen; doxygen Doxyfile
	cd doc/doxygen/latex; make

javadocs: $(SOURCE_FILES)
	rm -rf javadocs
	javadoc -d javadocs $(SOURCE_FILES)

#old	javadoc -classpath $(LOG4J_JAR) -d javadocs $(SOURCE_FILES)

clean-local:
	rm -rf classes
	rm -f $(BE_VIEWER_JAR)
	rm -rf javadocs
	rm -rf doc/doxygen/html

install-exec-hook:
	echo " ( cp '$(BE_VIEWER_JAR)' '$(DESTDIR)$(bindir)' )"; \
	cp -p $(BE_VIEWER_JAR) $(DESTDIR)$(bindir)
	echo " ( install '$(LAUNCHER_FILE)' '$(DESTDIR)$(bindir)'/BEViewer )"; \
	install $(LAUNCHER_FILE) $(DESTDIR)$(bindir)/BEViewer

uninstall-hook:
	echo " ( cd '$(DESTDIR)$(bindir)' && rm -f" $(BE_VIEWER_JAR) ")"; \
	cd $(DESTDIR)$(bindir) && rm -f $(BE_VIEWER_JAR)
	echo " ( cd '$(DESTDIR)$(bindir)' && rm -f BEViewer )"; \
	cd $(DESTDIR)$(bindir) && rm -f BEViewer

# convenience rule useful during development
run: $(BE_VIEWER_JAR) 
	java -Xmx1g -jar $(BE_VIEWER_JAR)

.PHONY: doxygen run


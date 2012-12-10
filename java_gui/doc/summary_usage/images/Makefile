
images = BEViewer_example.pdf BEViewer_blank_annotated.pdf

all: $(images)

BEViewer_example.pdf: BEViewer_example.png
	inkscape --without-gui --export-pdf=$@ $<

BEViewer_blank_annotated.pdf: BEViewer_blank_annotated.svg
	inkscape --without-gui --export-pdf=$@ $<


# $(svg_objects): %.pdf: %.svg
# 	inkscape --without-gui --export-pdf=$@ $<
# 
# $(png_objects): %.pdf: %.png
# 	inkscape --without-gui --export-pdf=$@ $<
# 
# $(gif_objects): %.pdf: %.gif
# 	inkscape --without-gui --export-pdf=$@ $<

clean:
	rm -rf $(images)


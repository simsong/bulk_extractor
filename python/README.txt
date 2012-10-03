This directory contains the following post-processing analysis tools.

bulk_diff.py          - compares two bulk_extractor runs and reports
                        what's changed. 

cda_tool.py	      - A variety of cross-drive analysis functions.

identify_filenames.py - reads feature files and a DFXML file for a
                        disk image and reports the file from which
			each feature came

post_process_exif     - reads the exif.txt feature file and produces a
		        CSV file from all of the XML-encoded EXIF information

This directory also contains modules for working with digital forensics XML:

bulk_extractor_reader.py 
	          - a DFXML python module for reading the report.xml
                    file created by bulk_extractor and reading the feature files.
		    Also allows reading a ZIP file produced from a
                    bulk_extrator output directory as if it were uncompressed.
dfxml.py          - a DFXML python module for reading DFXML files
fiwalk.py         - a DFXML python module for producing DFXML streams using fiwalk
ttable.py	  - produces nicely formatted Python tables


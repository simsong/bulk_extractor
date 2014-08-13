Bulk_extractor Testing
======================

There are several different kinds of BE tests:

1 - Testing with known files to make sure that known items are present.

2 - Testing during development to make sure that the program doesn't crash.

3 - Testing between versions to determine which features are "new" and which features are no longer discovered.

4 - Performance testing.



Testing with Known Files (New with BE 1.5.2)
============================================

usage:  $ python3 regress.py --datacheck       

The directory Data/ contains a set of files that have been specially
constructed for bulk_extractor testing.

The file data_features.txt is a concatenation of all of the features
that should be found in the Data directory.

The regress.py program with the '--datacheck' option run
bulk_extractor on the Data directory and read the data_features.txt
file to make sure that all of the features were found.





Additional information can be found on:

https://github.com/simsong/bulk_extractor/wiki/Testing


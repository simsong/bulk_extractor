bulk_extractor 1.5 testing plan:

First scan for false positives:
- Run bulk_extarctor1.5 with -e xor scanner on the cell phone, Mac and Windows images
- Use python/be_sampler.py to extract 100 hits from each feature file and manually review.
  - Report any false positives and determine if new rules should be written to remove them.

Now compare 1.4 to 1.5:
- Assemble a set of files and disk images in tests/Data that test:
  - Every scanner
  - A modern (<3 years old) cell phone image
  - A modern (<3 years old) Mac laptop image 
  - A Windows system image image (<5 years old)

- Run bulk_extractor1.4 on all of the images with regress.py --datadir option and put output in BE1.4/
- Run bulk_extractor1.5 RC on all images with regress.py --datadir and put output in BE1.5/
- Run regress.py --datadircomp option to compare the results of BE1.4 and BE1.5 runs.
  - Write up and explain significant differences.

- Repeat until ready to ship
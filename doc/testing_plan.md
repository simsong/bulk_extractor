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

================
Outstanding issues
June 30, 2014 - slg
Image filename:         /corp/nps/drives/nps-2011-2tb/nps-2011-2tb.E01
*** error: pos0=285028122624 was started by threadid 12 but never ended
*** error: pos0=260046848000 was started by threadid 23 but never ended
*** error: pos0=332255985664 was started by threadid 3 but never ended
*** error: pos0=589182271488 was started by threadid 25 but never ended
*** error: pos0=578964946944 was started by threadid 11 but never ended
*** error: pos0=478704304128 was started by threadid 6 but never ended
*** error: pos0=536099160064 was started by threadid 17 but never ended
*** error: pos0=369803395072 was started by threadid 19 but never ended
*** error: pos0=554201776128 was started by threadid 5 but never ended
*** error: pos0=349838508032 was started by threadid 37 but never ended
*** error: pos0=422030868480 was started by threadid 30 but never ended
*** error: pos0=572455387136 was started by threadid 61 but never ended
*** error: pos0=563798343680 was started by threadid 16 but never ended

- Why did BE not finish running?
- replicate the problem on just the POS0 with:

bulk_extractor -d2 -Z -Y 285028122624 -j1 -o out-R1 /corp/nps/drives/nps-2011-2tb/nps-2011-2tb.E01
-- Traces through every new scanner invocation


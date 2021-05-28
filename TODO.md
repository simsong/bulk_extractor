- make setting sp.info a function.
- better plan for hashing than md5g in phase1
- modify stand-alone scanner so that you can specify the scanner to add in?
- or #include it
- new idea - improve multi-threading by creating new workers for each scanner, rather than each sbuf. Free the sbuf when the last scanner is done with it. Use the auto_pointer?

- Use the stand-alone scanner as a template to start creating individual scanner tests.
  - First to scan-email as a test.
  - DO a test for the feature recorder set with a scanner?

bulk_extractor SQL tests:
- Regression tests
- test image w/ known outputs
- Test image with 1B words and run in reduced memory environment to make sure that histogram overflow works properly.

Wordlist:
 - new options to make it a plain wordlist, with no other information in the file.

- [ ] Remove dependency on OpenSSL


- [ ] Wordlist:  new options to make it a plain wordlist, with no other information in the file.

- [ ] scan_hex to decode hex-coded strings.

approach for histogram transformation:

- Each feature recorder keeps track of the histogram defs.
  - needs local vector of the defs?
  - or a pointer to the def?
- feature recorder asked to produce its histograms
- histograms can be produced in memory or at close.

Then:
- in-memory histogram needs to create a different in-memory for each def.



- [ ] Car VIN detector. See https://en.wikipedia.org/wiki/Vehicle_identification_number.

Sample VINs:
```
1G1FP31F8KL297099
2HGFA1F52BH523214
1FTWF3D56AEA90906
3C6JR6AT8FG525132
JA4MW51R91J008713
```

- [ ] Make scan_scan command-count a tunable parameter?

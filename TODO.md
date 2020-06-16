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

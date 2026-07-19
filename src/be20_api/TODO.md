# Log of work on
## 2021-04-23
- Got `TEST_CASE("run", "[scanner_set]")` mostly working.
- After it runs, the output directory looks like this:
```
(base) simsong@nimi be13_api % ls -l /var/folders/09/8v4pdnys627fqqh3vjbvsnq40000gn/T/ISmG9qlC/
total 4
-rw-r--r--  1 simsong  staff    0 Apr 23 21:20 alerts.txt
-rw-r--r--  1 simsong  staff  172 Apr 23 21:20 sha1_bufs.txt
-rw-------  1 simsong  staff    0 Apr 23 21:20 sha1_bufs_Az??
(base) simsong@nimi be13_api % cat  /var/folders/09/8v4pdnys627fqqh3vjbvsnq40000gn/T/ISmG9qlC/sha1_bufs.txt
# BANNER FILE NOT PROVIDED (-b option)
# BE13_API-Version: 1.0.0
# Feature-Recorder: sha1_bufs
# Feature-File-Version: 1.1
hello-0	d3486ae9136e7856bc42212385ea797094475802
```

- [ ] Histogram is created with the wrong filename
- [ ] Histogram file is empty

## 2021-04-24
Current problems are the UTF-8 histograms that are extracted with
regular expressions. Ideally we should do the regular expressions in
Unicode, not in UTF-8

Another option is to do everything as UTF-32 regex and convert the
UTF-32 to UTF-8 when rendering into the files.
- https://stackoverflow.com/questions/37989081/how-to-use-unicode-range-in-c-regex

Another option is to add an ICU dependency:
- https://unicode-org.github.io/icu/userguide/strings/regexp.html

See also:
- http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0169r0.html

Oh, Boost has a unicode regular expressions too:
- https://www.boost.org/doc/libs/1_46_1/libs/regex/doc/html/boost_regex/ref/non_std_strings/icu/unicode_algo.html

But make a decision. What we currently have is a mess.

## 2021-04-25
Found an error in which a value from the stack was being passed by
reference, the reference was being retained, and then it was going
bad.
- [ ] Review every pass by reference and change to pass by value when
  possible. Note that pass by value may be more efficient than pass by
  reference with modern compilers.
- [x] Looks like the Atomic Unicode Histogram is using an ASCII/UTF-8
  regular expression on a UTF32 value, which isn't working. Perhaps
  I'm wrong above, and all regular expressions should be done in UTF-8
  and not UTF-32?  EDIT: Decided not to do this.
- [x] Perhaps move to SRELL as the regex package?
  http://www.akenotsuki.com/misc/srell/en/.  EDIT: Decided not to do this.


## 2021-04-27
All errors in histogram production seem to be fixed!
- [ ] Need to decide if the first BE2.0 program will be bulk_extractor
  of tcpflow.  Since tcpflow works, let's with with bulk_extractor.


# Outstanding things to do

- [ ] move histograms out of feature_recorder and feature_recorder_set.
- [ ] Instead, histograms are made by the scanner set after the scanners have run, in the shutdown mode.
    - The feature recorders just need a way of reading the contents.
    - The feature_recorder can have any number of readers. It's just an open iostream.
- [ ] Make histogram in-memory and throw them out if you run out of memory, going into low-memory mode for the second pass.
- [ ] Merge of all outstanding histograms can be done single-threaded
      or multi-threaded.

## 2021-05-08
- [ ] Get scanner commands moved from scanner_set to scanner_config.
- [ ] Implement processing of scanner commands to scanner set.
- [ ] Implement tests

## 2021-06-12
- [ ] sbuf_stream and sbuf_private should both be factored into sbuf.

- [ ] FrequencyReportHistogram should use unique_ptr<> rather than
  actually the report elements on the vector.

## 2021-11-16
- [ ] Don't need get_scanner_by_name(). I just need a list of the
  enabled scanners and a map of scanner names to scanner info

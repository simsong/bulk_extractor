a<h3>Regression testing for release</h3>

- Compile with and without otimization and make sure results are the same

- Compare results (with --diff) from last release and current release and report on the differences.


<h3>Features Files</h3>

<strong>es_keys.txt</strong> (scan_aes)<br />
<strong>bulk.txt</strong> (scan_bulk)<br />
<strong>ccn.txt</strong> (scan_accts)<br />
<strong>ccn_track2.txt</strong> (scan_accts)<br />
<strong>domain.txt</strong> (scan_email)<br />
<strong>elf.txt</strong> (scan_elf)<br />
<strong>email.txt</strong> (scan_email)<br />
<strong>ether.txt</strong> (scan_net, scan_email)<br />
<strong>exif.txt</strong> (scan_exif, scan_exiv2)<br />
<strong>extx.txt</strong> (scan_extx)<br />
<strong>facebook.txt</strong> (scan_facebook)<br />
<strong>find.txt</strong> (scan_find)<br />
<strong>gps.txt</strong> (scan_gps, scan_exif, scan_exiv2)<br />
<strong>hex.txt</strong> (scan_base16)<br />
<strong>httpheader.txt</strong> (scan_httpheader)<br />
<strong>httplogs.txt</strong> (scan_httplogs)<br />
<strong>identified_blocks.txt</strong> (scan_hashdb)<br />
<strong>ip.txt</strong> (scan_net)<br />
<strong>jpeg_carved.txt</strong> (scan_exif)<br />
<strong>json.txt</strong> (scan_json)<br />
<strong>kml.txt</strong> (scan_kml)<br />
<strong>lift_tags.txt</strong> (scan_lift)<br />
<strong>lightgrep.txt</strong> (scan_lightgrep)<br />
<strong>packets.pcap</strong> (scan_net)<br />
<strong>pii.txt</strong> (scan_accts)<br />
<strong>pipe.txt</strong> (scan_pipe)<br />
<strong>rar.txt</strong> (scan_rar)<br />
<strong>rfc822.txt</strong> (scan_email)<br />
<strong>sceadan.txt</strong> (scan_sceadan)<br />
<strong>sqlite.txt</strong> (scan_sqlite)<br />
<strong>tcp.txt</strong> (scan_net)<br />
<strong>telephone.txt</strong> (scan_accts)<br />
<strong>unrar_carved.txt</strong> (scan_rar)<br />
<strong>unzip_carved.txt</strong> (scan_zip)<br />
<strong>url.txt</strong> (scan_email)<br />
<strong>url_facebook-address.txt</strong> (scan_email)<br />
<strong>url_facebook-id.txt</strong> (scan_email)<br />
<strong>url_microsoft-live.txt</strong> (scan_email)<br />
<strong>url_searches.txt</strong> (scan_email)<br />
<strong>url_services.txt</strong> (scan_email)<br />
<strong>vcard.txt</strong> (scan_vcard)<br />
<strong>windirs.txt</strong> (scan_windirs)<br />
<strong>winlnk.txt</strong> (scan_winlnk)<br />
<strong>winpe.txt</strong> (scan_winpe)<br />
<strong>winprefetch.txt</strong> (scan_winprefetch)<br />
<strong>wordlist.txt</strong> (scan_wordlist)<br />
<strong>zip.txt</strong> (scan_zip)<br />

<h4>Other Files</h4>
Scanners can also write out histogram files. Is there just a subset of scanners that do this?<br />
The histogram files mentioned in the manual include:<br />
ccn_histogram.txt<br />
ccn_histogram_track2.txt<br />
domain_histogram.txt<br />
ether_histogram.txt<br />
find_histogram.txt<br />
ip_histogram.txt<br />
lightgrep_histogram.txt<br />
tcp_histogram.txt<br />
telephone_histogram.txt<br />
wordlist_histogram.txt	<br />
<br />
report.xml<br />

<h3>Regression Test Files</h3>

These are the test files in git repository path bulk_extractor/tests/Data.<br />
I am trying to note the scanner(s) that read each file in parents to the right.<br />
<br />
Note: The purpose of these test files still needs to be identified and associated with one or more scanners.<br />
<strong>5.psd</strong><br />	
<strong>ansi.E01</strong><br />
<strong>base64.eml</strong> (scan_base64)<br />
<strong>base64.emlx</strong> (scan_base64)<br />
<strong>beth.odt</strong><br />
<strong>bitcoin.txt</strong><br />
<strong>bitlocker.tar</strong><br />
<strong>credit_card_numbers.htm</strong> (scan_accounts)<br />
<strong>deployPkg.dll.lnk</strong> (scan_winlnk?)<br />
<strong>FIREFOX.EXE-18ACFCFF.pf</strong><br />
<strong>german_ansi.E01</strong><br />
<strong>german_utf8.E01</strong><br />
<strong>kml_samples.E01</strong> (scan_kml)<br />
<strong>MEGATRON-psd7909</strong><br />
<strong>mywinprefetch_cat</strong> (scan_winprefetch)<br />
<strong>nps-2010-emails.E01</strong> (scan_email)<br />
<strong>NTLM-wenchao.pcap</strong> (scan_net)<br />
<strong>pdf_fragment</strong> (scan_pdf)<br />
<strong>rar_samples.tar</strong> (scan_rar)<br />
<strong>skipped-packets.bin</strong> (scan_net)<br />
<strong>ssn_test.txt</strong> (scan_accounts)<br />
<strong>test-acct.txt</strong> (scan_accounts)<br />
<strong>test-urls.txt</strong> (scan_email)<br />
<strong>testfile2_ANSI.txt</strong><br />
<strong>testfile2_UTF-8.txt</strong><br />
<strong>utf8-examples.txt</strong><br />
<strong>testpage.bin</strong><br />
<strong>utf8-examples.html</strong><br />
<strong>utf8-examples.rtf</strong><br />

<h3>Map each scanner to associated feature files and test files</h3>

Note: This needs to be corrected and updated still.<br />

Scanner accts   (Looks for phone numbers, credit card numbers, etc.)<br />
	
	Feature files: 	ccn.txt
					ccn_track2.txt
					domain.txt
					pii.txt
					telephone.txt

	Test files:		credit_card_numbers.htm
					ssn_test.txt		
					test-acct.txt

Scanner <strong>aes</strong>       (Detects in-memory AES keys from their key schedules.)<br />
	
	Feature files:  aes_keys.txt

	Test files:

Scanner <strong>ascii85</strong>  (Description Needed)<br />
	
	Feature files:

	Test files:

Scanner <strong>base16</strong>  (Decodes hexadecimal test)<br />
	
	Feature files:	hex.txt

	Test files:

Scanner <strong>base64</strong>  (Decodes BASE64 text)<br />
	
	Feature files: 

	Test files:		base64.eml
					base64.emlx
	
Scanner <strong>bulk</strong>     (Description Needed)<br />
	
	Feature files: bulk.txt

	Test files:

Scanner <strong>elf</strong>     (Detects and decodes ELF headers)<br />
	
	Feature files: elf.txt

	Test files:

Scanner <strong>email</strong>    (Description Needed)<br />
	
	Feature files: 	domain.txt
					email.txt
					ether.txt
					rfc822.txt
					url.txt
					url_facebook-address
					url_facebook-id
					url_microsoft-live 
					url_searches.txt
					url_services.txt

	Test files:	
			nps-2010-emails.E01	

Scanner <strong>exif</strong>    (Decodes EXIF headers in JPEGs using built-in decoder.)<br />
	
	Feature files: 	exif.txt 
					gps.txt
					jpeg_carved.txt

	Test files:

Scanner <strong>exiv2</strong>    (Decodes EXIF headers in JPEGs using libexiv2 - for regression testing)<br />
	
	Feature files:	exif.txt
					gps.txt

	Test files:

Scanner <strong>facebook</strong>  (Facebook HTML)<br />
	
	Feature files: facebook.txt

	Test files:

Scanner <strong>find</strong>     (Does keyword searching)<br />
	
	Feature files: 	find.txt

	Test files:

Scanner <strong>gps</strong>      (Detects XML from Garmin GPS devices)<br />
	
	Feature files: gps.txt

	Test files:

Scanner <strong>gzip</strong>     (Detects and decompresses GZIP files and gzip stream)<br />
	
	Feature files: zip.txt

	Test files:

Scanner <strong>hashdb</strong>   (Search for sector hashes/ make a sector hash database)<br />
	
	Feature files: identified_blocks.txt

	Test files:

Scanner <strong>hiber</strong>    (Detects and decompresses Windows hibernation fragments)<br />
	
	Feature files:

	Test files:

Scanner <strong>httplog</strong>  (Search for web server logs)<br />
	
	Feature files: httplog.txt

	Test files:

Scanner <strong>json</strong>     (Detects JavaScript Object Notation files)<br />
	
	Feature files: json.txt

	Test files:

Scanner <strong>kml</strong>      (Detects KML files)<br />
	
	Feature files:  gps.txt
					kml.txt

	Test files:
					kml_samples.E01

Scanner <strong>lightgrep</strong>	 (Description Needed)<br />
	Feature files: 	lightgrep.txt

	Test files:

Scanner <strong>net</strong>      (IP packet scanning and carving)<br />
	
	Feature files: 	domain.txt
					ether.txt
					ip.txt
					packets.pcap
					tcp.txt
	Test files:
	
Scanner <strong>outlook</strong>  (Decrypts Outlook Compressible Encryption.)<br />
	
	Feature files:	
	
	Test files:

Scanner <strong>pdf</strong>      (Extracts text from some kinds of PDF files.)<br />

	Feature files:

	Test files: 	pdf_fragment
	
Scanner <strong>rar</strong>      (RAR files)<br />
	
	Feature files:	rar.txt

	Test files:		rar_samples.tar

Scanner <strong>sqlite</strong>   (SQLite3 databases - only if they are contiguous.)<br />
	
	Feature files: sqlite.txt

	Test files:

Scanner <strong>vcard</strong>    (Carves VCARD files.)<br />
	
	Feature files: vcard.txt

	Test files:

Scanner <strong>windirs</strong>  (Windows directory entries)<br />
	
	Feature files: windirs.txt

	Test files:

Scanner <strong>winlnk</strong>   (Windows LNK files)<br />
	
	Feature files: winlnk.txt

	Test files:

Scanner <strong>winpe</strong>     (Description Needed)<br />
	
	Feature files: winpe.txt

	Test files:

Scanner <strong>winprefetch</strong>  (Extracts fields from Windows prefetch files and file fragments.)<br />
	
	Feature files: winprefetch.txt

	Test files:
	
Scanner <strong>wordlist</strong>  (Builds word list for password cracking.)<br />

	Feature files: wordlist.txt

	Test files:

Scanner <strong>xor</strong>      (XOR obfuscation)<br />

	Feature files:

	Test files:

Scanner <strong>zip</strong>      (Detects and decompresses ZIP files and zlib streams.)<br />
	
	Feature files:	zip.tx

	Test files:



<h2>Regression testing for release</h2>

- Compile with and without otimization and make sure results are the same

- Compare results (with --diff) from last release and current release and report on the differences.


<h2>Features Files</h2>

aes_keys.txt				(scan_aes)
alerts.txt --- which scanner is this associated with?
bulk.txt					(scan_bulk)
ccn.txt						(scan_accts)
ccn_track2.txt				(scan_accts)
domain.txt					(scan_email)
elf.txt						(scan_elf)
email.txt					(scan_email)
ether.txt					(scan_net, scan_email)
exif.txt					(scan_exif, scan_exiv2)
extx.txt					(scan_extx)
facebook.txt				(scan_facebook)
find.txt					(scan_find)
gps.txt						(scan_gps, scan_exif, scan_exiv2)
hex.txt						(scan_base16)
httpheader.txt				(scan_httpheader)
httplogs.txt				(scan_httplogs)		
identified_blocks.txt		(scan_hashdb)
ip.txt						(scan_net)
jpeg_carved.txt				(scan_exif)
json.txt					(scan_json)
kml.txt						(scan_kml)
lift_tags.txt				(scan_lift)
lightgrep.txt				(scan_lightgrep)
packets.pcap				(scan_net)
pii.txt						(scan_accts)
pipe.txt					(scan_pipe)
rar.txt						(scan_rar)
rfc822.txt					(scan_email)
sceadan.txt					(scan_sceadan)
sqlite.txt					(scan_sqlite)
tcp.txt						(scan_net)
telephone.txt				(scan_accts)
unrar_carved.				(scan_rar)
unzip_carved.txt			(scan_zip)
url.txt						(scan_email)
url_facebook-address.txt	(scan_email)
url_facebook-id.txt			(scan_email)
url_microsoft-live.txt		(scan_email)
url_searches.txt			(scan_email)
url_services.txt			(scan_email)
vcard.txt			   		(scan_vcard)
windirs.txt					(scan_windirs)
winlnk.txt					(scan_winlnk)
winpe.txt					(scan_winpe)
winprefetch.txt				(scan_winprefetch)
wordlist.txt				(scan_wordlist)
zip.txt						(scan_zip)

===Other Files===
Scanners can also write out histogram files. Is there just a subset of scanners that do this?
The histogram files mentioned in the manual include:
ccn_histogram.txt
ccn_histogram_track2.txt
domain_histogram.txt
ether_histogram.txt
find_histogram.txt
ip_histogram.txt
lightgrep_histogram.txt
tcp_histogram.txt
telephone_histogram.txt
wordlist_histogram.txt	

report.xml

Regression Test Files

These are the test files in git repository path bulk_extractor/tests/Data.
I am trying to note the scanner(s) that read each file in parents to the right. 

Note: The purpose of these test files still needs to be identified and associated with one or more scanners.

5.psd	
ansi.E01		
base64.eml					(scan_base64)
base64.emlx					(scan_base64)		
beth.odt		
bitcoin.txt		
bitlocker.tar		
credit_card_numbers.htm		(scan_accounts)
deployPkg.dll.lnk			(scan_winlnk?)
FIREFOX.EXE-18ACFCFF.pf	
german_ansi.E01		
german_utf8.E01		
kml_samples.E01				(scan_kml)
MEGATRON-psd7909	
mywinprefetch_cat			(scan_winprefetch)	
nps-2010-emails.E01	        (scan_email)
NTLM-wenchao.pcap			(scan_net)
pdf_fragment				(scan_pdf)
rar_samples.tar				(scan_rar)
skipped-packets.bin			(scan_net)
ssn_test.txt				(scan_accounts)		
test-acct.txt				(scan_accounts)
test-urls.txt				(scan_email)
testfile2_ANSI.txt	
testfile2_UTF-8.txt	
utf8-examples.txt
testpage.bin
utf8-examples.html
utf8-examples.rtf


Map each scanner to associated feature files and test files

Note: This needs to be corrected and updated still.

Scanner accts   (Looks for phone numbers, credit card numbers, etc.)
	Feature files: 	ccn.txt
					ccn_track2.txt
					domain.txt
					pii.txt
					telephone.txt

	Test files:		credit_card_numbers.htm
					ssn_test.txt		
					test-acct.txt

Scanner aes     (Detects in-memory AES keys from their key schedules.)
	Feature files:  aes_keys.txt

	Test files:

Scanner ascii85  (Description Needed)
	Feature files:

	Test files:

Scanner base16  (Decodes hexadecimal test)
	Feature files:	hex.txt

	Test files:

Scanner base64  (Decodes BASE64 text)
	Feature files: 

	Test files:		base64.eml
					base64.emlx
	
Scanner bulk     (Description Needed)
	Feature files: bulk.txt

	Test files:

Scanner elf     (Detects and decodes ELF headers)
	Feature files: elf.txt

	Test files:

Scanner email    (Description Needed)
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

Scanner exif    (Decodes EXIF headers in JPEGs using built-in decoder.)
	Feature files: 	exif.txt 
					gps.txt
					jpeg_carved.txt

	Test files:

Scanner exiv2    (Decodes EXIF headers in JPEGs using libexiv2 - for regression testing)
	Feature files:	exif.txt
					gps.txt

	Test files:

Scanner facebook  (Facebook HTML)
	Feature files: facebook.txt

	Test files:

Scanner find     (Does keyword searching)
	Feature files: 	find.txt

	Test files:

Scanner gps      (Detects XML from Garmin GPS devices)
	Feature files: gps.txt

	Test files:

Scanner gzip     (Detects and decompresses GZIP files and gzip stream)
	Feature files: zip.txt

	Test files:

Scanner hashdb   (Search for sector hashes/ make a sector hash database)
	Feature files: identified_blocks.txt

	Test files:

Scanner hiber    (Detects and decompresses Windows hibernation fragments)
	Feature files:

	Test files:

Scanner httplog  (Search for web server logs)
	Feature files: httplog.txt

	Test files:

Scanner json     (Detects JavaScript Object Notation files)
	Feature files: json.txt

	Test files:

Scanner kml      (Detects KML files)
	Feature files: gps.txt, kml.txt

	Test files:
		kml_samples.E01

Scanner lightgrep	 (Description Needed)
	Feature files: lightgrep.txt

	Test files:

Scanner net      (IP packet scanning and carving)
	Feature files: 	domain.txt
					ether.txt
					ip.txt
					packets.pcap
					tcp.txt
	Test files:
	
Scanner outlook  (Decrypts Outlook Compressible Encryption.)
	Feature files:	
	
	Test files:

Scanner pdf      (Extracts text from some kinds of PDF files.)
	Feature files:

	Test files: 	pdf_fragment
	
Scanner rar      (RAR files)
	Feature files:	rar.txt

	Test files:		rar_samples.tar

Scanner sqlite   (SQLite3 databases - only if they are contiguous.)
	Feature files: sqlite.txt

	Test files:

Scanner vcard    (Carves VCARD files.)
	Feature files: vcard.txt

	Test files:

Scanner windirs  (Windows directory entries)
	Feature files: windirs.txt

	Test files:

Scanner winlnk   (Windows LNK files)
	Feature files: winlnk.txt

	Test files:

Scanner winpe     (Description Needed)
	Feature files: winpe.txt

	Test files:

Scanner winprefetch  (Extracts fields from Windows prefetch files and file fragments.)
	Feature files: winprefetch.txt

	Test files:
	
Scanner wordlist  (Builds word list for password cracking.)
	Feature files: wordlist.txt

	Test files:

Scanner xor      (XOR obfuscation)
	Feature files:

	Test files:

Scanner zip      (Detects and decompresses ZIP files and zlib streams.)
	Feature files:	zip.tx

	Test files:



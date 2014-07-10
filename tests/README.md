<h3>Regression testing for release</h3>

- Compile with and without otimization and make sure results are the same

- Compare results (with --diff) from last release and current release and report on the differences.


<h3>Features Files</h3>

es_keys.txt (scan_aes)<br />
bulk.txt (scan_bulk)<br />
ccn.txt (scan_accts)<br />
ccn_track2.txt (scan_accts)<br />
domain.txt (scan_email)<br />
elf.txt (scan_elf)<br />
email.txt (scan_email)<br />
ether.txt (scan_net, scan_email)<br />
exif.txt (scan_exif, scan_exiv2)<br />
extx.txt (scan_extx)<br />
facebook.txt (scan_facebook)<br />
find.txt (scan_find)<br />
gps.txt (scan_gps, scan_exif, scan_exiv2)<br />
hex.txt (scan_base16)<br />
httpheader.txt (scan_httpheader)<br />
httplogs.txt (scan_httplogs)<br />
identified_blocks.txt (scan_hashdb)<br />
ip.txt (scan_net)<br />
jpeg_carved.txt (scan_exif)<br />
json.txt (scan_json)<br />
kml.txt (scan_kml)<br />
lift_tags.txt (scan_lift)<br />
lightgrep.txt (scan_lightgrep)<br />
packets.pcap (scan_net)<br />
pii.txt (scan_accts)<br />
pipe.txt (scan_pipe)<br />
rar.txt (scan_rar)<br />
rfc822.txt (scan_email)<br />
sceadan.txt (scan_sceadan)<br />
sqlite.txt (scan_sqlite)<br />
tcp.txt (scan_net)<br />
telephone.txt (scan_accts)<br />
unrar_carved.txt (scan_rar)<br />
unzip_carved.txt (scan_zip)<br />
url.txt (scan_email)<br />
url_facebook-address.txt (scan_email)<br />
url_facebook-id.txt (scan_email)<br />
url_microsoft-live.txt (scan_email)<br />
url_searches.txt (scan_email)<br />
url_services.txt (scan_email)<br />
vcard.txt (scan_vcard)<br />
windirs.txt (scan_windirs)<br />
winlnk.txt (scan_winlnk)<br />
winpe.txt (scan_winpe)<br />
winprefetch.txt (scan_winprefetch)<br />
wordlist.txt (scan_wordlist)<br />
zip.txt (scan_zip)<br />

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

5.psd<br />	
ansi.E01<br />		
base64.eml (scan_base64)<br />
base64.emlx (scan_base64)<br />		
beth.odt<br />		
bitcoin.txt<br />		
bitlocker.tar<br />		
credit_card_numbers.htm (scan_accounts)<br />
deployPkg.dll.lnk (scan_winlnk?)<br />
FIREFOX.EXE-18ACFCFF.pf<br />	
german_ansi.E01<br />		
german_utf8.E01<br />		
kml_samples.E01 (scan_kml)<br />
MEGATRON-psd7909<br />	
mywinprefetch_cat (scan_winprefetch)<br />	
nps-2010-emails.E01 (scan_email)<br />
NTLM-wenchao.pcap (scan_net)<br />
pdf_fragment (scan_pdf)<br />
rar_samples.tar (scan_rar)<br />
skipped-packets.bin (scan_net)<br />
ssn_test.txt (scan_accounts)<br />	
test-acct.txt (scan_accounts)<br />
test-urls.txt (scan_email)<br />
testfile2_ANSI.txt<br />	
testfile2_UTF-8.txt<br />	
utf8-examples.txt<br />
testpage.bin<br />
utf8-examples.html<br />
utf8-examples.rtf<br />


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

Scanner aes     (Detects in-memory AES keys from their key schedules.)<br />
	
	Feature files:  aes_keys.txt

	Test files:

Scanner ascii85  (Description Needed)<br />
	Feature files:

	Test files:

Scanner base16  (Decodes hexadecimal test)<br />
	Feature files:	hex.txt

	Test files:

Scanner base64  (Decodes BASE64 text)<br />
	Feature files: 

	Test files:		base64.eml
					base64.emlx
	
Scanner bulk     (Description Needed)<br />
	Feature files: bulk.txt

	Test files:

Scanner elf     (Detects and decodes ELF headers)<br />
	Feature files: elf.txt

	Test files:

Scanner email    (Description Needed)<br />
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

Scanner exif    (Decodes EXIF headers in JPEGs using built-in decoder.)<br />
	Feature files: 	exif.txt 
					gps.txt
					jpeg_carved.txt

	Test files:

Scanner exiv2    (Decodes EXIF headers in JPEGs using libexiv2 - for regression testing)<br />
	Feature files:	exif.txt
					gps.txt

	Test files:

Scanner facebook  (Facebook HTML)<br />
	Feature files: facebook.txt

	Test files:

Scanner find     (Does keyword searching)<br />
	Feature files: 	find.txt

	Test files:

Scanner gps      (Detects XML from Garmin GPS devices)<br />
	Feature files: gps.txt

	Test files:

Scanner gzip     (Detects and decompresses GZIP files and gzip stream)<br />
	Feature files: zip.txt

	Test files:

Scanner hashdb   (Search for sector hashes/ make a sector hash database)<br />
	Feature files: identified_blocks.txt

	Test files:

Scanner hiber    (Detects and decompresses Windows hibernation fragments)<br />
	Feature files:

	Test files:

Scanner httplog  (Search for web server logs)<br />
	Feature files: httplog.txt

	Test files:

Scanner json     (Detects JavaScript Object Notation files)<br />
	Feature files: json.txt

	Test files:

Scanner kml      (Detects KML files)<br />
	Feature files: gps.txt, kml.txt

	Test files:
		kml_samples.E01

Scanner lightgrep	 (Description Needed)<br />
	Feature files: lightgrep.txt

	Test files:

Scanner net      (IP packet scanning and carving)<br />
	Feature files: 	domain.txt
					ether.txt
					ip.txt
					packets.pcap
					tcp.txt
	Test files:
	
Scanner outlook  (Decrypts Outlook Compressible Encryption.)<br />
	Feature files:	
	
	Test files:

Scanner pdf      (Extracts text from some kinds of PDF files.)<br />
	Feature files:

	Test files: 	pdf_fragment
	
Scanner rar      (RAR files)<br />
	Feature files:	rar.txt

	Test files:		rar_samples.tar

Scanner sqlite   (SQLite3 databases - only if they are contiguous.)<br />
	Feature files: sqlite.txt

	Test files:

Scanner vcard    (Carves VCARD files.)<br />
	Feature files: vcard.txt

	Test files:

Scanner windirs  (Windows directory entries)<br />
	Feature files: windirs.txt

	Test files:

Scanner winlnk   (Windows LNK files)<br />
	Feature files: winlnk.txt

	Test files:

Scanner winpe     (Description Needed)<br />
	Feature files: winpe.txt

	Test files:

Scanner winprefetch  (Extracts fields from Windows prefetch files and file fragments.)<br />
	Feature files: winprefetch.txt

	Test files:
	
Scanner wordlist  (Builds word list for password cracking.)<br />
	Feature files: wordlist.txt

	Test files:

Scanner xor      (XOR obfuscation)<br />
	Feature files:

	Test files:

Scanner zip      (Detects and decompresses ZIP files and zlib streams.)<br />
	Feature files:	zip.tx

	Test files:



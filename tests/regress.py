#!/usr/bin/env python3
# coding=UTF-8
"""
Regression system: 

A basic framework for running bulk_extractor and viewing the results.

Options include:

 --verify  - verify a single BE report
 --tune    - runs bulk_extractor under a variety of conditions
 - Total number of features are reported and compared with the archives.
"""

__version__ = "1.5.0"

b'This module needs Python 2.7 or later.'

import os,sys
try:
    sys.path.append(os.getenv("DOMEX_HOME") + "/src/bulk_extractor/trunk/python/") # add the library
except TypeError as e:
    pass
sys.path.append("../python/")      # add the library
sys.path.append("python/")      # add the library

from subprocess import Popen,call,PIPE
import os.path,glob,zipfile,codecs
import bulk_extractor_reader

corp_default = "/corp"


default_infile   = "nps-2009-ubnist1/ubnist1.gen3.raw"
fast_infile      = "nps-2010-emails/nps-2010-emails.raw"
full_infile      = "nps-2009-domexusers/nps-2009-domexusers.raw"
exe = "src/bulk_extractor"
nps_drives_path = "/nps/drives/"
BOM = codecs.BOM_UTF8.decode('utf-8')

# Performance tuning

MiB = 1024*1024
tune_jobs_start = 1
tune_jobs_end   = 16
tune_jobs_step  = 1

tune_pagesize_start  = 1 * MiB
tune_pagesize_end    = 16 * MiB
tune_pagesize_step   = 1 * MiB

tune_marginsize_start = 1 * MiB
tune_marginsize_end   = 4 * MiB
tune_marginsize_step  = 1 * MiB


answers = {"ubnist1.gen3":{"ALERTS_found.txt":88,
                           "bulk_tags.txt":7477796,
                           "ccn.txt":1,
                           "domain.txt":233055,
                           "elf.txt":708,
                           "email.txt":184341,
                           "ether.txt":51,
                           "exif.txt":158,
                           "find.txt":26,
                           "ip.txt":12,
                           "json.txt":87,
                           "rar.txt":4,
                           "rfc822.txt":24628,
                           "tcp.txt":68,
                           "telephone.txt":143,
                           "url.txt":44275,
                           "windirs.txt":1667,
                           "winpe.txt":2,
                           "wordlist.txt":10121828,
                           "zip.txt":1962},
           "nps-2010-emails":{"domain.txt":602,
                              "email.txt":67,
                              "exif.txt":2,
                              "jpeg.txt":21,
                              "telephone.txt":2,
                              "url.txt":535,
                              "windirs.txt":30,
                              "zip.txt":240}
           }



def ptime(t):
    r = ""
    if t>3600:
        h = t/3600
        r = "%d hour " % h
        t = t%3600
    if t>60:
        m = t / 60
        r += "%d min " % m
        t = t%60
    r += "%d sec " % t
    return r

def find_file(fn):
    if os.path.exists(fn): return fn
    chk = []
    for ne in ('.E01','.aff','.000','.001'):
        tfn = fn.replace(".raw",ne)
        if os.path.exists(tfn):
            return tfn
        chk.append([tfn])
    raise RuntimeError("Cannot find file "+tfn)


if sys.version_info < (2,7):
    raise "Requires Python 2.7 or above"

def clear_cache():
    # Are we on a Mac?
    if os.path.exists("/usr/bin/purge"):
        call("/usr/bin/purge")
        return
    # We must be on linux
    f = open("/proc/sys/vm/drop_caches","wb")
    f.write("3\n")
    f.close()

def analyze_linebyline(outdir):
    """Quick analysis of an output directory"""
    import xml.etree.cElementTree
    import xml.dom.minidom
    fn = os.path.join(outdir,"report.xml")
    lines = 0
    inprocess = dict()
    for line in open(fn):
        line = line.replace("debug:","debug")
        try:
            doc = xml.dom.minidom.parseString(line)
            start = doc.getElementsByTagName("debugwork_start")
            if start:
                threadid = int(start[0].attributes['threadid'].firstChild.wholeText)
                inprocess[threadid] = start[0]
            end = doc.getElementsByTagName("debugwork_end")
            if end:
                threadid = int(start[0].attributes['threadid'].firstChild.wholeText)
                inprocess[threadid] = start[0]
        except xml.parsers.expat.ExpatError as e:
            print(e)
            pass
        lines += 1
    print("Total lines: {}".format(lines))
    exit(0)

def reproduce_flags(outdir):
    """Craft BE flags to quickly reproduce a crash"""
    import xml.etree.cElementTree
    import xml.dom.minidom
    filename = None
    offset = None
    fn = os.path.join(outdir,"report.xml")
    active_offsets = set()
    last_start_offset = None
    for line in open(fn):
        line = line.replace("debug:","debug")
        try:
            doc = xml.dom.minidom.parseString(line)
            prov_filename = doc.getElementsByTagName("provided_filename")
            if prov_filename:
                filename = str(prov_filename[0].firstChild.wholeText)
            start = doc.getElementsByTagName("debugwork_start")
            if start:
                offset = int(start[0].attributes['pos0'].firstChild.wholeText)
                last_start_offset = offset
                active_offsets.add(offset)
            end = doc.getElementsByTagName("debugwork_end")
            if end:
                offset = int(end[0].attributes['pos0'].firstChild.wholeText)
                try:
                    active_offsets.remove(offset)
                except KeyError:
                    pass
        except xml.parsers.expat.ExpatError as e:
            #print(e)
            pass
    if len(active_offsets) < 1:
        print("*** Warning: no unfinished sectors found; using best guess of last sector started")
        offset = last_start_offset
    else:
        offset = min(active_offsets)
    return "-Y {offset} {filename}".format(offset=offset, filename=filename)

def analyze_warning(fnpart,fn,lines):
    if fnpart not in answers:
        return "(No answers for {})".format(fnpart)
    if fn not in answers[fnpart]:
        return "(No answer for {})".format(fn)
    ref = answers[fnpart][fn]
    if ref==lines: return "OK"
    if lines<ref: return "LOW (expected {})".format(ref)
    return "HIGH (expected {})".format(ref)

def analyze_reportxml(xmldoc):
    # Determine if any pages were not analyzed
    proc = dict()
    for work_start in xmldoc.getElementsByTagName("debug:work_start"):
        threadid = work_start.getAttribute('threadid')
        pos0     = work_start.getAttribute('pos0')
        if pos0 in proc:
            print("*** error: pos0={} was started by threadid {} and threadid {}".format(pos0,proc[pos0],threadid))
        else:
            proc[pos0] = threadid
    for work_end in xmldoc.getElementsByTagName("debug:work_end"):
        threadid = work_end.getAttribute('threadid')
        pos0     = work_end.getAttribute('pos0')
        if pos0 not in proc:
            print("*** error: pos0={} was ended by threadid {} but never started!".format(pos0,threadid))
        elif threadid!=proc[pos0]:
            print("*** error: pos0={} was ended by threadid {} but ended by threadid {}".format(pos0,proc[pos0],threadid))
        else:
            del proc[pos0]
    
    for (pos0,threadid) in proc.items():
        print("*** error: pos0={} was started by threadid {} but never ended".format(pos0,threadid))
    
    scanner_times = []
    scanners = xmldoc.getElementsByTagName("scanner_times")[0]
    total = 0
    for path in scanners.getElementsByTagName("path"):
        name    = path.getElementsByTagName("name")[0].firstChild.wholeText
        calls   = int(path.getElementsByTagName("calls")[0].firstChild.wholeText)
        seconds = float(path.getElementsByTagName("seconds")[0].firstChild.wholeText)
        total   += seconds
        scanner_times.append((name,calls,seconds))
    print("Scanner paths by time and calls")
    scanner_times.sort(key=lambda a:a[2],reverse=True)

    print("  {0:>25}  {1:8}  {2:12}  {3:12}  {4:5}".format("name","calls","sec","sec/call","% total"))
    for (name,calls,seconds) in scanner_times:
        print("  {:>25}  {:8.0f}  {:12.4f}  {:12.4f}  {:5.2f}%".format(
                name,calls,seconds,seconds/calls,100.0*seconds/total))
    
    
def analyze_outdir(outdir):
    """Print statistics about an output directory"""
    print("Analyze {}".format(outdir))

    b = bulk_extractor_reader.BulkReport(outdir)
    print("bulk_extractor version: {}".format(b.version()))
    print("Image filename:         {}".format(b.image_filename()))
    
    # Print which scanners were run and how long they took
    analyze_reportxml(b.xmldoc)
    
    hfns = list(b.histogram_files()) # histogram files
    print("")
    print("Histogram Files:        {}".format(len(hfns)))

    def get_firstline(fn):
        """Returns the first line that is not a comment"""
        for line in b.open(fn,'rb'):
            if bulk_extractor_reader.is_comment_line(line):
                continue
            return line[:-1]

    for fn in sorted(hfns):
        h = b.read_histogram(fn)
        firstline = get_firstline(fn)
        if(type(firstline)==bytes and type(firstline)!=str):
            firstline = firstline.decode('utf-8')
        print("  {:>25} entries: {:>10,}  (top: {})".format(fn,len(h),firstline))

    fnpart = ".".join(b.image_filename().split('/')[-1].split('.')[:-1])

    ffns = sorted(list(b.feature_files())) 
    if ffns:
        features = {}
        print("")
        print("Feature Files:        {}".format(len(ffns)))
        for fn in ffns:     # feature files
            lines = 0
            for line in b.open(fn,'r'):
                if not bulk_extractor_reader.is_comment_line(line):
                    lines += 1
                    features[fn] = lines
            print("  {:>25} features: {:>12,}  {}".format(fn,lines,analyze_warning(fnpart,fn,lines)))
                    
        # If there is a SQLite database, analyze that too!
    if args.featurefile and args.featuresql:
        import sqlite3
        conn = sqlite3.connect(os.path.join(outdir,"report.sqlite"))
        if conn:
            c = conn.cursor()
            c.execute("PRAGMA cache_size = 200000")
            print("Comparing SQLite3 database to feature files:")
            for fn in ffns:
                try:
                    table = "f_"+fn.lower().replace(".txt","")
                    cmd = "select count(*) from "+table
                    print(cmd)
                    c.execute(cmd);
                    ct = c.fetchone()[0]
                    print("{}:   {}  {}".format(fn,features[fn],ct))
                    # Now check them all to make sure that the all match
                    count = 0
                    for line in b.open(fn,'r'):
                        ary = bulk_extractor_reader.parse_feature_line(line)
                        if ary:
                            (path,feature) = ary[0:2]
                            path = path.decode('utf-8')
                            feature = feature.decode('utf-8')
                            c.execute("select count(*) from "+table+" where path=? and feature_eutf8=?",(path,feature))
                            ct = c.fetchone()[0]
                            if ct==1:
                                #print("feature {} {} in table {} ({})".format(path,feature,table,ct))
                                pass
                            if ct==0:
                                #pass
                                print("feature {} {} not in table {} ({})".format(path,feature,table,ct))
                            count += 1
                            if count>args.featuretest: 
                                break

                except sqlite3.OperationalError as e:
                    print(e)


def make_zip(dname):
    archive_name = dname+".zip"
    b = bulk_extractor_reader.BulkReport(dname)
    z = zipfile.ZipFile(archive_name,compression=zipfile.ZIP_DEFLATED,mode="w")
    print("Creating ZIP archive {}".format(archive_name))
    for fname in b.all_files:
        print("  adding {} ...".format(fname))
        z.write(os.path.join(dname,fname),arcname=os.path.basename(fname))
    

def make_outdir(outdir_base):
    counter = 1
    while True:
        outdir = outdir_base + ("-%02d" % counter)
        if not os.path.exists(outdir):
            return outdir
        counter += 1

def run(cmd):
    print(" ".join(cmd))
    if args.dry_run: return
    r = call(cmd)
    if r!=0:
        raise RuntimeError("{} crashed with error code {}".format(args.exe,r))

def run_outdir(outdir,gdb=False):
    """Run bulk_extarctor to a given output directory """
    print("run_outdir: ",outdir)
    cargs=['-o',outdir]
    if args.featuresql:
        cargs += ['-S','write_feature_sqlite3=YES']
    else:
        cargs += ['-S','write_feature_sqlite3=NO']
    if args.featurefile:
        cargs += ['-S','write_feature_files=YES']
    else:
        cargs += ['-S','write_feature_files=NO']
    if args.jobs: cargs += ['-j'+str(args.jobs)]
    if args.pagesize: cargs += ['-G'+str(args.pagesize)]
    if args.marginsize: cargs += ['-g'+str(args.marginsize)]
    
    cargs += ['-e','all', '-x', 'sceadan']    # enable all scanners
    #cargs += ['-e','wordlist']    # enable all scanners
    if args.extra:
        while "  " in args.extra:
            args.extra = args.extra.replace("  "," ")
        cargs += args.extra.split(" ")
    if args.debug: cargs += ['-d'+str(args.debug)]

    if args.find:
        cargs += ['-r','tests/alert_list.txt']
        cargs += ['-w','tests/stop_list.txt']
        cargs += ['-w','tests/stop_list_context.txt']
        cargs += ['-f','[a-z\.0-9]*@gsa.gov']
        cargs += ['-F','tests/find_list.txt']
    cargs += [args.image]

    # Now that we have a command, figure out how to run it...
    if gdb:
        with open("/tmp/cmds","w") as f:
            f.write("b malloc_error_break\n")
            f.write("run ")
            f.write(" ".join(cargs))
            f.write("\n")
            
        cmd = ['gdb','-e',args.exe,'-x','/tmp/cmds']
    else:
        cmd = [args.exe] + cargs
    run(cmd)

             
def sort_outdir(outdir):
    """Sort the output directory with gnu sort"""
    print("Now sorting files in "+outdir)
    for fn in glob.glob(outdir + "/*.txt"):
        if "histogram" in fn: continue
        if "wordlist"  in fn: continue
        if "tags"      in fn: continue
        fns  = fn+".sorted"
        os.environ['LC_ALL']='C' # make sure we sort in C order
        call(['sort','--buffer-size=4000000000',fn],stdout=open(fns,"w"))
        # count how many lines
        wcout = Popen(['wc','-l',fns],stdout=PIPE).communicate()[0].decode('utf-8')
        lines = int(wcout.strip().split(" ")[0])
        os.rename(fns,fn)

def check(fn,lines):
    found_lines = len(file(fn).read().split("\n"))-1
    if lines!=found_lines:
        print("{:10} expected lines: {} found lines: {}".format(fn,found_lines,lines))


def asbinary(s):
    ret = ""
    count = 0
    for ch in s:
        ret += "%02x " % ch
        count += 1
        if count>5:
            count = 0
            ret += " "
    return ret
        

FEATURE_FILE = 1
MAX_OFFSET_SIZE  = 100
MAX_FEATURE_SIZE = 1000000
MAX_CONTEXT_SIZE = 1000000

def invalid_feature_file_line(line,fields):
    if len(line)==0: return None    # empty lines are okay
    if line[0]=='#': return None # comments are okay
    if len(fields)!=3: return "Wrong number of fields"     # wrong number of fields
    if len(fields[0])>MAX_OFFSET_SIZE: return "OFFSET > "+str(MAX_OFFSET_SIZE)
    if len(fields[1])>MAX_FEATURE_SIZE: return "FEATURE > "+str(MAX_FEATURE_SIZE)
    if len(fields[2])>MAX_CONTEXT_SIZE: return "CONTEXT > "+str(MAX_CONTEXT_SIZE)
    return None
    
def validate_file(f,kind):
    is_kml = f.name.endswith(".kml")
    linenumber = 0
    if args.debug: print("Validate UTF-8 encoding in ",f.name)
    for lineb in f:
        linenumber += 1
        lineb = lineb[:-1]      # remove the \n
        try:
            line = lineb.decode('utf-8')
        except UnicodeDecodeError as e:
            print("{}:{} {} {}".format(f.name,linenumber,str(e),asbinary(lineb)))
            continue
        if bulk_extractor_reader.is_comment_line(line):
            continue        # don't test comments
        if bulk_extractor_reader.is_histogram_line(line):
            continue        # don't test
        if kind==FEATURE_FILE:
            fields = line.split("\t")
            r = invalid_feature_file_line(line,fields)
            if r:
                print("{}: {:8} {} Invalid feature file line: {}".format(f.name,linenumber,r,line))
            if is_kml and fields[1].count("kml")!=2:
                print("{}: {:8} Invalid KML line: {}".format(f.name,linenumber,line))



def validate_report(fn,do_validate=True):
    """Make sure all of the lines in all of the files in the outdir are UTF-8 and that
    the feature files have 3 or more fields on each line.
    And validate the XML!
    """
    import glob,os.path
    if args.debug: print("Validating Report: ",fn)
    res = {}
    if os.path.isdir(fn) or fn.endswith(".zip"):
        b = bulk_extractor_reader.BulkReport(fn,do_validate=True)
        for fn in b.feature_files():
            if os.path.basename(fn) in str(args.ignore):
                print("** ignore {} **".format(fn))
                continue
            validate_file(b.open(fn,'rb'),FEATURE_FILE)
    else:
        validate_file(open(fn,'rb'))
            
            
def identify_filenames(outdir):
    """Run identify_filenames on output using the installed fiwalk"""
    if_outdir = outdir + "-annotated"
    ifname    = os.path.dirname(os.path.realpath(__file__)) + "/../python/identify_filenames.py"
    res = call([sys.executable,ifname,outdir,if_outdir,'--all'])

def diff(dname1,dname2):
    args.max = int(args.max)
    def safe_filesize(fn):
        try:
            return os.path.getsize(fn)
        except FileNotFoundError:
            return 0

    def files_in_dir(dname):
        files = [fn.replace(dname+"/","") for fn in glob.glob(dname+"/*") if os.path.isfile(fn)]
        return filter(lambda fn: not fn.endswith(".sqlite") ,files)
    def lines_to_set(fn):
        "Read the lines in a file and return them as a set. Ignore the lines beginning with #"
        return set(filter(lambda line:line[0:1]!='#',open(fn).read().split("\n")))
    
    files1 = set(files_in_dir(dname1))
    files2 = set(files_in_dir(dname2))
    if files1.difference(files2):
        print("Files only in {}:\n   {}".format(dname1," ".join(files1.difference(files2))))
    if files2.difference(files1):
        print("Files only in {}:\n   {}".format(dname2," ".join(files2.difference(files1))))

    # Look at the common files. For each report the files only in one or the other
    common = files1.intersection(files2)
    for fn in sorted(common):
        fn1 = os.path.join(dname1,fn)
        fn2 = os.path.join(dname2,fn)
        
        if fn=="report.xml":
            continue
        if fn=="wordlist.txt" and not args.diffwordlist:
            continue
        if fn=="packets.pcap": 
            s1 = os.path.getsize(fn1)
            s2 = os.path.getsize(fn2)
            if s1!=s2:
                print("size({})={} but size({})={}\n",fn1,s1,fn2,s2)
            continue
        def print_diff(dname,prefix,diffset):
            if not diffset:
                return
            print("\nOnly in {}: {} lines".format(os.path.join(dname,fn),len(diffset)))
            count = 0
            for line in sorted(diffset):
                extra = ""
                if len(line) > args.diffwidth: extra="..."
                print("{}{}{}".format(prefix,line[0:args.diffwidth],extra))
                count += 1
                if count>args.max:
                    print(" ... +{} more lines".format(len(diffset)-int(args.max)))
                    break
            return len(diffset)

        lines1 = lines_to_set(fn1)
        lines2 = lines_to_set(fn2)
        if lines1!=lines2:
            print("regressdiff {}:".format(fn))
            count1 = print_diff(dname1,"<",lines1.difference(lines2))
            count2 = print_diff(dname2,">",lines2.difference(lines1))
            if count1 or count2:
                print("\n-------------\n")


def run_and_analyze():
    global args
    outdir = make_outdir(args.outdir)
    t0 = time.time()
    run_outdir(outdir,args.gdb)
    sort_outdir(outdir)
    validate_report(outdir)
    if args.identify_filenames:
        if_outdir = identify_filenames(outdir)
    analyze_outdir(outdir)
    t = time.time() - t0
    print("Regression finished at {}. Elapsed time: {} ({} sec)\nOutput in {}".format(
        time.asctime(),ptime(t),t,outdir))

def datadircomp(dir1,dir2):
    print("Validating reports in {} and {}".format(dir1,dir2))
    for d in [dir1,dir2]:
        for fn in glob.glob(os.path.join(d,"*")):
            validate_report(fn,do_validate=True)
    print("Comparing outputs".format(dir1,dir2))
    for fn1 in glob.glob(os.path.join(dir1,"*")):
        fn2 = fn1.replace(dir1,dir2)
        diff(fn1,fn2)
        

    exit(0)

if __name__=="__main__":
    import argparse 
    global args
    import sys,time

    parser = argparse.ArgumentParser(description="Perform regression testing on bulk_extractor",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--gdb",help="run under gdb",action="store_true")
    parser.add_argument("--debug",help="debug level",type=int)
    parser.add_argument("--outdir",help="output directory base",default=None)
    parser.add_argument("--exe",help="Executable to run (default {})".format(exe),default=exe)
    parser.add_argument("--image",help="image to scan (default {})".format(default_infile),default=default_infile)
    parser.add_argument("--fast",help="Run with "+fast_infile,action="store_true")
    parser.add_argument("--full",help="Run with "+full_infile,action="store_true")
    parser.add_argument("--jobs",help="Specifies number of worker threads",type=int)
    parser.add_argument("--pagesize",help="Specifies page size",type=int)
    parser.add_argument("--no-find",dest='find',help="Does not do find test (faster)",action="store_false")

    g = parser.add_mutually_exclusive_group()
    g.add_argument("--featuresql",dest='featuresql',action='store_true',help="Enable SQL feature files")
    g.add_argument("--no-featuresql",dest='featuresql',action='store_false',help="Disable SQL feature files")
    parser.set_defaults(featuresql=True)

    g = parser.add_mutually_exclusive_group()
    g.add_argument("--featurefile",dest='featurefile',action='store_true',help="Enable FILE feature files")
    g.add_argument("--no-featurefile",dest='featurefile',action='store_false',help="Disable FILE feature files")
    parser.set_defaults(featurefile=True)

    parser.add_argument("--marginsize",help="Specifies the margin size",type=int)
    parser.add_argument("--extra",help="Specify extra arguments")
    parser.add_argument("--gprof",help="Recompile and run with gprof",action="store_true")
    parser.add_argument("--diff",help="diff mode. Compare two reports",type=str,nargs='*')
    parser.add_argument("--diffwidth",type=int,help="Number of characters to display on diff lines",default=170)
    parser.add_argument("--diffwordlist",type=bool,help="compare wordlist.txt",default=False)
    parser.add_argument("--max",help="Maximum number of differences to display",default="5")
    parser.add_argument("--memdebug",help="Look for memory errors",action="store_true")
    parser.add_argument("--analyze",help="Analyze a bulk_extractor output directory")
    parser.add_argument("--linebyline",help="Specifies a bulk_extractor output file to analyze line by line")
    parser.add_argument("--zip",help="Create a zip archive of the report")
    parser.add_argument("--validate",help="Validate the contents of a report (do not run bulk_extractor)",
                        type=str,nargs='*')
    parser.add_argument("--ignore",help="Specifies a feature file or files (file1,file2) to ignore")
    parser.add_argument("--sort",help="Sort the feature files",type=str,nargs='*')
    parser.add_argument("--reproduce",help="specifies a bulk_extractor output "
            + "file from a crash and produces bulk_extractor flags to quickly "
            + "reproduce the crash")
    parser.add_argument("--clearcache",help="clear the disk cache",action="store_true")
    parser.add_argument("--tune",help="run bulk_extractor tuning. Args are coded in this script.",action="store_true")
    parser.add_argument("--featuretest",default=50,type=int,
                        help="Specifies number of features to test to make sure they are in the SQL database",)
    parser.add_argument("--no-identify_filenames",dest='identify_filenames',action="store_false",
                        help="Do not run identify_filenames")
    parser.add_argument("--datadir",help="Process all files in a directory")
    parser.add_argument("--dry-run",help="Don't actually run the program",action='store_true')
    parser.add_argument("--datadircomp",help="Compare two data dirs")

    args = parser.parse_args()
    
    if args.datadircomp:
        dirs = args.datadircomp.split(",")
        datadircomp(dirs[0],dirs[1])

    if args.validate:
        for v in args.validate:
            try:
                validate_report(v)
            except IOError as e:
                print(str(e))
        exit(0)
    if args.analyze:
        import xml.dom.minidom
        import xml.parsers.expat
        try:
            if args.analyze.endswith(".xml"):
                analyze_reportxml(xml.dom.minidom.parse(open(args.analyze,"rb")))
            else:
                analyze_outdir(args.analyze);
            exit(0)
        except xml.parsers.expat.ExpatError as e:
            print("%s does not contain a valid report.xml file" % (args.analyze))
            print(e)
        print("Attempting line-by-line analysis...\n");
        analyze_linebyline(args.analyze)
        exit(0)
    if args.reproduce:
        print(reproduce_flags(args.reproduce))
        exit(0)
    if args.linebyline:
        analyze_linebyline(args.linebyline)
        exit(0)

    if args.zip:
        make_zip(args.zip); exit(0)

    if not os.path.exists(args.exe):
        raise RuntimeError("{} does not exist".format(args.exe))

    if args.clearcache: clear_cache()

    ################################################################
    ### After this point the program is run
    ################################################################

    # Are we processing an entire directory?
    if args.datadir:
        try:
            os.mkdir(args.outdir)
        except FileExistsError:
            pass
        if not args.outdir:
            print("--datadir requires that outdir be specified")
            exit(0)
        for fn in glob.glob(os.path.join(args.datadir,"*")):
            args.image = fn
            run_outdir(os.path.join(args.outdir,os.path.basename(fn)))
        exit(0)

    # Find the bulk_extractor version and add it to the outdir
    if args.outdir==None:
        args.outdir="regress"

    args.outdir += "-"+bulk_extractor_reader.be_version(args.exe)
    corp = os.getenv("DOMEX_CORP")
    if not corp:
        corp = corp_default
    drives =  corp + nps_drives_path

    if args.fast:
        args.image  = fast_infile
        if args.extra:
            args.extra += ' '
        else:
            args.extra = ''
        args.extra  += '-G 65536'
    if args.full:
        args.image  = full_infile

    args.outdir += "-" + os.path.basename(args.image)

    if not os.path.exists(args.image):
        args.image = find_file(drives+args.image)

    if args.memdebug:
        fn = "/usr/lib/libgmalloc.dylib"
        if os.path.exist(fn):
            print("Debugging with libgmalloc")
            os.putenv("DYLD_INSERT_LIBRARIES",fn)
            os.putenv("MALLOC_PROTECT_BEFORE","1")
            os.putenv("MALLOC_FILL_SPACE","1")
            os.putenv("MALLOC_STRICT_SIZE","1")
            os.putenv("MALLOC_CHECK_HEADER","1")
            os.putenv("MALLOC_PERMIT_INSANE_REQUESTS","1")
            run(args)
            exit(0)

        print("Debugging with standard Malloc")
        os.putenv("MallocLogFile","malloc.log")
        os.putenv("MallocGuardEdges","1")
        os.putenv("MallocStackLogging","1")
        os.putenv("MallocStackLoggingDirectory",".")
        os.putenv("MallocScribble","1")
        os.putenv("MallocCheckHeapStart","10")
        os.putenv("MallocCheckHeapEach","10")
        os.putenv("MallocCheckHeapAbort","1")
        os.putenv("MallocErrorAbort","1")
        os.putenv("MallocCorruptionAbort","1")
        run_and_analyze()
        exit(0)

    if args.gprof:
        call(['make','clean'])
        call(['make','CFLAGS=-pg','CXXFLAGS=-pg','LDFLAGS=-pg'])
        outdir = run_and_analyze()
        call(['gprof',program,"gmon.out"],stdout=open(outdir+"/GPROF.txt","w"))

    if args.diff:
        if len(args.diff)!=2:
            raise ValueError("--diff requires two arguments")
        diff(args.diff[0],args.diff[1])
        exit(0)

    if args.sort:
        for s in args.sort:
            sort_outdir(s)
        exit(0)

    if args.tune:
        print("tune_jobs_start: ",tune_jobs_start)
        print("tune_jobs_end  : ",tune_jobs_end  )
        print("tune_jobs_step : ",tune_jobs_step )

        print("tune_pagesize_start : ",tune_pagesize_start )
        print("tune_pagesize_end : ",tune_pagesize_end )
        print("tune_pagesize_step : ",tune_pagesize_step )

        print("tune_marginsize_start: ",tune_marginsize_start)
        print("tune_marginsize_end: ",tune_marginsize_end)
        print("tune_marginsize_step: ",tune_marginsize_step)

        job_steps = 1+(tune_jobs_end-tune_jobs_start)/tune_jobs_step
        pagesize_steps = 1+(tune_pagesize_end-tune_pagesize_start)/tune_pagesize_step
        marginsize_steps = 1+(tune_marginsize_end-tune_marginsize_start)/tune_marginsize_step
        print("Total anticipated runs: {}".format(job_steps*pagesize_steps*marginsize_steps))

        def run_with_parms(jobs,pagesize,margin):
            args.jobs = jobs
            args.pagesize = pagesize
            args.margin = margin
            clear_cache()
            outdir = make_outdir(args.outdir+"-{}-{}-{}".format(jobs,pagesize,margin))
            clear_cache()
            run_outdir(outdir)
            ofn = "report-{}-{}-{}.xml".format(j,p,m)
            os.rename(outdir+"/report.xml",ofn)

        for p in range(tune_pagesize_start,tune_pagesize_end+1,tune_pagesize_step):
            for m in range(tune_marginsize_start,tune_marginsize_end+1,tune_marginsize_step):
                run_with_parms(None,p,m)

        for j in range(tune_jobs_start,tune_jobs_end+1,tune_jobs_step):
            run_with_parms(j,None,None)
            
        exit(0)
        
    run_and_analyze()


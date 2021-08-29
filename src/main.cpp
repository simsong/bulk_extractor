/*
 * main.cpp
 *
 * The main() for bulk_extractor.
 * This has all of the code and global variables that aren't needed when BE is running as a library.
 */

#include "config.h"

#include <ctype.h>
#include <fcntl.h>
#include <set>
#include <setjmp.h>
#include <vector>
#include <queue>
#include <unistd.h>
#include <cctype>

#ifdef HAVE_MCHECK
#include <mcheck.h>
#else
void mtrace(){}
void muntrace(){}
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

// Open standard input in binary mode by default on Win32.
// See http://gnuwin32.sourceforge.net/compile.html for more
#ifdef WIN32
int _CRT_fmode = _O_BINARY;
#endif

#include "dfxml_cpp/src/dfxml_writer.h"
#include "dfxml_cpp/src/hash_t.h"  // needs config.h

#include "be13_api/aftimer.h"
#include "be13_api/scanner_params.h"
#include "be13_api/scanner_set.h"
#include "be13_api/utils.h"             // needs config.h
#include "be13_api/word_and_context_list.h"

#include "findopts.h"
#include "image_process.h"
#include "phase1.h"
#include "path_printer.h"

/* Bring in the definitions  */
#include "bulk_extractor_scanners.h"
#include "bulk_extractor_restarter.h"

/**
 * Output the #defines for our debug parameters. Used by the automake system.
 */
[[noreturn]] void debug_help()
{
    puts("#define DEBUG_PEDANTIC    0x0001	// check values more rigorously");
    puts("#define DEBUG_PRINT_STEPS 0x0002      // prints as each scanner is started");
    puts("#define DEBUG_SCANNER     0x0004	// dump all feature writes to stderr");
    puts("#define DEBUG_NO_SCANNERS 0x0008      // do not run the scanners ");
    puts("#define DEBUG_DUMP_DATA   0x0010	// dump data as it is seen ");
    puts("#define DEBUG_INFO        0x0040	// print extra info");
    puts("#define DEBUG_EXIT_EARLY  1000	// just print the size of the volume and exis ");
    puts("#define DEBUG_ALLOCATE_512MiB 1002	// Allocate 512MiB, but don't set any flags ");
    exit(1);
}

/****************************************************************
 *** Usage for the stand-alone program
 ****************************************************************/

static void usage(const char *progname, scanner_set &ss)
{
    Phase1::Config cfg;   // get a default config

    std::cout << "bulk_extractor version " PACKAGE_VERSION " " << /* svn_revision << */ "\n";
    std::cout << "Usage: " << progname << " [options] imagefile\n";
    std::cout << "  runs bulk extractor and outputs to stdout a summary of what was found where\n";
    std::cout << "\n";
    std::cout << "Required parameters:\n";
    std::cout << "   imagefile     - the file to extract\n";
    std::cout << " or  -R filedir  - recurse through a directory of files\n";
    std::cout << "   -o outdir    - specifies output directory. Must not exist.\n";
    std::cout << "                  bulk_extractor creates this directory.\n";
    std::cout << "Options:\n";
    std::cout << "   -i           - INFO mode. Do a quick random sample and print a report.\n";
    std::cout << "   -b banner.txt- Add banner.txt contents to the top of every output file.\n";
    std::cout << "   -r alert_list.txt  - a file containing the alert list of features to alert\n";
    std::cout << "                       (can be a feature file or a list of globs)\n";
    std::cout << "                       (can be repeated.)\n";
    std::cout << "   -w stop_list.txt   - a file containing the stop list of features (white list\n";
    std::cout << "                       (can be a feature file or a list of globs)s\n";
    std::cout << "                       (can be repeated.)\n";
    std::cout << "   -F <rfile>   - Read a list of regular expressions from <rfile> to find\n";
    std::cout << "   -f <regex>   - find occurrences of <regex>; may be repeated.\n";
    std::cout << "                  results go into find.txt\n";
    std::cout << "   -q           - quiet - no status output (changed in v2.0).\n";
    std::cout << "   -s frac[:passes] - Set random sampling parameters\n";
    std::cout << "\nTuning parameters:\n";
    //    std::cout << "   -C NN        - specifies the size of the context window (default " << feature_recorder::context_window_default << ")\n";
    std::cout << "   -S fr:<name>:window=NN   specifies context window for recorder to NN\n";
    std::cout << "   -S fr:<name>:window_before=NN  specifies context window before to NN for recorder\n";
    std::cout << "   -S fr:<name>:window_after=NN   specifies context window after to NN for recorder\n";
    std::cout << "   -G NN        - specify the page size (default " << cfg.opt_pagesize << ")\n";
    std::cout << "   -g NN        - specify margin (default " <<cfg.opt_marginsize << ")\n";
    std::cout << "   -j NN        - Number of analysis threads to run (default " << std::thread::hardware_concurrency() << ")\n";
    std::cout << "   -J           - no threading: read and process data in the primary thread.\n";
    std::cout << "   -M nn        - sets max recursion depth (default " << scanner_config::DEFAULT_MAX_DEPTH << ")\n";
    std::cout << "   -m <max>     - maximum number of minutes to wait after all data read\n";
    std::cout << "                  default is " << cfg.max_bad_alloc_errors << "\n";
    std::cout << "\nPath Processing Mode:\n";
    std::cout << "   -p <path>/f  - print the value of <path> with a given format.\n";
    std::cout << "                  formats: r = raw; h = hex.\n";
    std::cout << "                  Specify -p - for interactive mode.\n";
    std::cout << "                  Specify -p -http for HTTP mode.\n";
    std::cout << "\nParallelizing:\n";
    std::cout << "   -Y <o1>      - Start processing at o1 (o1 may be 1, 1K, 1M or 1G)\n";
    std::cout << "   -Y <o1>-<o2> - Process o1-o2\n";
    std::cout << "   -A <off>     - Add <off> to all reported feature offsets\n";
    std::cout << "\nDebugging:\n";
    std::cout << "   -h           - print this message\n";
    std::cout << "   -H           - print detailed info on the scanners\n";
    std::cout << "   -V           - print version number\n";
    std::cout << "   -z nn        - start on page nn\n";
    std::cout << "   -dN          - debug mode (see source code)\n";
    std::cout << "   -Z           - zap (erase) output directory\n";
    std::cout << "\nControl of Scanners:\n";
    std::cout << "   -P <dir>     - Specifies a plugin directory\n";
    std::cout << "             Default dirs include /usr/local/lib/bulk_extractor /usr/lib/bulk_extractor and\n";
    std::cout << "             BE_PATH environment variable\n";
    std::cout << "   -e <scanner>  enables <scanner> -- -e all   enables all\n";
    std::cout << "   -x <scanner>  disable <scanner> -- -x all   disables all\n";
    std::cout << "   -E <scanner>    - turn off all scanners except <scanner>\n";
    std::cout << "                     (Same as -x all -e <scanner>)\n";
    std::cout << "          note: -e, -x and -E commands are executed in order\n";
    std::cout << "              e.g.: '-E gzip -e facebook' runs only gzip and facebook\n";
    std::cout << "   -S name=value - sets a bulk extractor option name to be value\n";
    std::cout << "\n";
    ss.info_scanners(std::cerr, false, true, 'e', 'x');
#ifdef HAVE_LIBEWF
    std::cout << "                  HAS SUPPORT FOR E01 FILES\n";
#endif
#ifdef HAVE_EXIV2
    std::cout << "                  EXIV2 ENABLED\n";
#endif
#ifdef HAVE_LIBLIGHTGREP
    std::cout << "                  LIGHTGREP ENABLED\n";
#endif
    std::cout << "\n";
}


[[noreturn]] void throw_FileNotFoundError(const std::string &fname)
{
    std::cerr << "Cannot open: " << fname << "\n";
    throw std::runtime_error("Cannot open file");
}

/**
 * scaled_stoi64:
 * Like a normal stoi, except it can handle modifies k, m, and g
 */


/*
 * Make sure that the filename provided is sane.
 * That is, do not allow the analysis of a *.E02 file...
 */
void validate_path(const std::filesystem::path fn)
{
    if (!std::filesystem::exists(fn)){
        std::cerr << "file does not exist: " << fn << "\n";
        throw std::runtime_error("file not found.");
    }
    if (fn.extension()=="E02" || fn.extension()=="e02"){
        std::cerr << "Error: invalid file name\n";
        std::cerr << "Do not use bulk_extractor to process individual EnCase files.\n";
        std::cerr << "Instead, just run bulk_extractor with FILENAME.E01\n";
        std::cerr << "The other files in an EnCase multi-volume archive will be opened\n";
        std::cerr << "automatically.\n";
        throw std::runtime_error("run on E02.");
    }
}

/**
 * Create the dfxml output
 */

std::string be_hash_name {"sha1"};
static void add_if_present(std::vector<std::string> &scanner_dirs,const std::string &dir)
{
    if (access(dir.c_str(),O_RDONLY) == 0){
        scanner_dirs.push_back(dir);
    }
}

[[noreturn]] void notify_thread(scanner_set *ssp, aftimer *master_timer)
{
    while(true){
        if (ssp) {
            time_t rawtime = time (0);
            struct tm *timeinfo = localtime (&rawtime);
            std::cerr << asctime(timeinfo) << "\n";
            std::map<std::string,std::string> stats = ssp->get_realtime_stats();

            // get the times
            master_timer->lap();
            stats["elapsed_time"] = master_timer->elapsed_text();
            for(const auto &it : stats ){
                std::cerr << it.first << ": " << it.second << "\n";
            }
            std::cerr << "================================================================\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


int main(int argc,char **argv)
{
    mtrace();

    const char *progname = argv[0];
    const auto original_argc = argc;
    const auto original_argv = argv;

    word_and_context_list alert_list;		/* shold be flagged */
    word_and_context_list stop_list;		/* should be ignored */
    aftimer master_timer;

    scanner_config   sc;   // config for be13_api
    Phase1::Config   cfg;  // config for the image_processing system

    /* Options */
    std::string opt_path {};
    int         opt_zap = 0;
    int         opt_h = 0;
    int         opt_H = 0;
    std::string opt_sampling_params;
    bool        opt_write_feature_files = true;
    bool        opt_write_sqlite3     = false;

    /* Startup */
    setvbuf(stdout,0,_IONBF,0);		// don't buffer stdout
    //std::string command_line = dfxml_writer::make_command_line(argc,argv);
    std::vector<std::string> scanner_dirs; // where to look for scanners

    /* Add the default plugin_path */
    add_if_present(scanner_dirs,"/usr/local/lib/bulk_extractor");
    add_if_present(scanner_dirs,"/usr/lib/bulk_extractor");
    add_if_present(scanner_dirs,".");

    if (getenv("BE_PATH")) {
        std::vector<std::string> dirs = split(getenv("BE_PATH"),':');
        for(std::vector<std::string>::const_iterator it = dirs.begin(); it!=dirs.end(); it++){
            add_if_present(scanner_dirs,*it);
        }
    }

#ifdef WIN32
    setmode(1,O_BINARY);		// make stdout binary
#endif

    if (argc==1) opt_h=1;                // generate help if no arguments provided

    /* Process options */
    const std::string ALL { "all" };
    int ch;
    char *empty = strdup("");
    while ((ch = getopt(argc, argv, "A:B:b:C:d:E:e:F:f:G:g:HhiJj:M:m:o:P:p:qRr:S:s:VW:w:x:Y:z:Z")) != -1) {
        if (optarg==nullptr) optarg=empty;
        std::string arg = optarg!=ALL ? optarg : scanner_config::scanner_command::ALL_SCANNERS;
	switch (ch) {
	case 'A': sc.offset_add  = stoi64(optarg);break;
	case 'b': sc.banner_file = optarg; break;
	case 'C': sc.context_window_default = atoi(optarg);break;
	case 'd':
	{
            if (strcmp(optarg,"h")==0) debug_help();
	    cfg.debug = atoi(optarg);
            if (cfg.debug==0) cfg.debug=1;
	}
	break;
	case 'E':            /* Enable all scanners */
            sc.push_scanner_command( scanner_config::scanner_command::ALL_SCANNERS, scanner_config::scanner_command::DISABLE);
            sc.push_scanner_command( arg, scanner_config::scanner_command::ENABLE);
	    break;
	case 'e':           /* enable a spedcific scanner */
            sc.push_scanner_command(arg, scanner_config::scanner_command::ENABLE);
	    break;
	case 'F': FindOpts::get().Files.push_back(optarg); break;
	case 'f': FindOpts::get().Patterns.push_back(optarg); break;
	case 'G': cfg.opt_pagesize = scaled_stoi64(optarg); break;
	case 'g': cfg.opt_marginsize = scaled_stoi64(optarg); break;
        case 'i':
            std::cout << "info mode:\n";
            cfg.opt_info = true;
            break;
	case 'j': cfg.num_threads = atoi(optarg); break;
        case 'J': cfg.num_threads = 0; break;
	case 'M': sc.max_depth = atoi(optarg); break;
	case 'm': cfg.max_bad_alloc_errors = atoi(optarg); break;
	case 'o': sc.outdir = optarg;break;
	case 'P': scanner_dirs.push_back(optarg);break;
	case 'p': opt_path = optarg; break;
        case 'q': cfg.opt_quiet = true; break;
	case 'r':
	    if (alert_list.readfile(optarg)){
                throw_FileNotFoundError(optarg);
	    }
	    break;
	case 'R': cfg.opt_recurse = true; break;
	case 'S':
	{
	    std::vector<std::string> params = split(optarg,'=');
	    if (params.size()!=2){
		std::cerr << "Invalid paramter: " << optarg << "\n";
		exit(1);
	    }
	    sc.namevals[params[0]] = params[1];
	    continue;
	}
	case 's':
            opt_sampling_params = optarg;
            break;
	case 'V': std::cout << "bulk_extractor " << PACKAGE_VERSION << "\n"; exit (0);
	case 'W':
            fprintf(stderr,"-W has been deprecated. Specify with -S word_min=NN and -S word_max=NN\n");
            exit(1);
	    break;
	case 'w': if (stop_list.readfile(optarg)){
                throw_FileNotFoundError(optarg);
	    }
	    break;
	case 'x':
            sc.push_scanner_command( arg, scanner_config::scanner_command::DISABLE);
	    break;
	case 'Y': {
	    std::string optargs = optarg;
	    size_t dash = optargs.find('-');
	    if (dash==std::string::npos){
		cfg.opt_offset_start = stoi64(optargs);
	    } else {
		cfg.opt_offset_start = scaled_stoi64(optargs.substr(0,dash));
		cfg.opt_offset_end   = scaled_stoi64(optargs.substr(dash+1));
	    }
	    break;
	}
	case 'z': cfg.opt_page_start = stoi64(optarg);break;
	case 'Z': opt_zap=true;break;
	case 'H':
            opt_H++;
            continue;
	case 'h':
            opt_h++;
            continue;
	}
    }

    argc -= optind;
    argv += optind;

    /* Create a configuration that will be used to initialize the scanners */
    /* Make individual configuration options appear on the command line interface. */
    sc.get_config("debug_histogram_malloc_fail_frequency",&AtomicUnicodeHistogram::debug_histogram_malloc_fail_frequency,
                  "Set >0 to make histogram maker fail with memory allocations");
    sc.get_config("hash_alg",&be_hash_name,"Specifies hash algorithm to be used for all hash calculations");
    sc.get_config("write_feature_files",&opt_write_feature_files,"Write features to flat files");
    sc.get_config("write_feature_sqlite3",&opt_write_sqlite3,"Write feature files to report.sqlite3");
    sc.get_config("report_read_errors",&cfg.opt_report_read_errors,"Report read errors");

    /* Load all the scanners and enable the ones we care about */

    //plugin::load_scanner_directories(scanner_dirs,sc);
    if (opt_H || opt_h) {
        sc.outdir = scanner_config::NO_OUTDIR; // don't create outdir if we are getting help.
    }

    if (sc.outdir.empty()){
        std::cerr << "error: -o outdir must be specified\n";
        exit(1);
    }

    struct feature_recorder_set::flags_t f;
    scanner_set ss(sc, f, nullptr);     // make a scanner_set but with no XML writer. We will create it below
    ss.add_scanners(scanners_builtin);

    /* Print usage if necessary. Requires scanner set, but not commands applied.
     * This would create the outdir if one was specified.
     */
    if ( opt_h ) {
        usage(progname, ss);
        exit(1);
    }
    if ( opt_H ) {
        ss.info_scanners(std::cout, true, true, 'e', 'x');
        exit(1);
    }

    /* Remember if directory is clean or not */
    bool clean_start = directory_empty(sc.outdir);

    /* Applying the scanner commands will create the alert recorder. */
    try {
        ss.apply_scanner_commands();
    }
    catch (const scanner_set::NoSuchScanner &e) {
        std::cerr << "no such scanner: " << e.what() << "\n";
        exit(1);
    }

    /* Give an error if a find list was specified
     * but no scanner that uses the find list is enabled.
     */

    if (!FindOpts::get().empty()) {
        /* Look through the enabled scanners and make sure that
	 * at least one of them is a FIND scanner
	 */
        if (!ss.is_find_scanner_enabled()){
            throw std::runtime_error("find words are specified with -F but no find scanner is enabled.\n");
        }
    }

    /* The zap option wipes the contents of a directory, useful for debugging */
    if (opt_zap){
        for (const auto &entry : std::filesystem::directory_iterator( sc.outdir ) ) {
            std::cout << "erasing " << entry.path().string() << "\n";
            std::filesystem::remove( entry );
	}
        clean_start = true;
    }

    dfxml_writer *xreport = new dfxml_writer(sc.outdir / Phase1::REPORT_FILENAME, false); // do not make DTD
    ss.set_dfxml_writer( xreport );

    /* Start the clock */
    master_timer.start();

    /* Get image or directory */
    if (*argv == NULL) {
        if (cfg.opt_recurse) {
            std::cerr << "filedir not provided\n";
        } else {
            std::cerr << "imagefile not provided\n";
        }
        exit(1);
    }
    sc.input_fname = *argv;

    /* are we supposed to run the path printer? */
    if (opt_path.size() > 0){
	if (argc!=1) throw std::runtime_error("-p requires a single argument.");
        image_process *p = image_process::open( sc.input_fname, cfg.opt_recurse, cfg.opt_pagesize, cfg.opt_marginsize);
        path_printer pp(&ss, p, std::cout);
        if (opt_path=="-http" || opt_path=="--http"){
            pp.process_http(std::cin);
        } else if (opt_path=="-i" || opt_path=="-"){
            pp.process_interactive(std::cin);
        } else {
            pp.process_path(opt_path);
        }
	exit(0);
    }

    /* Open the image file (or the device) now.
     * We use *p because we don't know which subclass we will be getting.
     */

    image_process *p = image_process::open( sc.input_fname, cfg.opt_recurse, cfg.opt_pagesize, cfg.opt_marginsize);
    Phase1 phase1(cfg, *p, ss);

    /* Determine if this is the first time through or if the program was restarted.
     * Restart procedure: just press up-arrow and return to re-run the command in verbatim.
     */
    if (clean_start){
        /* First time running */
	/* Validate the args */
	if ( argc == 0 ) throw std::runtime_error("Clean start, but no disk image provided. Run with -h for help.");
        if ( argc > 1  ){
            throw std::runtime_error("Clean start, but too many arguments provided. Run with -h for help.");
        }
	validate_path(sc.input_fname);
    } else {
	/* Restarting */
        bulk_extractor_restarter r(sc, phase1);

        try {
            r.restart();
        }
        catch (bulk_extractor_restarter::CantRestart &e) {
            std::cerr << "Cannot restart from " << sc.outdir << "\n";
            exit(1);
        }
    }

    /* Create the DFXML file in the report directory.
     * If we are restarting, the dfxml file was renamed.
     */


    /* Determine the feature files that will be used from the scanners that were enabled */
    auto feature_file_names = ss.feature_file_list();
#if 0
    uint32_t flags = 0;
    if (stop_list.size()>0)        flags |= feature_recorder_set::CREATE_STOP_LIST_RECORDERS;
    if (opt_write_sqlite3)         flags |= feature_recorder_set::ENABLE_SQLITE3_RECORDERS;
    if (!opt_write_feature_files)  flags |= feature_recorder_set::DISABLE_FILE_RECORDERS;

    /* Create the feature_recorder_set */
    feature_recorder_set fs(flags, be_hash_name, sc.input_fname, sc.outdir);
    fs.init( feature_file_names );      // TODO: this should be in the initializer

    /* Enable histograms */
    if (opt_enable_histograms) plugin::add_enabled_scanner_histograms_to_feature_recorder_set(fs); // TODO: This should be in initializer
    plugin::scanners_init(fs);    // TODO: This should be in the initiazer

    fs.set_stop_list(&stop_list);
    fs.set_alert_list(&alert_list);

    /* Look for commands that impact per-recorders */
    for(scanner_info::config_t::const_iterator it=sc.namevals.begin();it!=sc.namevals.end();it++){
        /* see if there is a <recorder>: */
        std::vector<std::string> params = split(it->first,':');
        if (params.size()>=3 && params.at(0)=="fr"){
            feature_recorder &fr = fs.named_feature_recorder(params.at(1));
            const std::string &cmd = params.at(2);
            if (fr){
                if (cmd=="window")        fr->set_context_window(stoi64(it->second));
                if (cmd=="window_before") fr->set_context_window_before(stoi64(it->second));
                if (cmd=="window_after")  fr->set_context_window_after(stoi64(it->second));
            }
        }
        /* See if there is a scanner? */
    }
#endif

    /* provide documentation to the user; the DFXML information comes from elsewhere */
    if (!cfg.opt_quiet){
        std::cout << "bulk_extractor version: " << PACKAGE_VERSION << "\n";
#ifdef HAVE_GETHOSTNAME
        char hostname[1024];
        memset(hostname,0,sizeof(hostname));
        if (gethostname(hostname,sizeof(hostname)-1)==0){
            if (hostname[0]) std::cout << "Hostname: " << hostname << "\n";
        }
#endif
        std::cout << "Input file: " << sc.input_fname << "\n";
        std::cout << "Output directory: " << sc.outdir << "\n";
        std::cout << "Disk Size: " << p->image_size() << "\n";
        std::cout << "Scanners: ";
        for (auto const &it : ss.get_enabled_scanners()){
            std::cout << it << " ";
        }
        std::cout << "\n";

        if (cfg.num_threads>0){
            std::cout << "Threads: " << cfg.num_threads << "\n";
        } else {
            std::cout << "Threading Disabled\n";
        }
    }

    /*** PHASE 1 --- Run on the input image */
    new std::thread(&notify_thread, &ss, &master_timer);    // launch the notify thread
    ss.phase_scan();

#if 0
    if ( fs.flag_set(feature_recorder_set::ENABLE_SQLITE3_RECORDERS )) {
        fs.db_transaction_begin();
    }
#endif
    if (opt_sampling_params.size()>0){
        cfg.set_sampling_parameters(opt_sampling_params);
    }

    /* Go multi-threaded if requested */
    if (cfg.num_threads > 0){
        std::cout << "going multi-threaded...(" << cfg.num_threads << ")\n";
        ss.launch_workers(cfg.num_threads);
    } else {
        std::cout << "running single-threaded (DEBUG)...\n";

    }

    phase1.dfxml_write_create( original_argc, original_argv);
    xreport->xmlout("provided_filename", sc.input_fname); // save this information

    /* TODO: Load up phase1 seen_page_ideas if we are restarting */

    //if (cfg.debug & DEBUG_PRINT_STEPS) std::cerr << "DEBUG: STARTING PHASE 1\n";

    xreport->add_timestamp("phase1 start");

    std::cerr << "Calling check_previously_processed at one\n";


    phase1.phase1_run();
    ss.join();                          // wait for threads to come together

#if 0
    if ( fs.flag_set(feature_recorder_set::ENABLE_SQLITE3_RECORDERS )) {
        fs.db_transaction_commit();
    }
#endif
    xreport->add_timestamp("phase1 end");
    if (phase1.image_hash.size() > 0 ){
        std::cout << "Hash of Disk Image: " << phase1.image_hash << "\n";
    }

    /*** PHASE 2 --- Shutdown ***/
    if (!cfg.opt_quiet) std::cout << "Phase 2. Shutting down scanners\n";
    xreport->add_timestamp("phase2 start");
    ss.shutdown();
    xreport->add_timestamp("phase2 end");
    master_timer.stop();

    /*** PHASE 3 ---  report and then print final usage information ***/
    xreport->push("report");
    xreport->xmlout("total_bytes",phase1.total_bytes);
    xreport->xmlout("elapsed_seconds",master_timer.elapsed_seconds());
    xreport->xmlout("max_depth_seen",ss.get_max_depth_seen());
    xreport->xmlout("dup_bytes_encountered",ss.get_dup_bytes_encountered());
    ss.dump_scanner_stats();
    ss.dump_name_count_stats();
    xreport->pop("report");

    xreport->add_rusage();
    xreport->pop("dfxml");			// bulk_extractor
    xreport->close();

    if (cfg.opt_quiet==0){
        float mb_per_sec = (phase1.total_bytes / 1000000.0) / master_timer.elapsed_seconds();

        std::cout.precision(4);
        std::cout << "Elapsed time: " << master_timer.elapsed_seconds() << " sec." << std::endl
                  << "Total MB processed: " << int(phase1.total_bytes / 1000000) << std::endl
                  << "Overall performance: " << mb_per_sec << " << MBytes/sec ";
        if (cfg.num_threads>0){
            std::cout << mb_per_sec/cfg.num_threads << " (MBytes/sec/thread)\n";
        }
        std::cout << "sbufs created:   " << sbuf_t::sbuf_total << std::endl;
        std::cout << "sbufs remaining: " << sbuf_t::sbuf_count << std::endl;
    }

    try {
        feature_recorder &fr = ss.fs.named_feature_recorder("email");
        std::cout << "Total " << fr.name << " features found: " << fr.features_written << std::endl;
    }
    catch (const feature_recorder_set::NoSuchFeatureRecorder &e) {
        std::cout << "Did not scan for email addresses." << std::endl;
    }

    muntrace();
    exit(0);
}

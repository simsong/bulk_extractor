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
#include <cstdlib>

#ifdef HAVE_MCHECK
#include <mcheck.h>
#else
void mtrace(){}
void muntrace(){}
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#ifdef HAVE_TERM_H
#include <term.h>
#endif

// Open standard input in binary mode by default on Win32.
// See http://gnuwin32.sourceforge.net/compile.html for more
#ifdef WIN32
int _CRT_fmode = _O_BINARY;
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
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

void usage(const char *progname, scanner_set &ss)
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
    std::cout << "   -1           - bulk_extractor v1.x legacy mode\n";
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

struct notify_opts {
    scanner_set *ssp;
    aftimer *master_timer;
    std::atomic<double> *fraction_done;
    bool opt_legacy;
};

[[noreturn]] void notify_thread(struct notify_opts *o)
{
    assert(o->ssp != nullptr);
    const char *cl="";
    const char *ho="";
    const char *ce="";
    const char *cd="";
    int cols = 80;
#ifdef HAVE_LIBTERMCAP
    char buf[65536], *table=buf;
    cols = tgetnum("co");
    if (!o->opt_legacy) {
        const char *str = ::getenv("TERM");
        if (!str){
            std::cerr << "Warning: TERM environment variable not set." << std::endl;
        } else {
            switch (tgetent(buf, str)) {
            case 0:
                std::cerr << "Warning: No terminal entry '" << str << "'. " << std::endl;
                break;
            case -1:
                std::cerr << "Warning: terminfo database culd not be found." << std::endl;
                break;
            case 1: // success
                ho = tgetstr("ho", &table);   // home
                cl = tgetstr("cl", &table);   // clear screen
                ce = tgetstr("ce", &table);   // clear to end of line
                cd = tgetstr("cd", &table);   // clear to end of screen
                break;
            }
        }
    }
#endif

    std::cout << cl;                    // clear screen
    while(true){

        // get screen size change if we can!
#if defined(HAVE_IOCTL) && defined(HAVE_STRUCT_WINSIZE_WS_COL)
        struct winsize ws;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws)==0){
            cols = ws.ws_col;
        }
#endif

        time_t rawtime = time (0);
        struct tm timeinfo = *(localtime(&rawtime));
        std::map<std::string,std::string> stats = o->ssp->get_realtime_stats();

        // get the times
        o->master_timer->lap();
        stats["elapsed_time"] = o->master_timer->elapsed_text();
        if (o->fraction_done) {
            double done = *o->fraction_done;
            stats["fraction_read"] = std::to_string(done * 100) + std::string(" %");
            stats["estimated_time_remaining"] = o->master_timer->eta_text(done);
            stats["estimated_date_completion"] = o->master_timer->eta_date(done);

            // print the legacy status
            if(o->opt_legacy) {
                char buf1[64], buf2[64];
                snprintf(buf1, sizeof(buf1), "%2d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
                snprintf(buf2, sizeof(buf2), "(%.2f%%)", done * 100);
                uint64_t max_offset = strtoll( stats[ scanner_set::MAX_OFFSET ].c_str() , nullptr, 10);
                std::cout << buf1 << " Offset " << max_offset / (1000*1000) << "MB "
                          << buf2 << " Done in " << stats["estimated_time_remaining"]
                          << " at " << stats["estimated_time_completion"] << std::endl;
            }
        }
        if (!o->opt_legacy) {
            std::cout << ho << "bulk_extractor      " << asctime(&timeinfo) << "  " << std::endl;
            for(const auto &it : stats ){
                std::cout << it.first << ": " << it.second;
                if (ce[0] ){
                    std::cout << ce;
                } else {
                    // Space out to the 50 column to erase any junk
                    int spaces = 50 - (it.first.size() + it.second.size());
                    for(int i=0;i<spaces;i++){
                        std::cout << " ";
                    }
                }
                std::cout << std::endl;
            }
            if( o->fraction_done ){
                if (cols>10){
                    double done = *o->fraction_done;
                    int before = (cols - 3) * done;
                    int after  = (cols - 3) * (1.0 - done);
                    std::cout << std::string(before,'=') << '>' << std::string(after,'.') << '|' << ce << std::endl;
                }
            }
            std::cout << cd << std::endl << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int bulk_extractor_main(int argc,char * const *argv)
{
    mtrace();

    const char *progname = argv[0];
    const auto original_argc = argc;
    const auto original_argv = argv;

    word_and_context_list alert_list;		/* shold be flagged */
    word_and_context_list stop_list;		/* should be ignored */
    std::atomic<double>  fraction_done = 0;                  /* a callback of sorts */
    aftimer master_timer;

    scanner_config   sc;   // config for be13_api
    Phase1::Config   cfg;  // config for the image_processing system

    cfg.fraction_done = &fraction_done;

    /* Options */
    std::string opt_path {};
    int         opt_zap = 0;
    int         opt_h = 0;
    int         opt_H = 0;
    std::string opt_sampling_params;
    //bool        opt_write_feature_files = true;
    //bool        opt_write_sqlite3     = false;

    /* Startup */
    setvbuf(stdout,0,_IONBF,0);		// don't buffer stdout
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
    while ((ch = getopt(argc, argv, "A:B:b:C:d:E:e:F:f:G:g:HhiJj:M:m:o:P:p:qRr:S:s:VW:w:x:Y:z:Z1")) != -1) {
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
        case '1': cfg.opt_legacy = true; break;
	case 'H':
            opt_H++;
            continue;
	case 'h':
            opt_h++;
            continue;
	}
    }

    /* Legacy mode if stdout is not a tty */
#ifdef HAVE_ISATTY
    if (!isatty(1)){
        cfg.opt_legacy = true;
    }
#endif

    argc -= optind;
    argv += optind;

    /* Create a configuration that will be used to initialize the scanners */
    /* Make individual configuration options appear on the command line interface. */
    sc.get_global_config("debug_histogram_malloc_fail_frequency",
                         &AtomicUnicodeHistogram::debug_histogram_malloc_fail_frequency,
                         "Set >0 to make histogram maker fail with memory allocations");
    sc.get_global_config("hash_alg",&be_hash_name,"Specifies hash algorithm to be used for all hash calculations");
    //sc.get_global_config("write_feature_files",&opt_write_feature_files,"Write features to flat files");
    //sc.get_global_config("write_feature_sqlite3",&opt_write_sqlite3,"Write feature files to report.sqlite3");
    sc.get_global_config("report_read_errors",&cfg.opt_report_read_errors,"Report read errors");

    /* Load all the scanners and enable the ones we care about */

    //plugin::load_scanner_directories(scanner_dirs,sc);
    if (opt_H || opt_h) {
        sc.outdir = scanner_config::NO_OUTDIR; // don't create outdir if we are getting help.
    }

    struct feature_recorder_set::flags_t f;
    scanner_set ss(sc, f, nullptr);     // make a scanner_set but with no XML writer. We will create it below
    ss.add_scanners(scanners_builtin);

    /* Get image or directory */
    if (argc==0 || *argv == nullptr) {
        if (cfg.opt_recurse) {
            std::cerr << "filedir not provided\n";
        } else {
            std::cerr << "imagefile not provided\n";
        }
        usage(progname, ss);
        exit(1);
    }
    sc.input_fname = *argv;

    if (sc.outdir.empty()){
        std::cerr << "error: -o outdir must be specified\n";
        usage(progname, ss);
        exit(1);
    }

    /* Print usage if necessary. Requires scanner set, but not commands applied.
     */
    if ( opt_h ) {
        usage(progname, ss);
        exit(1);
    }
    if ( opt_H ) {
        ss.info_scanners(std::cout, true, true, 'e', 'x');
        exit(1);
    }

    /* The zap option wipes the contents of a directory, useful for debugging */
    if (opt_zap){
        for (const auto &entry : std::filesystem::recursive_directory_iterator( sc.outdir ) ) {
            if (! std::filesystem::is_directory(entry.path())){
                std::cout << "erasing " << entry.path().string() << "\n";
                std::filesystem::remove( entry );
            }
	}
    }

    bool clean_start = opt_zap || directory_empty(sc.outdir);

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

    if (clean_start==false){
	/* Restarting */
        bulk_extractor_restarter r(sc,cfg);
        r.restart();                    // load the restart file and rename report.xml
    }

    image_process *p = image_process::open( sc.input_fname, cfg.opt_recurse, cfg.opt_pagesize, cfg.opt_marginsize);

    /* are we supposed to run the path printer? */
    if (opt_path.size() > 0){
	if (argc!=1) throw std::runtime_error("-p requires a single argument.");
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

    dfxml_writer *xreport = new dfxml_writer(sc.outdir / Phase1::REPORT_FILENAME, false); // do not make DTD
    ss.set_dfxml_writer( xreport );
    /* Start the clock */
    master_timer.start();

    Phase1 phase1(cfg, *p, ss);

    /* Validate the args */
    if ( argc == 0 ) throw std::runtime_error("No disk image provided. Run with -h for help.");
    if ( argc > 1  ){
            throw std::runtime_error("Too many arguments provided. Run with -h for help.");
    }
    validate_path(sc.input_fname);

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

#endif

    /* provide documentation to the user; the DFXML information comes from elsewhere */
    if (!cfg.opt_quiet){
        std::cout << "bulk_extractor version: " << PACKAGE_VERSION << "\n";
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
    struct notify_opts o;
    o.ssp = &ss;
    o.master_timer = &master_timer;
    o.fraction_done = &fraction_done;
    o.opt_legacy = cfg.opt_legacy;
    new std::thread(&notify_thread, &o);    // launch the notify thread
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
    xreport->add_timestamp("phase1 start");

    std::cerr << "Calling check_previously_processed at one\n";

    try {
        phase1.phase1_run();
        ss.join();                          // wait for threads to come together
    }
    catch (const feature_recorder::DiskWriteError &e) {
        std::cerr << "Disk write error during Phase 1 (scanning). Disk is probably full." << std::endl
                  << "Remove extra files and restart bulk_extractor with the exact same command line to continue." << std::endl;
        exit(1);
    }

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
    try {
        ss.shutdown();
    }
    catch (const feature_recorder::DiskWriteError &e) {
        std::cerr << "Disk write error during Phase 2 (histogram making). Disk is probably full." << std::endl
                  << "Remove extra files and restart bulk_extractor with the exact same command line to continue." << std::endl;
        exit(1);
    }

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

        std::cout << "All Threads Finished!\n";
        std::cout.precision(4);
        std::cout << "Elapsed time: " << master_timer.elapsed_seconds() << " sec." << std::endl
                  << "Total MB processed: " << int(phase1.total_bytes / 1000000) << std::endl
                  << "Overall performance: " << mb_per_sec << " << MBytes/sec ";
        if (cfg.num_threads>0){
            std::cout << mb_per_sec/cfg.num_threads << " (MBytes/sec/thread)\n";
        }
        std::cout << "sbufs created:   " << sbuf_t::sbuf_total << std::endl;
        std::cout << "sbufs unaccounted: " << sbuf_t::sbuf_count << " (should be 0) " << std::endl;
    }

    try {
        feature_recorder &fr = ss.fs.named_feature_recorder("email");
        std::cout << "Total " << fr.name << " features found: " << fr.features_written << std::endl;
    }
    catch (const feature_recorder_set::NoSuchFeatureRecorder &e) {
        std::cout << "Did not scan for email addresses." << std::endl;
    }

    muntrace();
    return(0);
}

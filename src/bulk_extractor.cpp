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

#if defined(HAVE_MCHECK) && defined(HAVE_MCHECK_H)
#include <mcheck.h>
#else
void mtrace(){}
void muntrace(){}
#endif

// Open standard input in binary mode by default on Win32.
// See http://gnuwin32.sourceforge.net/compile.html for more
#ifdef _WIN32
int _CRT_fmode = _O_BINARY;
#endif

#include "dfxml_cpp/src/dfxml_writer.h"
#include "dfxml_cpp/src/hash_t.h"  // needs config.h

#include "be13_api/aftimer.h"
#include "be13_api/scanner_params.h"
#include "be13_api/scanner_set.h"
#include "be13_api/utils.h"             // needs config.h
#include "be13_api/word_and_context_list.h"
#include "be13_api/path_printer.h"

#include "bulk_extractor.h"
#include "findopts.h"
#include "image_process.h"
#include "phase1.h"

/* Bring in the definitions  */
#include "bulk_extractor_scanners.h"
#include "bulk_extractor_restarter.h"

#include "cxxopts.hpp"

/**
 * Output the #defines for our debug parameters. Used by the automake system.
 */
[[noreturn]] void debug_help()
{
    puts( "#define DEBUG_PEDANTIC    0x0001	// check values more rigorously" );
    puts( "#define DEBUG_PRINT_STEPS 0x0002      // prints as each scanner is started" );
    puts( "#define DEBUG_SCANNER     0x0004	// dump all feature writes to stderr" );
    puts( "#define DEBUG_NO_SCANNERS 0x0008      // do not run the scanners " );
    puts( "#define DEBUG_DUMP_DATA   0x0010	// dump data as it is seen " );
    puts( "#define DEBUG_INFO        0x0040	// print extra info" );
    puts( "#define DEBUG_EXIT_EARLY  1000	// just print the size of the volume and exis " );
    puts( "#define DEBUG_ALLOCATE_512MiB 1002	// Allocate 512MiB, but don't set any flags " );
    exit( 1);
}

/****************************************************************
 *** Usage for the stand-alone program
 ****************************************************************/

#if 0
{
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
    std::cout << "   -0           - Do not run notification thread\n";
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
}
#endif

[[noreturn]] void throw_FileNotFoundError( const std::string &fname )
{
    std::cerr << "Cannot open: " << fname << std::endl ;
    throw std::runtime_error( "Cannot open file" );
}

/**
 * scaled_stoi64:
 * Like a normal stoi, except it can handle modifies k, m, and g
 */


/*
 * Make sure that the filename provided is sane.
 * That is, do not allow the analysis of a *.E02 file...
 */
void validate_path( const std::filesystem::path fn)
{
    if ( !std::filesystem::exists( fn )){
        std::cerr << "file does not exist: " << fn << std::endl ;
        throw std::runtime_error( "file not found." );
    }
    if ( fn.extension()=="E02" || fn.extension()=="e02" ){
        std::cerr << "Error: invalid file name" << std::endl ;
        std::cerr << "Do not use bulk_extractor to process individual EnCase files." << std::endl ;
        std::cerr << "Instead, just run bulk_extractor with FILENAME.E01" << std::endl ;
        std::cerr << "The other files in an EnCase multi-volume archive will be opened" << std::endl ;
        std::cerr << "automatically." << std::endl ;
        throw std::runtime_error( "run on E02." );
    }
}

/**
 * Create the dfxml output
 */

std::string be_hash_name {"sha1"};
static void add_if_present( std::vector<std::string> &scanner_dirs,const std::string &dir)
{
    if ( access( dir.c_str(),O_RDONLY) == 0){
        scanner_dirs.push_back( dir);
    }
}

int bulk_extractor_main( std::ostream &cout, std::ostream &cerr, int argc,char * const *argv)
{
    mtrace();
    const auto original_argc = argc;
    const auto original_argv = argv;

    word_and_context_list alert_list;       /* should be flagged */
    word_and_context_list stop_list;        /* should be ignored */
    std::atomic<double>  fraction_done = 0; /* a shared memory space */
    aftimer master_timer;

    Phase1::Config   cfg;  // config for the image_processing system

    cfg.fraction_done = &fraction_done;

#ifdef USE_SQLITE
    bool        opt_write_feature_files = true;
    bool        opt_write_sqlite3     = false;
#endif

    /* Startup */
    setvbuf( stdout,0,_IONBF,0);		// don't buffer stdout
    std::vector<std::string> scanner_dirs; // where to look for scanners

    /* Add the default plugin_path */
    add_if_present( scanner_dirs,"/usr/local/lib/bulk_extractor" );
    add_if_present( scanner_dirs,"/usr/lib/bulk_extractor" );
    add_if_present( scanner_dirs,"." );

    if ( getenv( "BE_PATH" )) {
        std::vector<std::string> dirs = split( getenv( "BE_PATH" ),':');
        for( std::vector<std::string>::const_iterator it = dirs.begin(); it!=dirs.end(); it++){
            add_if_present( scanner_dirs,*it);
        }
    }

#ifdef _WIN32
    setmode( 1,O_BINARY);		// make stdout binary
#endif

    /* Process options */
    const std::string ALL { "all" };


    /* 2021-09-13 - slg - option processing rewritten to use cxxopts */
    std::string bulk_extractor_help( "A high-performance flexible digital forensics program." );
    std::string image_name_help( "Name of image to scan (or directory if -r is provided)" );
#ifdef HAVE_LIBEWF
    image_name_help += " (May be a E01 file )";
#endif
#ifdef HAVE_EXIV2
    bulk_extractor_help += " (Includes EXVI2 Support)";
#endif
#ifdef HAVE_LIBLIGHTGREP
    bulk_extractor_help += " (Includes LightGrep support)";
#endif

    scanner_config   sc;   // config for be13_api
    cxxopts::Options options( "bulk_extractor", bulk_extractor_help.c_str());
    options.set_width( notify_thread::terminal_width( 80 ));
    options.add_options()
    ("image_name", image_name_help.c_str(), cxxopts::value<std::string>())
        ("A,offset_add", "Offset added (in bytes) to feature locations", cxxopts::value<int64_t>()->default_value("0"))
        ("b,banner_file", "Path of file whose contents are prepended to top of all feature files",cxxopts::value<std::string>())
	("C,context_window", "Size of context window reported in bytes",
         cxxopts::value<int>()->default_value(std::to_string(sc.context_window_default)))
        ("d,debug", "enable debugging", cxxopts::value<int>()->default_value("1"))
        ("D,debug_help", "help on debugging")
        ("E,enable_exclusive", "disable all scanners except the one specified. Same as -x all -E scanner.", cxxopts::value<std::string>())
        ("e,enable",   "enable a scanner", cxxopts::value<std::vector<std::string>>())
        ("x,disable",  "disable a scanner", cxxopts::value<std::vector<std::string>>())
        ("f,find",     "search for a pattern", cxxopts::value<std::vector<std::string>>())
        ("F,find_file", "read patterns to search from a file", cxxopts::value<std::vector<std::string>>())
        ("G,pagesize",   "page size in bytes", cxxopts::value<std::string>()->default_value(std::to_string(cfg.opt_pagesize )))
        ("g,marginsize", "margin size in bytes", cxxopts::value<std::string>()->default_value(std::to_string(cfg.opt_pagesize )))
        ("i,info",       "info mode")
        ("j,threads",    "number of threads", cxxopts::value<int>()->default_value(std::to_string(cfg.num_threads)))
        ("J,no_threads",  "read and process data in the primary thread")
	("M,max_depth",   "max recursion depth", cxxopts::value<int>()->default_value(std::to_string(scanner_config::DEFAULT_MAX_DEPTH)))
	("m,max_bad_alloc_errors", "max bad allocation errors", cxxopts::value<int>()->default_value(std::to_string(cfg.max_bad_alloc_errors)))
	("max_minute_wait", "maximum number of minutes to wait until all data are read", cxxopts::value<int>()->default_value(std::to_string(60)))
        ("o,outdir",        "output directory", cxxopts::value<std::string>())
        ("P,scanner_dir",
         "directories for scanner shared libraries. Multiple directories can be specified. "
         "Default directories include /usr/local/lib/bulk_extractor, /usr/lib/bulk_extractor "
         "and any directories specified in the BE_PATH environment variable.", cxxopts::value<std::vector<std::string>>())
        ("p,path",         "print the value of <path>[:length][/h][/r] with optional length, hex output, or raw output.", cxxopts::value<std::string>())
        ("q,quit",         "no status output")
        ("r,alert_list",   "file to read alert list from", cxxopts::value<std::string>())
        ("R,recurse",      "treat image file as a directory to recursively explore")
        ("S,set",          "set a name=value option", cxxopts::value<std::vector<std::string>>())
        ("s,sampling",     "random sampling parameter frac[:passes]", cxxopts::value<std::string>())
        ("V,version",      "Display PACKAGE_VERSION (currently) " PACKAGE_VERSION)
        ("w,stop_list",    "file to read stop list from", cxxopts::value<std::string>())
        ("Y,scan",         "specify <start>[-end] of area on disk to scan", cxxopts::value<std::string>())
        ("z,page_start",   "specify a starting page number", cxxopts::value<int>())
        ("Z,zap",          "wipe the output directory (recursively) before starting")
        ("0,no_notify",    "disable real-time notification")
        ("1,version1",    "version 1.0 notification (console-output)")
        ("H,info_scanners", "report information about each scanner")
        ("h,help",          "print help screen")
        ;

    options.positional_help( "image_name" );
    options.parse_positional( "image_name" );
    auto result = options.parse( argc, argv);
    if ( result.count( "debug_help" )){ debug_help(); return 3;}

    sc.offset_add  = result["offset_add"].as<int64_t>();
    sc.context_window_default = result["context_window"].as<int>();
    cfg.debug = result["debug"].as<int>();

    try {
        sc.banner_file = result["banner_file"].as<std::string>();
    } catch ( cxxopts::option_has_no_value_exception &e ) { }

    if ( result.count( "enable_exclusive" )) {
        sc.push_scanner_command( scanner_config::scanner_command::ALL_SCANNERS, scanner_config::scanner_command::DISABLE);
        sc.push_scanner_command( result["enable_exclusive"].as<std::string>(), scanner_config::scanner_command::ENABLE);
    }

    try {
        for ( const auto &name : result["enable"].as<std::vector<std::string>>() ) {
            sc.push_scanner_command( name, scanner_config::scanner_command::ENABLE);
        }
    } catch ( cxxopts::option_has_no_value_exception &e ) { }


    try {
        for ( const auto &name : result["disable"].as<std::vector<std::string>>() ) {
            sc.push_scanner_command( name, scanner_config::scanner_command::DISABLE);
        }
    } catch ( cxxopts::option_has_no_value_exception &e ) { }

    try {
        for ( const auto &it : result["find"].as<std::vector<std::string>>() ) {
            FindOpts::get().Patterns.push_back( it);
        }
    } catch ( cxxopts::option_has_no_value_exception &e ) { }


    try {
        for ( const auto &it : result["find_file"].as<std::vector<std::string>>() ) {
            FindOpts::get().Files.push_back( it);
        }
    } catch ( cxxopts::option_has_no_value_exception &e ) { }

    cfg.opt_pagesize   = scaled_stoi64( result["pagesize"].as<std::string>());
    cfg.opt_marginsize = scaled_stoi64( result["marginsize"].as<std::string>());
    cfg.opt_info       = result.count( "info" );

    try {
        cfg.num_threads    = result["threads"].as<int>();
    }  catch ( cxxopts::option_has_no_value_exception &e ) {
        cfg.num_threads = 0;
    }

    sc.max_depth             = result["max_depth"].as<int>();
    cfg.max_bad_alloc_errors = result["max_bad_alloc_errors"].as<int>();

    try {
        for ( const auto &it : result["scanner_dir"].as<std::vector<std::string>>() ) {
            scanner_dirs.push_back( it);break;
        }
    } catch ( cxxopts::option_has_no_value_exception &e ) { }


    cfg.opt_quiet = result.count( "quiet" );
    try {
        alert_list.readfile( result["alert_list"].as<std::string>() );
    } catch ( cxxopts::option_has_no_value_exception &e ) { }

    cfg.opt_recurse = result.count( "recurse" );

    try {
        for ( const auto &it : result["set"].as<std::vector<std::string>>() ) {
            std::vector<std::string> kv = split( it,'=');
            if ( kv.size()!=2) {
                cerr << "Invalid -S paramter: '" << it << "' must be key=value format" << std::endl ;
                return -1;
            }
            sc.namevals[kv[0]] = kv[1];
        }
    } catch ( cxxopts::option_has_no_value_exception &e ) { }


    try {
        cfg.set_sampling_parameters( result["sampling"].as<std::string>());
    } catch ( cxxopts::option_has_no_value_exception &e ) { }

    if ( result.count( "version" )) { cout << "bulk_extractor " << PACKAGE_VERSION << std::endl; return 0; }
    if ( result.count( "stop_list" )) stop_list.readfile( result["stop_list"].as<std::string>());
    try {
        std::string optargs = result["scan"].as<std::string>();
        size_t dash = optargs.find( '-');
        if ( dash==std::string::npos){
            cfg.opt_scan_start = stoi64( optargs);
        } else {
            cfg.opt_scan_start = scaled_stoi64( optargs.substr( 0,dash));
            cfg.opt_scan_end   = scaled_stoi64( optargs.substr( dash+1));
        }
    } catch ( cxxopts::option_has_no_value_exception &e ) { }


    try {
        cfg.opt_page_start = result["page_start"].as<int>();
    } catch ( cxxopts::option_has_no_value_exception &e ) { }

    cfg.opt_notification = ( result.count( "no_notify" )==0);
    cfg.opt_legacy = result.count( "version1" );

    /* Legacy mode if stdout is not a tty */
#ifdef HAVE_ISATTY
    if ( !isatty( 1)){
        cfg.opt_legacy = true;
    }
#endif

    /* Create a configuration that will be used to initialize the scanners */
    /* Make individual configuration options appear on the command line interface. */
    sc.get_global_config( "notify_rate", &cfg.opt_notify_rate, "seconds between notificaiton update" );
    sc.get_global_config( "debug_histogram_malloc_fail_frequency",
                         &AtomicUnicodeHistogram::debug_histogram_malloc_fail_frequency,
                         "Set >0 to make histogram maker fail with memory allocations" );
    sc.get_global_config( "hash_alg",&be_hash_name,"Specifies hash algorithm to be used for all hash calculations" );
    //sc.get_global_config( "write_feature_files",&opt_write_feature_files,"Write features to flat files" );
    //sc.get_global_config( "write_feature_sqlite3",&opt_write_sqlite3,"Write feature files to report.sqlite3" );
    sc.get_global_config( "report_read_errors",&cfg.opt_report_read_errors,"Report read errors" );

    /* If we are getting help or info scanners, make a fake scanner set with new output directory */
    if ( result.count( "help" ) || result.count( "info_scanners" )) {
        struct feature_recorder_set::flags_t f;
        scanner_set ss( sc, f, nullptr);     // make a scanner_set but with no XML writer. We will create it below
        ss.add_scanners( scanners_builtin);

        if ( result.count( "help" )) {     // -h
            cout << options.help() << std::endl;
            cout << "Global config options: " << std::endl << ss.sc.global_help_options << std::endl;
            ss.info_scanners( cout, false, true, 'e', 'x');
            return 1;
        } else {                        // -H
            ss.info_scanners( cout, true, true, 'e', 'x');
            return 2;
        }
    }

    try {
        sc.input_fname = result["image_name"].as<std::string>();
    } catch ( cxxopts::option_has_no_value_exception &e ) {
        if ( cfg.opt_recurse ) {
            cerr << "filedir not provided" << std::endl ;
        } else {
            cerr << "imagefile not provided" << std::endl ;
        }
        cout << options.help() << std::endl;
        return 3;
    }


    if ( result.count( "path" ) == 0 ){
        /* Code that runs if we are not using the path printer */

        try {
            sc.outdir                = result["outdir"].as<std::string>();
        } catch ( cxxopts::option_has_no_value_exception &e ) {
            cerr << "error: -o outdir must be specified" << std::endl ;
            cout << options.help() << std::endl;
            return 4;
        }

        /* The zap option wipes the contents of a directory, useful for debugging */
        if ( result.count( "zap" ) && std::filesystem::is_directory( sc.outdir )) {
            for ( const auto &entry : std::filesystem::recursive_directory_iterator( sc.outdir ) ) {
                if ( ! std::filesystem::is_directory( entry.path())){
                    cout << "erasing " << entry.path().string() << std::endl ;
                    std::filesystem::remove( entry );
                }
            }
        }
        cout << "mkdir " << sc.outdir << std::endl ;
        std::filesystem::create_directory( sc.outdir); // make sure directory exists
    }

    /* Load all the scanners and enable the ones we care about.  This
     * happens because:
     * - We need the scanners to generate the help message.
     * - We need the scanners for path printing.
     * - We need the scanners for scanning.
     */

    struct feature_recorder_set::flags_t f;
    scanner_set ss( sc, f, nullptr);     // make a scanner_set but with no XML writer. We will create it below
    ss.add_scanners( scanners_builtin);

    /* Applying the scanner commands will create the alert recorder. */
    try {
        ss.apply_scanner_commands();
    }
    catch ( const scanner_set::NoSuchScanner &e ) {
        cerr << "no such scanner: " << e.what() << std::endl ;
        return 5;
    }

    if ( result.count( "path" ) == 0 ){
        /* Give an error if a find list was specified
         * but no scanner that uses the find list is enabled.
         */

        if ( !FindOpts::get().empty()) {
            /* Look through the enabled scanners and make sure that
             * at least one of them is a FIND scanner
             */
            if ( !ss.is_find_scanner_enabled()){
                throw std::runtime_error( "find words are specified with -F but no find scanner is enabled.");
            }
        }

        if ( std::filesystem::exists( sc.outdir/"report.xml" )){
            /* Restarting */
            bulk_extractor_restarter r( sc,cfg);
            r.restart();                    // load the restart file and rename report.xml
        }
    }

    image_process *p = image_process::open( sc.input_fname, cfg.opt_recurse, cfg.opt_pagesize, cfg.opt_marginsize );

    /* are we supposed to run the path printer? */
    if ( result.count( "path" ) ) {
        std::string opt_path = result["path"].as<std::string>();
        path_printer pp( &ss, p, cout);
        if ( opt_path=="-http" || opt_path=="--http" ){
            pp.process_http( std::cin);
        } else if ( opt_path=="-i" || opt_path=="-" ){
            pp.process_interactive( std::cin);
        } else {
            pp.process_path( opt_path);
        }
	return 0;
    }

    /* Open the image file ( or the device) now.
     * We use *p because we don't know which subclass we will be getting.
     */

    dfxml_writer *xreport = new dfxml_writer( sc.outdir / Phase1::REPORT_FILENAME, false ); // do not make DTD
    ss.set_dfxml_writer( xreport );
    /* Start the clock */
    master_timer.start();

    Phase1 phase1( cfg, *p, ss);

    /* Validate the args */
    validate_path( sc.input_fname );

    /* Create the DFXML file in the report directory.
     * If we are restarting, the dfxml file was renamed.
     */

    /* Determine the feature files that will be used from the scanners that were enabled */
    auto feature_file_names = ss.feature_file_list();
#if 0
    uint32_t flags = 0;
    if ( stop_list.size()>0)        flags |= feature_recorder_set::CREATE_STOP_LIST_RECORDERS;
    if ( opt_write_sqlite3)         flags |= feature_recorder_set::ENABLE_SQLITE3_RECORDERS;
    if ( !opt_write_feature_files)  flags |= feature_recorder_set::DISABLE_FILE_RECORDERS;
#endif

    /* provide documentation to the user; the DFXML information comes from elsewhere */
    if ( !cfg.opt_quiet){
        cout << "bulk_extractor version: " << PACKAGE_VERSION << std::endl ;
        cout << "Input file: " << sc.input_fname << std::endl ;
        cout << "Output directory: " << sc.outdir << std::endl ;
        cout << "Disk Size: " << p->image_size() << std::endl ;
        cout << "Scanners: ";
        for ( auto const &it : ss.get_enabled_scanners()){
            cout << it << " ";
        }
        cout << std::endl ;

        if ( cfg.num_threads>0){
            cout << "Threads: " << cfg.num_threads << std::endl ;
        } else {
            cout << "Threading Disabled" << std::endl ;
        }
    }

    /*** PHASE 1 --- Run on the input image */
    struct notify_thread::notify_opts *o = new notify_thread::notify_opts(cfg);
    o->ssp = &ss;
    o->master_timer  = &master_timer;
    o->fraction_done = &fraction_done;
    o->phase = 1;

    if ( cfg.opt_notification) {
        notify_thread::launch_notify_thread( o);
    }
    ss.phase_scan();

#ifdef USE_SQLITE3
    if ( fs.flag_set( feature_recorder_set::ENABLE_SQLITE3_RECORDERS )) {
        fs.db_transaction_begin();
    }
#endif

    /* Go multi-threaded if requested */
    if ( cfg.num_threads > 0){
        cout << "going multi-threaded...( " << cfg.num_threads << " )" << std::endl ;
        ss.launch_workers( cfg.num_threads);
    } else {
        cout << "running single-threaded (DEBUG)..." << std::endl ;

    }

    phase1.dfxml_write_create( original_argc, original_argv);
    xreport->xmlout( "provided_filename", sc.input_fname ); // save this information
    xreport->add_timestamp( "phase1 start" );

    try {
        phase1.phase1_run();
        ss.join();                          // wait for threads to come together
    }
    catch ( const feature_recorder::DiskWriteError &e ) {
        cerr << "Disk write error during Phase 1 ( scanning). Disk is probably full." << std::endl
                  << "Remove extra files and restart bulk_extractor with the exact same command line to continue." << std::endl;
        return 6;
    }

#ifdef USE_SQLITE3
    if ( fs.flag_set( feature_recorder_set::ENABLE_SQLITE3_RECORDERS )) {
        fs.db_transaction_commit();
    }
#endif
    xreport->add_timestamp( "phase1 end" );
    if ( phase1.image_hash.size() > 0 ){
        cout << "Hash of Disk Image: " << phase1.image_hash << std::endl ;
    }

    /*** PHASE 2 --- Shutdown ***/
    {
        std::unique_lock<std::mutex> lock(o->Mphase);
        o->phase = 2;                        // will cause notify thread to shut down, and the notify thread will delete the object
    }
    if ( !cfg.opt_quiet) cout << "Phase 2. Shutting down scanners" << std::endl ;
    xreport->add_timestamp( "phase2 start" );
    try {
        std::cout << "Computing final histograms and shutting down..." << std::endl ;
        ss.shutdown();
    }
    catch ( const feature_recorder::DiskWriteError &e ) {
        cerr << "Disk write error during Phase 2 ( histogram making). Disk is probably full." << std::endl
                  << "Remove extra files and restart bulk_extractor with the exact same command line to continue." << std::endl;
        return 7;
    }

    xreport->add_timestamp( "phase2 end" );
    master_timer.stop();

    /*** PHASE 3 ---  report and then print final usage information ***/
    if ( !cfg.opt_quiet) cout << "Phase 3. Generating stats and printing final usage information" << std::endl;
    xreport->push( "report" );
    xreport->xmlout( "total_bytes",phase1.total_bytes);
    xreport->xmlout( "elapsed_seconds",master_timer.elapsed_seconds());
    xreport->xmlout( "max_depth_seen",ss.get_max_depth_seen());
    xreport->xmlout( "dup_bytes_encountered",ss.get_dup_bytes_encountered());
    ss.dump_scanner_stats();
    ss.dump_name_count_stats();
    xreport->pop( "report" );
    xreport->add_rusage();
    xreport->pop( "dfxml" );			// bulk_extractor
    xreport->close();

    if ( cfg.opt_quiet==0){
        float mb_per_sec = ( phase1.total_bytes / 1000000.0) / master_timer.elapsed_seconds();

        cout << "All Threads Finished!" << std::endl ;
        cout.precision( 4);
        cout << "Elapsed time: " << master_timer.elapsed_seconds() << " sec." << std::endl
                  << "Total MB processed: " << int( phase1.total_bytes / 1000000) << std::endl
                  << "Overall performance: " << mb_per_sec << " << MBytes/sec ";
        if ( cfg.num_threads>0){
            cout << mb_per_sec/cfg.num_threads << " ( MBytes/sec/thread)" << std::endl ;
        }
        cout << "sbufs created:   " << sbuf_t::sbuf_total << std::endl;
        cout << "sbufs unaccounted: " << sbuf_t::sbuf_count << " ( should be 0) " << std::endl;
    }

    try {
        feature_recorder &fr = ss.fs.named_feature_recorder( "email" );
        cout << "Total " << fr.name << " features found: " << fr.features_written << std::endl;
    }
    catch ( const feature_recorder_set::NoSuchFeatureRecorder &e ) {
        cout << "Did not scan for email addresses." << std::endl;
    }

    muntrace();
    /* TODO: We could wait for the notify thread to actually exit, but then we would need to address the sleep, and end up putting in a condition varialbe, and it would be a pain. */
    return( 0 );
}

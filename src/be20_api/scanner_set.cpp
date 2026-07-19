/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * common.cpp:
 * bulk_extractor backend stuff, used for both standalone executable and bulk_extractor.
 */

/* needed loading shared libraries and getting free memory*/
#include "config.h"

#include <sys/types.h>


#include <cstdio>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#ifdef HAVE_LINUX_SYSCTL_H
#include <linux/sysctl.h>
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "machine_stats.h"
#include "utils.h"
#include "formatter.h"

#include "threadpool.h"

#include "aftimer.h"
#include "dfxml_cpp/src/dfxml_writer.h"
#include "dfxml_cpp/src/hash_t.h"
#include "formatter.h"
#include "scanner_config.h"
#include "scanner_set.h"
#include "path_printer.h"

/****************************************************************
 *** SCANNER SET IMPLEMENTATION (previously the PLUG-IN SYSTEM)
 ****************************************************************/

/* BE2 revision:
 * In BE1, the active scanners were maintained by the plugin system's global state.
 * In BE2, there is no global state. Instead, scanners are grouped into scanner set, which
 *        in turn has a feature_recorder_set, which in turn has multiple feature recorders.
 *        The scanner set can then be asked to process a sbuf.
 *        All of the global variables go away.
 */

/* constructor and destructors */
scanner_set::scanner_set(scanner_config& sc_, const feature_recorder_set::flags_t& f, class dfxml_writer* writer_)
    : pool(*this), fs(f, sc_), sc(sc_), writer(writer_)
{
    debug_flags.debug_no_scanner_bypass    = getenv_debug("DEBUG_NO_SCANNER_BYPASS");
    debug_flags.debug_print_steps          = getenv_debug("DEBUG_PRINT_STEPS");
    debug_flags.debug_scanner              = getenv_debug("DEBUG_SCANNER");
    debug_flags.debug_dump_data            = getenv_debug("DEBUG_SCANNER_DUMP_DATA");
    debug_flags.debug_benchmark            = getenv_debug("DEBUG_BENCHMARK");
    debug_flags.debug_scanners_same_thread = getenv_debug("DEBUG_SCANNERS_SAME_THREAD");
    debug_flags.debug_sbuf_gc              = getenv_debug("DEBUG_SBUF_GC");
    debug_flags.debug_sbuf_gc0             = getenv_debug("DEBUG_SBUF_GC0");
    fs.flags.pedantic                      = getenv_debug("DEBUG_FS_PEDANTIC");
    pool.debug                             = getenv_debug("DEBUG_THREAD_POOL");

    const char *dsi = std::getenv("DEBUG_SCANNERS_IGNORE");
    if (dsi!=nullptr) debug_flags.debug_scanners_ignore=dsi;
}

scanner_set::~scanner_set()
{
    cleanup();
    if (threading) {
        join();                             // kill the threads if they are still running
        threading = false;
    }
    if (benchmark_cpu_thread) {
        benchmark_cpu_thread->join();
        delete benchmark_cpu_thread;
        benchmark_cpu_thread = nullptr;
    }
    /* Delete all of the scanner info blocks */

    const std::lock_guard<std::mutex> lock(Mscanner_info_db);
    for (auto &it : scanner_info_db){
        delete it.second;
        it.second = nullptr;
    }
    scanner_info_db.clear();
}

void scanner_set::set_dfxml_writer(class dfxml_writer *writer_)
{
    if (writer) {
        throw std::runtime_error("dfxml_writer already set");
    }
    writer = writer_;
}

class dfxml_writer *scanner_set::get_dfxml_writer() const
{
    return writer;
}


/****************************************************************
 *** scanner database
 ****************************************************************/

const std::string scanner_set::get_scanner_name(scanner_t scanner) const {
    const std::lock_guard<std::mutex> lock(Mscanner_info_db);
    auto it = scanner_info_db.find(scanner);
    if (it == scanner_info_db.end()) throw NoSuchScanner("get_scanner_name: scanner point not in sanner_info_db.");
    return it->second->name;
}

scanner_t* scanner_set::get_scanner_by_name(const std::string search_name) const {
    auto it = scanner_names.find(search_name);
    if (it == scanner_names.end()) throw NoSuchScanner(search_name);
    return it->second;
}

/****************************************************************
 *** thread interface
 ****************************************************************/

void scanner_set::launch_workers(int count)
{
    pool.launch_workers(count);
    threading = true;
}

void scanner_set::update_queue_stats(const sbuf_t *sbufp, int dir)
{
    assert (sbufp != nullptr );
    assert ( dir==1 || dir==-1);
    if (sbufp->depth()==0){
        if (dir == -1) {
            assert( depth0_sbufs_in_queue >= 0);
            assert( depth0_bytes_in_queue >= sbufp->bufsize);
        }
        depth0_sbufs_in_queue += dir;
        depth0_bytes_in_queue += sbufp->bufsize * dir;
    }
    if (dir == -1 ){
        assert( sbufs_in_queue >= 1 );
        assert( bytes_in_queue >= sbufp->bufsize);
    }
    sbufs_in_queue += dir;
    bytes_in_queue += sbufp->bufsize * dir;
}


void scanner_set::thread_set_status(const std::string &status)
{
    const std::lock_guard<std::mutex> lock(Mthread_status);
    thread_status[std::this_thread::get_id()] = status;
}

/****************************************************************
 ** System Info
 ****************************************************************/

/*
 * Print the status of each thread in the threadpool.
 */
std::map<std::string, std::string> scanner_set::get_realtime_stats() const
{
    std::map<std::string, std::string> ret;
    {
        if (threading) {
            ret[THREAD_COUNT_STR]        = std::to_string(pool.get_worker_count());
            ret[TASKS_QUEUED_STR]        = std::to_string(pool.get_tasks_queued());
            ret[DEPTH0_SBUFS_QUEUED_STR] = std::to_string(depth0_sbufs_in_queue);
            ret[DEPTH0_BYTES_QUEUED_STR] = std::to_string(depth0_bytes_in_queue);
            ret[SBUFS_QUEUED_STR]        = std::to_string(sbufs_in_queue);
            ret[BYTES_QUEUED_STR]        = std::to_string(bytes_in_queue);
        }
    }
    int counter = 0;
    {
        const std::lock_guard<std::mutex> lock(Mthread_status);
        for (const auto &it : thread_status) {
            counter++;
            std::string status = std::string(it.second);
            if (status.size() > 0 && isdigit(status[0])) {
                uint64_t status_offset = static_cast<uint64_t>(strtoll(status.c_str(), nullptr, 10));
                ret[ Formatter() << "thread-" << counter ] = status;
                if (status_offset > max_offset) {
                    max_offset = status_offset;
                }
            }
        }
    }
    ret[MAX_OFFSET] = std::to_string(max_offset);

    uint64_t available_memory = machine_stats::get_available_memory();
    if (available_memory!=0){
        ret[AVAILABLE_MEMORY_STR] = std::to_string(available_memory);
    }
    ret[SBUFS_CREATED_STR]   = std::to_string(sbuf_t::sbuf_total);
    ret[SBUFS_REMAINING_STR] = std::to_string(sbuf_t::sbuf_count);
    return ret;
}

/* Kill the threads and delete the threadpool */
void scanner_set::join()
{
    if (threading) {
        pool.join();
    }
}

/****************************************************************
 *** cpu benchmark thread
 ****************************************************************/


void scanner_set::launch_cpu_benchmark_thread(void *arg)
{
    scanner_set *ss = static_cast<scanner_set *>(arg);
    assert(ss->writer != nullptr);
    while(ss->current_phase == scanner_params::PHASE_SCAN && ss->get_worker_count() > 0 ) {
        uint64_t virtual_size = 0;
        uint64_t resident_size = 0;
        machine_stats::get_memory(&virtual_size, &resident_size);
        ss->writer->xmlout("debug:cpu_benchmark","",
                           Formatter()
                           << "worker_count='" << ss->get_worker_count() << "' "
                           << "tasks_queued='" << ss->get_tasks_queued() << "' "
                           << "depth0_sbufs_in_queue='" << ss->depth0_sbufs_in_queue << "' "
                           << "cpu_percent='" << machine_stats::get_cpu_percentage() << "' "
                           << "vss='" << virtual_size << "' "
                           << "rss='" << resident_size << "' "
                           << aftimer::now_str(" t='","'"), true);
        std::this_thread::sleep_for( std::chrono::seconds( 1 ));
    }
}



/****************************************************************
 *** scanner stats
 ****************************************************************/

void scanner_set::add_scanner_stat(scanner_t *scanner, const struct scanner_set::stats &n)
{
    const std::lock_guard<std::mutex> lock(Mscanner_stats);
    scanner_stats[scanner] += n;
}


#if 0
/****************************************************************
 *** per-path stats
 ****************************************************************/

void scanner_set::add_path_stat(std::string path, const struct scanner_set::stats &n)
{
    path_stats[path] += n;
}
#endif


/****************************************************************
 *** Feature recorders
 ****************************************************************/

feature_recorder& scanner_set::named_feature_recorder(const std::string name) const
{
    return fs.named_feature_recorder(name);
}

std::vector<std::string> scanner_set::feature_file_list() const
{
    return fs.feature_file_list();
}

/****************************************************************
 *** Scanning
 ****************************************************************/


void scanner_set::add_scanner(scanner_t scanner) {
    /* If scanner is already loaded, that's an error */
    {
        const std::lock_guard<std::mutex> lock(Mscanner_info_db);
        if (scanner_info_db.find(scanner) != scanner_info_db.end()) { throw std::runtime_error("scanner already added"); }
    }

    /* Initialize the scanner.
     * Use an empty sbuf and an empty feature recorder to create an empty scanner params that is in PHASE_STARTUP.
     * We then ask the scanner to initialize.
     */
    //PrintOptions po;
    scanner_params sp(sc, this, nullptr, scanner_params::PHASE_INIT, nullptr);

    // Send the scanner the PHASE_INIT message, which will cause it to fill in the sp.info field.
    sp.info = new scanner_params::scanner_info(scanner);
    auto old_info = sp.info;
    (*scanner)(sp);
    assert(sp.info == old_info);        // make sure that info wasn't changed on us

    // The scanner should have set the info field.
    if (sp.info->name == "") {
        throw std::runtime_error("scanner_set::add_scanner: a scanner did not set its name.  "
                                 "Re-run with DEBUG_SCANNER_SET_REGISTER=1 to find those that did.");
    }
    if (debug_flags.debug_scanners_ignore.find(sp.info->name) != std::string::npos){
        std::cerr << "DEBUG: ignore add_scanner " << sp.info->name << "\n";
        return;
    }
    {
        const std::lock_guard<std::mutex> lock(Mscanner_info_db);
        scanner_info_db[scanner]     = sp.info; // was std::move(); not needed anymore
        scanner_names[sp.info->name] = scanner;

        // Enable the scanner if it is not disabled by default.
        if (scanner_info_db[scanner]->scanner_flags.default_enabled) {
            enabled_scanners.insert(scanner);
        }
    }
}

/* add_scanners allows a calling program to add a null-terminated array of scanners. */
void scanner_set::add_scanners(scanner_t* const* scanners)
{
    for (int i = 0; scanners[i]; i++) { add_scanner(scanners[i]); }
}

/* Add a scanner from a file (it's in the file as a shared library) */
void scanner_set::add_scanner_file(std::string fn)
{
    if(time(0)>10) {                    // prevents compiler warning
        throw std::runtime_error("scanner_set::add_scanner_file: not implemented yet.");
    }
#if 0
    /* Figure out the function name */
    size_t extloc = fn.rfind('.');
    if(extloc==std::string::npos){
        fprintf(stderr,"Cannot find '.' in %s",fn.c_str());
        exit(1);
    }
    std::string func_name = fn.substr(0,extloc);
    size_t slashloc = func_name.rfind('/');
    if(slashloc!=std::string::npos) func_name = func_name.substr(slashloc+1);
    slashloc = func_name.rfind('\\');
    if(slashloc!=std::string::npos) func_name = func_name.substr(slashloc+1);

    if(debug) std::cout << "Loading: " << fn << " (" << func_name << ")\n";
    scanner_t *scanner = 0;
#if defined(HAVE_DLOPEN)
    void *lib=dlopen(fn.c_str(), RTLD_LAZY);

    if(lib==0){
        fprintf(stderr,"dlopen: %s\n",dlerror());
        exit(1);
    }

    /* Resolve the symbol */
    scanner = (scanner_t *)dlsym(lib, func_name.c_str());

    if(scanner==0){
        fprintf(stderr,"dlsym: %s\n",dlerror());
        exit(1);
    }
#elif defined(HAVE_LOADLIBRARY)
    /* Use Win32 LoadLibrary function */
    /* See http://msdn.microsoft.com/en-us/library/ms686944(v=vs.85).aspx */
    HINSTANCE hinstLib = LoadLibrary(TEXT(fn.c_str()));
    if(hinstLib==0){
        fprintf(stderr,"LoadLibrary(%s) failed",fn.c_str());
        exit(1);
    }
    scanner = (scanner_t *)GetProcAddress(hinstLib,func_name.c_str());
    if(scanner==0){
        fprintf(stderr,"GetProcAddress(%s) failed",func_name.c_str());
        exit(1);
    }
#else
    std::cout << "  ERROR: Support for loadable libraries not enabled\n";
    return;
#endif
    load_scanner(*scanner,sc);
#endif
}

/* Add all of the scanners in a directory */
void scanner_set::add_scanner_directory(const std::string &dirname)
{
    if(time(0)>10) {                    // prevents compiler warning
        throw std::runtime_error("scanner_set::add_scanner_directory: not implemented yet.");
    }
#if 0
TODO: Re-implement using C++17 directory reading.


        if(fname.substr(0,5)=="scan_" || fname.substr(0,5)=="SCAN_"){
            size_t extloc = fname.rfind('.');
            if(extloc==std::string::npos) continue; // no '.'
            std::string ext = fname.substr(extloc+1);
#ifdef _WIN32
            if(ext!="DLL") continue;    // not a DLL
#else
            if(ext!="so") continue;     // not a shared library
#endif
            load_scanner_file(dirname+"/"+fname,sc );
        }
    }
#endif
}

/* This interface creates if we are in init phase, doesn't if we are in scan phase */
/**
 * Print a list of scanners.
 * We need to load them to do this, so they are loaded with empty config
 * Note that scanners can only be loaded once, so this exits.
 * Used for the help system
 */
void scanner_set::info_scanners(std::ostream &out, bool detailed_info, bool detailed_settings,
                                const char enable_opt,
                                const char disable_opt) {
    /* Get a list of scanner names */
    const std::lock_guard<std::mutex> lock(Mscanner_info_db);
    std::vector<std::string> all_scanner_names,enabled_scanner_names, disabled_scanner_names;

    for (auto &it : scanner_info_db) {
        all_scanner_names.push_back( it.second->name );
    }

    /* Sort them */
    sort (all_scanner_names.begin(), all_scanner_names.end());

    /* Now print info on each scanner in alphabetical order */
    for (const auto &name : all_scanner_names) {
        scanner_t *scanner = get_scanner_by_name(name);
        const struct scanner_params::scanner_info &scanner_info  = *(scanner_info_db[scanner]);
        if (detailed_info) {
            if (scanner_info.name.size()) out << "Scanner Name: " << scanner_info.name;
            if (is_scanner_enabled(scanner_info.name)) { out << " (ENABLED) "; }
            out << "\n";
            out << "flags:  " << scanner_info.scanner_flags.asString() << "\n";
            if (scanner_info.author.size()) out << "Author: " << scanner_info.author << "\n";
            if (scanner_info.description.size()) out << "Description: " << scanner_info.description << "\n";
            if (scanner_info.url.size()) out << "URL: " << scanner_info.url << "\n";
            if (scanner_info.scanner_version.size()) out << "Scanner Version: " << scanner_info.scanner_version << "\n";
            out << "Min sbuf size: " << scanner_info.min_sbuf_size << "\n";
            out << "Feature Names: ";
            int count = 0;
            for (auto i2 : scanner_info.feature_defs) {
                if (count++ > 0) out << ", ";
                out << i2.name;
            }
            if (count == 0) { out << "(none)"; }
            out << "\n";
            if (detailed_settings) {
                out << "Settable Options (and their defaults): \n";
                out << scanner_info.help_options;
            }
            out << "------------------------------------------------\n\n";
        }
        if (scanner_info.scanner_flags.no_usage) continue;
        if (is_scanner_enabled(scanner_info.name)) {
            enabled_scanner_names.push_back(scanner_info.name);
        } else {
            disabled_scanner_names.push_back(scanner_info.name);
        }
    }
    if (enabled_scanner_names.size()) {
        out << "These scanners enabled; disable with -" << disable_opt << ":\n";
        for (const auto &it : enabled_scanner_names) {
            out << "   -" << disable_opt << " " << it << " - disable scanner " << it << "\n";
            /* Print its options if it has any */
            out << scanner_info_db[get_scanner_by_name(it)]->help_options;
        }
    }
    if (disabled_scanner_names.size()) {
        out << "These scanners disabled; enable with -" << enable_opt << ":\n";
        sort(disabled_scanner_names.begin(), disabled_scanner_names.end());
        for (const auto &it : disabled_scanner_names) {
            out << "   -" << enable_opt << " " << it << " - enable scanner " << it << "\n";
            /* Print its options if it has any */
            out << scanner_info_db[get_scanner_by_name(it)]->help_options;
        }
    }

    /* And print information about the feature recorders that have settable options */
    fs.info_feature_recorders( out );
}


/**
 * apply_sacanner_commands:
 * applies all of the enable/disable commands and create the feature recorders
 */

void scanner_set::apply_scanner_commands() {
    const std::lock_guard<std::mutex> lock(Mscanner_info_db);
    if (current_phase != scanner_params::PHASE_INIT) {
        throw std::runtime_error(
                                 Formatter()
                                 << "apply_scanner_commands can only be run in scanner_params::PHASE_INIT."
                                 << " current phase="
                                 << (unsigned int)(current_phase));
    }
    for (const auto &cmd : sc.get_scanner_commands()) {
        if (cmd.scannerName == scanner_config::scanner_command::ALL_SCANNERS) {
            /* If name is 'all' and the NO_ALL flag is not set for that scanner,
             * then either enable it or disable it as appropriate
             */
            for (const auto &it : scanner_info_db) {
                if (it.second->scanner_flags.no_all) {
                    continue;
                }
                if (cmd.command == scanner_config::scanner_command::ENABLE) {
                    enabled_scanners.insert(it.first);
                }
                if (cmd.command == scanner_config::scanner_command::DISABLE) {
                    enabled_scanners.erase(it.first);
                }
            }
        } else {
            /* enable or disable this scanner */
            scanner_t* scanner = get_scanner_by_name(cmd.scannerName);
            if (cmd.command == scanner_config::scanner_command::ENABLE) {
                enabled_scanners.insert(scanner);
            } else {
                enabled_scanners.erase(scanner);
            }
        }
    }

    /* Create feature recorders for each enabled scanner.
     * Multiple scanners may request the same feature recorder without generating an error.
     */
    fs.create_alert_recorder();
    for (const auto &sit : scanner_info_db) {
        if (is_scanner_enabled(sit.second->name )){
            for (const auto &it : sit.second->feature_defs) {
                fs.create_feature_recorder(it);
            }

            /* Create all of the requested histograms
             * Multiple scanners may request the same histograms without generating an error.
             */
            for (const auto &it : sit.second->histogram_defs) {
                // Make sure that a feature recorder exists for the feature specified in the histogram
                fs.named_feature_recorder(it.feature).histogram_add(it);
            }
        }
    }

    /* Tell each of the enabled scanners to init */
    current_phase = scanner_params::PHASE_INIT2;
    scanner_params sp(sc, this, nullptr, scanner_params::PHASE_INIT2, nullptr);
    for (const auto &it : enabled_scanners) { (*it)(sp); }

    /* set the carve defaults from the command line (-S jpeg_carve_mode=...) or the scanner definitions if one is not provided*/
    fs.set_carve_defaults();

    /* Read the command line arguments for carve_modes */
    current_phase = scanner_params::PHASE_ENABLED;
}

bool scanner_set::is_scanner_enabled(const std::string& name)
{
    scanner_t* scanner = get_scanner_by_name(name);
    {
        return enabled_scanners.find(scanner) != enabled_scanners.end();
    }
}

// put the enabled scanners into the vector
std::vector<std::string> scanner_set::get_enabled_scanners() const
{
    std::vector<std::string> ret;
    for (const auto &it : enabled_scanners) {
        const std::lock_guard<std::mutex> lock(Mscanner_info_db);
        auto f = scanner_info_db.find(it);
        ret.push_back(f->second->name);
    }
    return ret;
}

// Return true if any of the enabled scanners are a FIND scanner
bool scanner_set::is_find_scanner_enabled()
{
    for (const auto &it : enabled_scanners) {
        const std::lock_guard<std::mutex> lock(Mscanner_info_db);
        if (scanner_info_db[it]->scanner_flags.find_scanner) { return true; }
    }
    return false;
}


/* written as part of <configuration> */
void scanner_set::dump_enabled_scanner_config() const
{
    if (writer!=nullptr) {
        writer->push("scanners");
        /* Generate a list of the scanners in use */
        for (const auto &it : get_enabled_scanners()) {
            writer->xmlout("scanner",it);
        }
        writer->pop("scanners");		// scanners
    }
}

/* Typically written during shutdown */
void scanner_set::dump_scanner_stats() const
{
    if (writer!=nullptr) {
        writer->push("scanner_stats");
        const std::lock_guard<std::mutex> lock(Mscanner_stats);
        for (const auto &it: scanner_stats) {
            writer->set_oneline(true);
            writer->push("scanner");
            writer->xmlout("name", get_scanner_name( it.first ));
            writer->xmlout("seconds", static_cast<double>(it.second.ns) / 1E9);
            writer->xmlout("calls", it.second.calls);
            writer->pop();
            writer->set_oneline(false);
        }
        writer->pop();
    }
}

/****************************************************************
 *** PHASE_ENABLE methods.
 ****************************************************************/

const std::filesystem::path scanner_set::get_input_fname() const
{
    return sc.input_fname;
}

/****************************************************************
 *** PHASE_SCAN methods.
 ****************************************************************/

/* Transition to the scanning phase */
void scanner_set::phase_scan() {
    if (current_phase != scanner_params::PHASE_ENABLED) {
        throw std::runtime_error("start_scan can only be run in scanner_params::PHASE_ENABLED");
    }
    fs.frm_freeze();
    current_phase = scanner_params::PHASE_SCAN;
    if (debug_flags.debug_benchmark && writer!=nullptr) {
        void *arg = static_cast<void *>(this);
        benchmark_cpu_thread = new std::thread( scanner_set::launch_cpu_benchmark_thread, arg);
    }
}

// https://stackoverflow.com/questions/16190078/how-to-atomically-update-a-maximum-value
template <typename T> void update_maximum(std::atomic<T>& maximum_value, T const& value) noexcept {
    T prev_value = maximum_value;
    while (prev_value < value && !maximum_value.compare_exchange_weak(prev_value, value)) {}
}

/****************************************************************
 ** sbuf management
 *
 * Schedule an sbuf to be processed.
 * If there is no process pool or if it the sbuf is too small, process it in the current thread.
 * Othewise put it on the work queue.
 */
void scanner_set::schedule_sbuf(const sbuf_t *sbufp)
{
    assert (sbufp != nullptr );

    if (debug_flags.debug_print_steps) {
        std::cerr << "schedule_sbuf " << *sbufp << std::endl;
    }

    /* Run in same thread:
     - If there is no pool
     - If the sbuf is too small
     - If the sbuf has a parent (because if it does, the parent might get cleared while this sbuf is pending.)
     - */
    retain_sbuf(sbufp);
    if ( threading==false
         || debug_flags.debug_scanners_same_thread==true
         || (sbufp->depth() > 0 && sbufp->bufsize < SAME_THREAD_SBUF_SIZE)
         || (sbufp->has_parent())) {
        process_sbuf(sbufp);
        release_sbuf(sbufp);            // will delete if no one else has a copy
        return;
    }

    if (debug_flags.debug_print_steps) {
        std::cerr << "schedule_sbuf - push_task " << *sbufp << std::endl;
    }

    pool.push_task(sbufp);              // will get released in threadpool
}


std::mutex cerrM;
void scanner_set::retain_sbuf(const sbuf_t *sbufp)
{
    if (debug_flags.debug_sbuf_gc ||
        ( debug_flags.debug_sbuf_gc0 && sbufp->depth()==0)){
        const std::lock_guard<std::mutex> lock(cerrM);
        std::cerr << "retain_sbuf " << sbufp->pos0 << std::endl;
    }
    sbufp->reference_count += 1;
    update_queue_stats( sbufp, +1 );
}

void scanner_set::release_sbuf(const sbuf_t *sbufp)
{
    if (debug_flags.debug_sbuf_gc ||
        ( debug_flags.debug_sbuf_gc0 && sbufp->depth()==0)){
        const std::lock_guard<std::mutex> lock(cerrM);
        std::cerr << "release_sbuf " << sbufp->pos0 << std::endl;
    }
    update_queue_stats( sbufp, -1 );
    thread_set_status(sbufp->pos0.str() + " release_sbuf");
    record_work_end( sbufp );
    if (--sbufp->reference_count == 0 ){
        delete sbufp;
    }
}



/**
 * Records when each sbuf starts. Used for restarting and graphing CPU utilization during run.
 */
void scanner_set::record_work_start(const sbuf_t *sbufp)
{
    if (sbufp->depth()==0 && writer) {
        writer->xmlout("debug:work_start","",
                       Formatter()
                       << "threadid='"  << std::this_thread::get_id() << "'"
                       << " pos0='"     << dfxml_writer::xmlescape(sbufp->pos0.str()) << "'"
                       << " pagesize='" << sbufp->pagesize << "'"
                       << " bufsize='"  << sbufp->bufsize << "'"
                       << aftimer::now_str(" t='","'"), true);
    }
}

void scanner_set::record_work_start_stop_pos0str(const std::string pos0str)
{
    if (writer) {
        writer->xmlout("debug:work_start","", Formatter() << "pos0='" << dfxml_writer::xmlescape(pos0str) << "' restarted='1' " , true);
        writer->xmlout("debug:work_stop","",  Formatter() << "pos0='" << dfxml_writer::xmlescape(pos0str) << "' restarted='1' ", true);
    }
}


void scanner_set::record_work_end(const sbuf_t *sbufp)
{
    if (sbufp->depth()==0 && writer) {

        Formatter fmt;
        fmt << "threadid='" << std::this_thread::get_id() << "' "
            << "pos0='" << dfxml_writer::xmlescape(sbufp->pos0.str()) << "' "
            << "rc='" << sbufp->reference_count << "'";
        // add the time if this is a debug
        if (debug_flags.debug_benchmark) {
            fmt << aftimer::now_str(" t='","'");
        }
        writer->xmlout("debug:work_stop", "",fmt, true);
    }
}


/****************************************************************
 ** sbuf processing
 ****************************************************************/

/* Process an sbuf with a particular scanner.
 * sbuf must be retained prior to calling this.
 *
 * TODO: Move the fast processing of scan/do not scan into the main thread.
 */
void scanner_set::process_sbuf(const sbuf_t* sbufp, scanner_t *scanner)
{
    scanner_params sp(sc, this, nullptr, scanner_params::PHASE_SCAN, sbufp);
    const struct scanner_params::scanner_info *info = nullptr;
    {
        const std::lock_guard<std::mutex> lock(Mscanner_info_db);
        info = scanner_info_db[scanner];
    }

    // Look for reasons not to run a scanner
    // this is a lot of find operations - could we make a vector of the enabled scanner_info_dbs?
    const class sbuf_t& sbuf = *sbufp;       // read-only reference
    const auto &name  = info->name;           // scanner name
    const auto &flags = info->scanner_flags; // scanner flags

    if (enabled_scanners.find(scanner) == enabled_scanners.end()) {
        return;
    }

    if (sbuf.depth() > 0 && flags.depth0_only) {
        // depth >0 and this scanner only run at depth 0
        return;
    }

    // is sbuf large enough?
    if (sbuf.bufsize < info->min_sbuf_size) {
        return;
    }

    // Don't rescan data that has been seen twice --- and if scanner doesn't doesn't want dups.
    if (sbuf.seen_before && flags.scan_seen_before == false) {
        if (debug_flags.debug_benchmark && writer) {
            writer->xmlout("debug:bypass", "",
                           Formatter()
                           << "sbuf='" << sbuf.pos0.str() << "' "
                           << "bufsize='" << sbuf.bufsize << "' "
                           << "scanner='" << get_scanner_name(scanner) << "' "
                           << "reason='seen_before'", true);
        }
        return;
    }

    size_t ngram_size = sbuf.find_ngram_size(sc.max_ngram);
    if (ngram_size > 0 && flags.scan_ngram_buffer == false) {
        if (debug_flags.debug_benchmark && writer) {
            writer->xmlout("debug:bypass", "",
                           Formatter() << "sbuf='" << sbuf.pos0.str() << "' ngram_size='" << ngram_size << "'", true);
        }
        return;
    }

    size_t distinct_chars = sbuf.get_distinct_character_count();
    if (info->min_distinct_chars > distinct_chars) {
        if (debug_flags.debug_benchmark && writer) {
            writer->xmlout("debug:bypass", "",
                           Formatter() << "sbuf='" << sbuf.pos0.str() << "' min_distinct_chars='" << distinct_chars << "'", true);
        }
        return;
    }

    // If the scanner is a recurse_all, it always calls recurse. We can't it twice in the stack, or else
    // we get infinite regression.
    if (flags.recurse_always && sbuf.pos0.contains( info->pathPrefix)) {
        return;
    }

    // Check to see if scanner wants memory or filesystems and if we possibly have them
    if (info->scanner_flags.scanner_wants_memory && sbuf.possibly_has_memory==false){
        return;
    }

    if (info->scanner_flags.scanner_wants_filesystems && sbuf.possibly_has_filesystem==false){
        return;
    }

    try {
        /* Compute the effective path for stats */
        std::string epath;
        if (record_call_stats) {
            bool inname = false;
            for (auto cc : sbuf.pos0.path) {
                if (isupper(cc)) inname = true;
                if (inname) epath.push_back(toupper(cc));
                if (cc == '-') inname = false;
            }
            if (epath.size() > 0) epath.push_back('-');
            for (auto cc : name) { epath.push_back(toupper(cc)); }
        }

        /* Call the scanner.*/
        aftimer t;
        thread_set_status( sbuf.pos0.str() + ": " + name + " (" + std::to_string(sbuf.bufsize) + " bytes)" );

        if (debug_flags.debug_print_steps) {
            std::cerr << "sbuf.pos0=" << sbuf.pos0
                      << " calling scanner " << name << " threadid=" << std::this_thread::get_id() << std::endl;
        }
        if (record_call_stats || debug_flags.debug_print_steps) {
            t.start();
        }
        (*scanner)(sp);

        if (record_call_stats || debug_flags.debug_print_steps) {
            t.stop();
            struct stats st(t.elapsed_nanoseconds(), 1);
            add_scanner_stat(scanner, st);
            if (debug_flags.debug_print_steps) {
                std::cerr << "sbuf.pos0=" << sbuf.pos0 << "   return scanner " << name
                          << " t=" << t.elapsed_seconds() << " threadid=" << std::this_thread::get_id() << std::endl;
            }
        }
    }
    catch (const feature_recorder::DiskWriteError &e) {
        sp.ss->disk_write_errors ++;
        try {
            feature_recorder &ar = fs.get_alert_recorder();
            ar.write(sbuf.pos0, "scanner=" + name, Formatter() << "<exception>" << e.what() << "</exception>");
            ar.flush();
        }
        catch (feature_recorder_set::NoSuchFeatureRecorder& e2) {
        }
    }

    catch (const std::exception& e) {
        try {
            feature_recorder &ar = fs.get_alert_recorder();
            ar.write(sbuf.pos0, "scanner=" + name, Formatter() << "<exception>" << e.what() << "</exception>");
            ar.flush();
        }
        catch (feature_recorder_set::NoSuchFeatureRecorder& e2) {
        }
    }
    catch (...) {
        try {
            feature_recorder &ar = fs.get_alert_recorder();
            ar.write(sbuf.pos0, "scanner=" + name, "<unknown_exception></unknown_exception>");
            ar.flush();
        } catch (feature_recorder_set::NoSuchFeatureRecorder& e) {}
    }
}

/* Process an sbuf (typically in its own thread).
 * sbuf must be retained prior to calling this.
 */
void scanner_set::process_sbuf(const sbuf_t* sbufp)
{
    /****************************************************************
     ** validate runtime enviornment.
     **/

    if (debug_flags.debug_print_steps) {
        std::cerr << "process_sbuf " << *sbufp << std::endl;
    }

    if (current_phase != scanner_params::PHASE_SCAN) {
        throw std::runtime_error("process_sbuf can only be run in scanner_params::PHASE_SCAN");
    }

    /****************************************************************
     ** validate sbuf and then record that we are processing it.
     */
    assert(sbufp != nullptr);
    assert(sbufp->children == 0);       // we are going to free it, so it better not have any children.
    assert(sbufp->reference_count > 0); // it better have been retained.

    if (sbufp->bufsize==0){
        thread_set_status("IDLE");
        return;
    }

    /****************************************************************
     ** Record that we are starting to work on the sbuf for statistics purposes.
     **/
    record_work_start( sbufp );
    thread_set_status( sbufp->pos0.str() + " process_sbuf (" + std::to_string(sbufp->bufsize) + ")" );

    const class sbuf_t& sbuf = *sbufp;  // read-only reference

    const pos0_t& pos0 = sbuf.pos0;
    /* If we are too deep, error out */
    if (sbuf.depth() >= max_depth) {
        fs.get_alert_recorder().write(pos0,
                                      feature_recorder::MAX_DEPTH_REACHED_ERROR_FEATURE,
                                      feature_recorder::MAX_DEPTH_REACHED_ERROR_CONTEXT);
        thread_set_status("IDLE");
        return;
    }

    update_maximum<unsigned int>(max_depth_seen, sbuf.depth());

    /* Determine if we have seen this buffer before.
     * Some scanners are okay with duplicate processing, but the default is that they are not.
     * Nevertheless, we will add dupliate bytes to the hash of the disk image.
     */
    sbufp->seen_before = previously_processed_count(sbuf) > 0; // abstraction violation
    if (sbufp->seen_before) {
        dup_bytes_encountered += sbuf.bufsize;
    }

    /*
     * abstraction violations:
     * store information about the sbuf in the sbuf itself.
     * Ugly, but efficient.
     */

    /* Determine if this scanner is likely to have memory or a file system.
     */
    sbufp->possibly_has_memory     = (sbuf.pos0.depth() == 0);
    sbufp->possibly_has_filesystem = (sbuf.pos0.depth() == 0);
    std::string lastAddedPart = sbuf.pos0.lastAddedPart();
    if (lastAddedPart.size()>0) {
        for (size_t i=0;i<lastAddedPart.size();i++){
            lastAddedPart[i] = tolower(lastAddedPart[i]);
        }
        /* Get the scanner and find it it makes those things. If we
         * can't find it, the last added scanner is probably a
         * filename or something.  Another way to handle this would be
         * for pos0_t to actually have a vector that tracks all of the
         * stacked scanners (as scanner_t *), but that would result in
         * a *lot* of overhead that would be rarely used.
         */
        try {
            scanner_t *parent_scanner      = get_scanner_by_name(lastAddedPart);
            {
                const std::lock_guard<std::mutex> lock(Mscanner_info_db);
                sbufp->possibly_has_memory     = scanner_info_db[parent_scanner]->scanner_flags.scanner_produces_memory;
                sbufp->possibly_has_filesystem = scanner_info_db[parent_scanner]->scanner_flags.scanner_produces_filesystems;
            }
        } catch (scanner_set::NoSuchScanner &e) {
            // ignore the exception
        }
    }

    thread_set_status( sbuf.pos0.str() + " finding ngram size (" + std::to_string(sbuf.bufsize) + ")" );

    /****************************************************************
     *** CALL EACH OF THE SCANNERS ON THE SBUF
     ****************************************************************/

    if (debug_flags.debug_dump_data) {
        sbuf.hex_dump(std::cerr);
    }

    /* Make the scanner params once, rather than every time through */
    // loop for each scanner.
    for (const auto &it : enabled_scanners) {
        // Process if not threading or if we are supposed to process all in the same thread
        retain_sbuf(sbufp);
        if (!threading || debug_flags.debug_scanners_same_thread) {
            process_sbuf(sbufp, it);
            release_sbuf(sbufp);
        } else {
            pool.push_task(sbufp, it);
        }
    }
    thread_set_status("IDLE");
    return;
}

/****************************************************************
 *** PHASE_SHUTDOWN methods.
 ****************************************************************/


void scanner_set::log(const std::string message)
{
    if (writer){
        writer->xmlout("log", message, aftimer::now_str("t='","'"), false);
        writer->flush();
    }
    else {
        std::cerr  << "log: " << message << "\n";
    }
}

void scanner_set::log(const sbuf_t &sbuf, const std::string message) // writes sbuf if not too deep.
{
    if (sbuf.depth() <= log_depth) {
        log( Formatter()
             << "pos0=" << sbuf.pos0
             << " buflen=" << sbuf.bufsize
             << (sbuf.has_hash() ? sbuf.hash() : "")
             << ": " << message );
    }
}




/**
 * Shutdown:
 * 1 - change phase from PHASE_SCAN to PHASE_SHUTDOWN.
 * 2 - Shut down all of the scanners.
 * 3 - write out the in-memory histograms.
 * 4 - terminate the XML file.
 */
void scanner_set::shutdown()
{
    if (debug_flags.debug_print_steps) {
        std::cerr << "scanner_set::shutdown" << std::endl;
    }

    if (current_phase != scanner_params::PHASE_SCAN) {
        throw std::runtime_error("shutdown can only be called in scanner_params::PHASE_SCAN");
    }
    current_phase = scanner_params::PHASE_SHUTDOWN;

    /* Tell the scanners we are shutting down */
    scanner_params sp(sc, this, nullptr, scanner_params::PHASE_SHUTDOWN, nullptr);
    for (const auto &it : enabled_scanners) { (*it)(sp); }

    fs.feature_recorders_shutdown();

    /* Tell every feature recorder to flush all of its histograms if they haven't been generated.
     */
    fs.histograms_generate();
    cleanup();
}

/* Tells scanners to free their memory */
void scanner_set::cleanup()
{
    if (debug_flags.debug_print_steps) {
        std::cerr << "scanner_set::cleanup" << std::endl;
    }

    if (current_phase != scanner_params::PHASE_CLEANED) {
        current_phase = scanner_params::PHASE_CLEANUP;
        /* Tell the scanners we are cleaning up down */
        scanner_params sp(sc, this, nullptr, scanner_params::PHASE_CLEANUP, nullptr);
        const std::lock_guard<std::mutex> lock(Mscanner_info_db);
        for (const auto &it : scanner_info_db) { (*it.first)(sp); }
        current_phase = scanner_params::PHASE_CLEANED;
    }
}

/*
 * uses hash to determine if a block was prevously seen.
 * Hopefully sbuf.buf() is zero-copy.
 */
std::string scanner_set::hash(const sbuf_t& sbuf) const
{
    return sbuf.hash(fs.hasher.func);
}

uint64_t scanner_set::previously_processed_count(const sbuf_t& sbuf) {
    std::string hash = sbuf.hash();
    return previously_processed_counter[ hash ]++;
}


/****************************************************************
 *** packet handling
 ****************************************************************/


/**
 * Process a pcap packet.
 * Designed to be very efficient because we have so many packets.
 */
#if 0
void scanner_set::process_packet(const be20::packet_info &pi)
{
    for (packet_plugin_info_vector_t::iterator it = packet_handlers.begin(); it != packet_handlers.end(); it++){
        (*(*it).callback)((*it).user,pi);
    }
}
#endif


#if 0
/* Vector of callbacks */
typedef std::vector<packet_plugin_info> packet_plugin_info_vector_t;
//packet_plugin_info_vector_t  packet_handlers;   // pcap callback handlers
/* object for keeping track of packet callbacks */
class packet_plugin_info {
public:
    packet_plugin_info(void *user_,be20::packet_callback_t *callback_):user(user_),callback(callback_){};
    void *user;
    be20::packet_callback_t *callback;
};
void scanner_set::load_scanner_packet_handlers()
{
    for (const auto &it: enabled_scanners){
        const scanner_def *sd = (*it);
            if(sd->info.packet_cb){
                packet_handlers.push_back(packet_plugin_info(sd->info.packet_user,sd->info.packet_cb));
            }
        }
    }
}
#endif

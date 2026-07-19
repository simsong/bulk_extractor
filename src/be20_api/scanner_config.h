/*
 * scanner_config.h:
 *
 * class to hold the full configuration of the scanner_set and the feature recorders.
 *
 * Includes a set of name=value pairs from the command line and the list of all scanners that
 * are enabled or disabled.
 *
 * This class is also used to build the help string.
 *
 * All of the scanners get the same config, so the names that the scanners want need to be unique.
 * We could have adopted a system where each scanner had its own configuraiton space, but we didn't.
 * Scanner histograms are added to 'histograms' by machinery.
 */

#ifndef _SCANNER_CONFIG_H_
#define _SCANNER_CONFIG_H_

#include <cinttypes>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "utils.h"

/* There is only one scanner-config object. It is called for all of the scanners
 */
struct scanner_config {
    /**
     * Commands whether to enable or disable a scanner.
     * Typically created from parsing command-line arguments
     */
    struct scanner_command {
        static inline const std::string ALL_SCANNERS = "all";
        enum command_t { DISABLE, ENABLE };
        scanner_command(const scanner_command& sc) : scannerName(sc.scannerName), command(sc.command){};
        scanner_command(const std::string& scannerName_, scanner_command::command_t c)
            : scannerName(scannerName_), command(c){};
        std::string scannerName{};
        command_t command{};
        /* default copy construction and assignment */
        scanner_command& operator=(const scanner_command& a) {
            this->scannerName = a.scannerName;
            this->command = a.command;
            return *this;
        }
    };

private:
    /* The global configuration */
    typedef std::map<std::string, std::string> config_t; // configuration for scanner passed in
    config_t namevals{};                                 //  (input) name=val map
    std::string global_help_options {""};
    // The commands for those scanners (enable, disable, options, etc.
    typedef std::vector<struct scanner_config::scanner_command> scanner_commands_t;
    scanner_commands_t scanner_commands {};

public:
    const scanner_commands_t get_scanner_commands() {
        return static_cast<const scanner_commands_t>(scanner_commands);
    }
    void set_config(std::string name, std::string val) {
        namevals[name] = val;
    }
    std::string get_help() const { return global_help_options;}

    template <typename T> void get_global_config(const std::string& name, T* val, const std::string& help) {
        std::stringstream s;
        s << "   -S " << name << "=" << *val << "    " << help << " (" << name << ")\n";
        global_help_options += s.str(); // add the help in

        auto it = namevals.find(name);
        if (it != namevals.end() && val) {
            set_from_string(val, it->second);
        }
    }

    /* Find options */
    struct {
        std::vector<std::filesystem::path> files {};     // accumulates pattern files
        std::vector<std::string> patterns {};            // accumulates cmdline patterns
    } FindOpts {};

    bool find_opts_empty() const {
        return FindOpts.files.empty() && FindOpts.patterns.empty();
    }

    // Find interface
    const std::vector<std::string> &find_patterns() const        { return FindOpts.patterns; }
    const std::vector<std::filesystem::path> &find_files() const { return FindOpts.files; }
    void add_find_pattern(std::string pattern)                   { FindOpts.patterns.push_back(pattern);}
    void add_find_path(std::filesystem::path path)               { FindOpts.files.push_back(path);}


    size_t context_window_default{16}; // global option
    uint64_t offset_add{0}; // add this number to the first offset in every feature file (used for parallelism)
    std::filesystem::path banner_file{}; // add the contents of this file to the top of every feature file
    static inline const uint32_t DEFAULT_MAX_DEPTH {12};
    static inline const uint32_t DEFAULT_MAX_NGRAM {10};
    virtual ~scanner_config(){};
    scanner_config(){};
    scanner_config(const scanner_config&) = default;
    std::filesystem::path input_fname {NO_INPUT}; // where input comes from
    std::filesystem::path outdir {NO_OUTDIR};     // where output goes
    std::string hash_algorithm {"sha1"};          // which hash algorithm are using; default to SHA1

    bool allow_recurse { true };         // can be turned off for testing

    inline static const std::string NO_INPUT = "<NO-INPUT>"; // 'filename' indicator that the FRS has no input file
    inline static const std::string NO_OUTDIR = "<NO-OUTDIR>"; // 'dirname' indicator that the FRS produces no file output
    inline static const std::string CARVE_MODE_SUFFIX = "_carve_mode";

    std::string get_nameval(std::string name) const {
        auto it = namevals.find(name);
        return it != namevals.end() ? it->second  : "";
    }

    int get_carve_mode(const std::string name) const {
        std::string option_name = name + CARVE_MODE_SUFFIX;
        config_t::const_iterator it = namevals.find(option_name);
        if (it == namevals.end()) return -1;
        return std::stoi( std::string(it->second));
    }

    /* Set configuration; added to the static config */
    uint32_t max_depth {DEFAULT_MAX_DEPTH};
    uint32_t max_ngram {DEFAULT_MAX_NGRAM};                         // maximum ngram size to scan for

    /* Control which scanners are enabled */
    // enable/disable a specific scanner
    void push_scanner_command(const std::string& scannerName, scanner_command::command_t c) {
        scanner_commands.push_back(scanner_command(scannerName, c));
    }
    void enable_all_scanners() {
        push_scanner_command(scanner_command::ALL_SCANNERS, scanner_command::ENABLE);
    }
    void disable_all_scanners() {
        push_scanner_command(scanner_command::ALL_SCANNERS, scanner_command::DISABLE);
    }
};

#endif

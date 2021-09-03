#ifndef BULK_EXTRACTOR_RESTARTER_H
#define BULK_EXTRACTOR_RESTARTER_H

#include "config.h"

#include <unistd.h>
#include <sstream>
#include <string>
#include <exception>
#include <filesystem>

#include "be13_api/formatter.h"

#ifdef HAVE_EXPAT_H
#include <expat.h>
#endif

#include "phase1.h"

class bulk_extractor_restarter {
    std::stringstream       cdata {};
    std::string             thisElement {};
    std::string             provided_filename {};
    scanner_config          &sc;
    Phase1::Config          &cfg;
public:;
    bulk_extractor_restarter(scanner_config &sc_,
                             Phase1::Config &cfg_):sc(sc_),cfg(cfg_){};
    class CantRestart : public std::exception {
        std::string m_error{};
    public:;
        CantRestart(std::string_view error): m_error(error) {};
        const char* what() const noexcept override { return m_error.c_str(); }
    };
#ifdef HAVE_LIBEXPAT
    static void startElement(void *userData, const char *name_, const char **attrs) {
        class bulk_extractor_restarter &self = *(bulk_extractor_restarter *)userData;
        self.cdata.str("");
        self.thisElement = name_;
        if (self.thisElement=="debug:work_start"){
            for(int i=0;attrs[i] && attrs[i+1];i+=2){
                if (strcmp(attrs[i],"pos0") == 0){
                    self.cfg.seen_page_ids.insert(attrs[i+1]);
                }
            }
        }
    }
    static void endElement(void *userData,const char *name_){
        class bulk_extractor_restarter &self = *(bulk_extractor_restarter *)userData;
        if (self.thisElement=="provided_filename") self.provided_filename = self.cdata.str();
        self.cdata.str("");
    }
    static void characterDataHandler(void *userData,const XML_Char *s,int len){
        class bulk_extractor_restarter &self = *(bulk_extractor_restarter *)userData;
        self.cdata.write(s,len);
    }
    void restart() {
        std::filesystem::path report_path = sc.outdir / Phase1::REPORT_FILENAME;

        XML_Parser parser = XML_ParserCreate(NULL);
        XML_SetUserData(parser, this);
        XML_SetElementHandler(parser, startElement, endElement);
        XML_SetCharacterDataHandler(parser,characterDataHandler);
        std::fstream in(report_path);
        if (!in.is_open()){
            throw std::runtime_error( Formatter() << "Cannot open " << report_path << ": " << strerror(errno));
        }
        try {
            bool error = false;
            std::string line;
            while(getline(in,line) && !error){
                if (!XML_Parse(parser, line.c_str(), line.size(), 0)) {
                    std::cout << "XML Error: " << XML_ErrorString(XML_GetErrorCode(parser))
                              << " at line " << XML_GetCurrentLineNumber(parser) << "\n";
                    error = true;
                    break;
                }
            }
            if (!error) XML_Parse(parser, "", 0, 1);    // clear the parser
        }
        catch (const std::exception &e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
        XML_ParserFree(parser);
        in.close();
        /* Now rename the report filename */
        std::filesystem::path report_path_bak = report_path.string() + "." + std::to_string(time( nullptr));
        std::filesystem::rename(report_path, report_path_bak);
    }
#else
    void restart(Phase1::Config &cfg, scanner_config &sc) {
        throw std::runtime_error("Compiled without libexpat; cannot restart.");
    }
#endif
};
#endif

#include "config.h"

#include "path_printer.h"
#include "scanner_params.h"
#include "scanner_set.h"
#include "utils.h"
#include "formatter.h"

std::string PrintOptions::get(std::string key, std::string default_) const
{
    auto search = this->find(key);
    if (search != this->end()) {
        return search->second;
    }
    return default_;
}


void PrintOptions::add_rfc822_header(std::ostream &os,std::string line)
{
    size_t colon = line.find(":");
    if (colon==std::string::npos){
        os << "HTTP/1.1 502 Malformed HTTP request" << HTTP_EOL;
        return;
    }
    std::string name = line.substr(0,colon);
    std::string val  = line.substr(colon+1);
    while(val.size()>0 && (val[0]==' '||val[0]=='\t')) val = val.substr(1);
    (*this)[name]=val;
}


/**
 * lowerstr - Turns an ASCII string into all lowercase case (should be UTF-8)
 */

std::string path_printer::lowerstr(const std::string str)
{
    std::string ret;
    for (auto ch:str){
	ret.push_back(tolower(ch));
    }
    return ret;
}



/***************************************************************************************
 *** PATH PRINTER - Used by bulk_extractor for printing pages associated with a path ***
 ***************************************************************************************/

/* Get the next token from the path. Tokens are separated by dashes.
 * NOTE: modifies argument
 */
std::string path_printer::get_and_remove_token(std::string &path)
{
    while(path[0]=='-'){
	path = path.substr(1); // remove any leading dashes.
    }
    size_t dash = path.find('-');	// find next dash
    if (dash==std::string::npos){		// no string; return the rest
        std::string prefix = path;
	path = "";
	return prefix;
    }
    std::string prefix = path.substr(0,dash);
    path = path.substr(dash+1);
    return prefix;
}

path_printer::path_printer(scanner_set &ss_, abstract_image_reader *reader_, std::ostream &os_):
    ss(ss_),
    reader(reader_),
    out(os_)
{
    ss.phase_scan();                   // advance to phase scan
}


void path_printer::process_sp(const scanner_params &sp) const
{
    /* 1. Get next token
     * 2a  if prefix part is a number, skip forward that much in sbuf and repeat.
     * 2b  if the prefix is PRINT, print the buffer
     * 2c. if next part is a string, strip it and run that scanner. It will make a recursive call.
           when recurse() is called, it should call the this path_printer recursively!
     * 3. When we print, throw an exception following the printing to prevent continued analysis of buffer.
     * 4. We catch the exception. This way the sbufs never need to be deleted, so we can allocate them on the stack.
     */

    std::string new_path = sp.pp_path;
    std::string prefix = get_and_remove_token(new_path);

    /* Time to print ?*/
    if (prefix.size()==0 || prefix==PRINT || (prefix=="0" && new_path=="PRINT")){

	uint64_t print_start = 0;
	uint64_t content_length = 0;

        const auto it2 = sp.pp_po->find("Range");
	if (it2!=sp.pp_po->end()){
	    if (it2->second[5]=='='){
		size_t dash = it2->second.find('-');
                std::string v1 = it2->second.substr(6,dash-6);
                std::string v2 = it2->second.substr(dash+1);
		print_start = stoi64(v1);
		content_length = stoi64(v2)-print_start+1;
	    }
	} else {
            print_start = 0;
            content_length = stoi64( sp.pp_po->get(CONTENT_LENGTH, DEFAULT_CONTENT_LENGTH));
        }

	if (print_start > sp.sbuf->bufsize){
	    content_length = 0;			// can't print anything
	}

	if (content_length>0 && print_start+content_length > sp.sbuf->bufsize){
	    content_length = sp.sbuf->bufsize-print_start;
	}

        if (sp.pp_po->content_length!=0) {
            content_length = sp.pp_po->content_length;
        }

	switch (sp.pp_po->print_mode){
	case PrintOptions::MODE_HTTP:
	    os << "Content-Length: "		<< content_length  << PrintOptions::HTTP_EOL;
	    os << "Content-Range: bytes "	<< print_start << "-" << print_start+content_length-1 << PrintOptions::HTTP_EOL;
	    os << "X-Range-Available: bytes " << 0 << "-" << sp.sbuf->bufsize-1 << PrintOptions::HTTP_EOL;
	    os << PrintOptions::HTTP_EOL;
	    sp.sbuf->raw_dump(os, print_start, content_length); // send to stdout as binary
	    break;
	case PrintOptions::MODE_RAW:
	    os << content_length << PrintOptions::HTTP_EOL;
	    sp.sbuf->raw_dump(os, print_start, content_length); // send to stdout as binary
	    break;
	case PrintOptions::MODE_HEX:
	    sp.sbuf->hex_dump(os, print_start, content_length);
	    break;
	case PrintOptions::MODE_NONE:
	    break;
	}
        throw path_printer_finished();   // our job here is done
    }
    /* If we are in an offset block, process recursively with the offset */
    if (isdigit(prefix[0])){
	size_t offset = stoi64(prefix);
	if ( offset > sp.sbuf->bufsize ){
            throw std::runtime_error(Formatter() << "Error: " << new_path << " only has "
                                     << sp.sbuf->bufsize << " bytes; can't offset to " << offset);
	}
        sbuf_t *child = sp.sbuf->new_slice(pos0_t(new_path), offset, (size_t)(sp.sbuf->bufsize - offset));
        try {
            scanner_params sp2(sp, child, new_path);
            process_sp( sp2 );
        } catch (path_printer_finished &e) {
        }
        delete child;
	return;
    }
    /* Find the scanner and use it */
    auto *s = sp.ss->get_scanner_by_name(lowerstr(prefix));
    if (!s) {
        // TODO: send a better error message out?
        std::cerr << "Unknown scanner in path: '" << prefix << "'\n";
        return;
    }
    // we use the slice method because .. well, it works!
    sbuf_t *child = sp.sbuf->new_slice(pos0_t(), 0, sp.sbuf->bufsize);
    try {
        // when the scanner calls sp.recurse(), it will cause
        // additional decoding or the printing to take place.
        scanner_params sp2(sp, child, new_path);
        (*s)(sp2);
    } catch (path_printer_finished &e) {
        // nothing to do after printing is finished!
    }
    delete child;
}

/**
 * process_path uses the scanners to decode the path for the purpose of
 * decoding the image data and extracting the information.
 */

void path_printer::display_path(std::string path, const PrintOptions &po) const
{
    if ( reader==nullptr) {
	if (po.http_mode) {
	    os << "HTTP/1.1 502 Filename not provided" << PrintOptions::HTTP_EOL << PrintOptions::HTTP_EOL;
	} else {
            os << "Filename not provided\n";
	}
        return;
    }

    std::string  prefix = get_and_remove_token(path);
    int64_t offset = stoi64(prefix);

    /* Get the offset into the buffer process */
    sbuf_t  *sbufp = sbuf_t::sbuf_malloc( pos0_t("", offset), po.process_path_bufsize, po.process_path_bufsize);
    ssize_t count = reader->pread( reinterpret_cast<char *>(sbufp->malloc_buf()), po.process_path_bufsize, offset);
    if (count<0){
        os << strerror(errno) << " (Read Error)\n";
        delete sbufp;
	return;
    }
    sbufp = sbufp->realloc(count);       // resize to what was read

    /* make up a feature recorder set and with a disabled feature recorder.
     * Then we call the path printer, which throws an exception after the printing
     * to prevent further printing.
     *
     * The printer is called when a PRINT token is found in the
     * forensic path, so that has to be added.
     */
    feature_recorder_set::flags_t flags;
    flags.disabled = true;
    flags.no_alert = true;
    scanner_config sc;                  // defaults to NO_INPUT, NO_OUTPUT

    /* When we recurse, we store the path we are decoding and the print options in the scanner_params */

    feature_recorder_set fs(flags, sc);
    scanner_params sp(sc, &ss, this, scanner_params::phase_t::PHASE_SCAN, sbufp);
    sp.pp_po   = &po;
    sp.pp_path = path+"-PRINT";
    try {
        process_sp(sp);
    } catch (path_printer_finished &e) {
        // nothing to do after printing is finished!
    }
    delete sbufp;
}


/**
 * process a path for a given filename.
 * Opens the image and calls the function above.
 * Also implements HTTP server with "-http" option.
 * Feature recorders disabled.
 */
void path_printer::process_path(std::string path)
{
    /* Just get a path and display it */

    PrintOptions po;
    if (ends_with(path,"/r")) {
	path = path.substr(0,path.size()-2);
	po.print_mode = PrintOptions::MODE_RAW;
    }
    if (ends_with(path,"/h")) {
	path = path.substr(0,path.size()-2);
	po.print_mode = PrintOptions::MODE_HEX;
    }

    /* See if there is an extent */
    size_t colon = path.find(':');
    if (colon != std::string::npos) {
        po.content_length = stoi64( path.substr(colon+1) );
        path.resize(colon);
    }

    display_path(path, po);
    out << os.str();
    os.str(std::string());
}


void path_printer::process_interactive(std::istream &is)
{
    out << "Path Interactive Mode: (enter '.' to abort)\n";
    std::string path;
    while (!is.eof()) {
        getline(is, path);
        if (is.eof()) break;
        if (path==".") break;
        PrintOptions po;
        po.print_mode = PrintOptions::MODE_HEX;
        display_path(path, po);
    };
    return;
}

void path_printer::process_http(std::istream &is)
{
    while (!is.eof()) {
        /* get the HTTP query */
        std::string line;		// the specific query
        PrintOptions po;
        po.http_mode  = true;
        po.print_mode = PrintOptions::MODE_HTTP;	// options for this query

        getline(is, line);
        truncate_at(line,'\r');         // truncate at \r if we end with \r\n
        if (line.substr(0,4)!="GET "){
            out << "HTTP/1.1 501 Method not implemented" << PrintOptions::HTTP_EOL << PrintOptions::HTTP_EOL;
            return;
        }
        size_t space = line.find(" HTTP/1.1");
        if (space==std::string::npos){
            out << "HTTP/1.1 501 Only HTTP/1.1 is implemented" << PrintOptions::HTTP_EOL << PrintOptions::HTTP_EOL;
            return;
        }
        std::string p2 = line.substr(4,space-4);

        /* Get the additional header options */
        do {
            getline(is,line);
            truncate_at(line,'\r');
            if (line.size()==0) break;         // newline
            po.add_rfc822_header(out, line);
        } while(true);

        /* Process some specific URLs */
        if (p2=="/info"){
            out << "X-Image-Size: " << reader->image_size() << PrintOptions::HTTP_EOL;
            out << "X-Image-Filename: " << reader->image_fname() << PrintOptions::HTTP_EOL;
            out << "Content-Length: 0" << PrintOptions::HTTP_EOL;
            out << PrintOptions::HTTP_EOL;
            continue;
        }

        /* Ready to go with path and options */
        display_path(p2, po);
    }
}

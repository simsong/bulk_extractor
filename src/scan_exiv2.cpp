/**
 * scan_exiv2: - requires exiv2 library to be installed.
 * Revision history:
 * 2011-jun-29 slg - Increase exif_gulp_size to 1MB (is this needed at all anymore?)
 *                 - Added support for getting GPS information from photo and adding it to gps.txt
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "be13_api/utils.h"

#include "dfxml/src/dfxml_writer.h"

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>

//#include "md5.h"

#ifdef HAVE_EXIV2

/* exiv2 has errors */
#ifdef HAVE_DIAGNOSTIC_SHADOW
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#ifdef HAVE_DIAGNOSTIC_EFFCPP
#pragma GCC diagnostic ignored "-Weffc++"
#endif

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/error.hpp>

const size_t min_exif_size = 4096;
const size_t exif_gulp_size = 1024*1024;	// how many bytes of EXIF to read


using namespace std;

/** 
 * jpeg_start() returns true if the buffer starts with a jpeg.
 */
inline bool jpeg_start(const sbuf_t &sbuf){
    return (sbuf[0]==(unsigned char)'\xff'
	    && sbuf[1]==(unsigned char)'\xd8'
	    && sbuf[2]==(unsigned char)'\xff');
}

static size_t min(size_t a,size_t b)
{
    return a<b ? a : b;
}

/**
 * Used for helping to convert libexiv2's time format to ISO8601
 */
static char slash_to_dash(char ch)
{
    if(ch=='/') return '-';
    return ch;
}


/**
 * Used for helping to convert libexiv2's GPS format to decimal lat/long
 */

static double sub_stod(string s)
{
    double d=0;
    sscanf(s.c_str(),"%lf",&d);
    return d;
}

static double rational(string s)
{
    std::vector<std::string> parts = split(s,'/');
    if(parts.size()!=2) return sub_stod(s);	// no slash, so return without
    double top = sub_stod(parts[0]);
    double bot = sub_stod(parts[1]);
    return bot>0 ? top / bot : top;
}

static string fix_gps(string s)
{
    std::vector<std::string> parts = split(s,' ');
    if(parts.size()!=3) return s;	// return the original
    double res = rational(parts[0]) + rational(parts[1])/60.0 + rational(parts[2])/3600.0;
    s = dtos(res);
    return s;
}

static string fix_gps_ref(string s)
{
    if(s=="W" || s=="S") return "-";
    return "";
}

int exif_show_all=1;
extern "C"
void scan_exiv2(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "exiv2";
        sp.info->author         = "Simson L. Garfinkel";
        sp.info->description    = "Searches for EXIF information using exiv2";
        sp.info->scanner_version= "1.0";
	sp.info->feature_names.insert("exif");
	sp.info->feature_names.insert("gps");
	sp.info->flags = scanner_info::SCANNER_DISABLED; // disabled because we have be_exif
	return;
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN){

	const sbuf_t &sbuf = sp.sbuf;
	feature_recorder *exif_recorder = sp.fs.get_name("exif");
	feature_recorder *gps_recorder  = sp.fs.get_name("gps");

#ifdef HAVE_EXIV2__LOGMSG__SETLEVEL
	/* New form to suppress error messages on exiv2 */
	Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);
#endif    

	size_t pos_max = 1;		// by default, just scan 1 byte
	if(sbuf.bufsize > min_exif_size){
	    pos_max = sbuf.bufsize - min_exif_size; //  we can scan more!
	}
    
	/* Loop through all possible locations in the buffer */
	for(size_t pos=0;pos<pos_max;pos++){
	    size_t count = exif_gulp_size;
	    count = min(count,sbuf.bufsize-pos);
	    //size_t count = sbuf.bufsize-pos; // use all to end

	    /* Explore the beginning of each 512-byte block as well as the starting location
	     * of any JPEG on any boundary. This will cause processing of any multimedia file
	     * that Exiv2 recognizes (for which I do not know all the headers.
	     */
	    if(pos%512==0 || jpeg_start(sbuf+pos)){
		try {
		    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(sbuf.buf+pos,count);
		    if(image->good()){
			image->readMetadata();
		    
			Exiv2::ExifData &exifData = image->exifData();
			if (exifData.empty()) continue;
		    
			/*
			 * Create the MD5 of the first 4K to use as a unique identifier.
			 */
			sbuf_t tohash(sbuf,0,4096);
			string md5_hex = exif_recorder->fs.hasher.func(tohash.buf,tohash.bufsize);

			char xmlbuf[1024];
			snprintf(xmlbuf,sizeof(xmlbuf),
				 "<exiv2><width>%d</width><height>%d</height>",
				 image->pixelWidth(),image->pixelHeight());

			/* Scan through the fields reported by exiv2.
			 * Create XML for each and extract GPS information if present.
			 */
			string photo_time;	// use this when gps_time is not available
			string gps_time;
			string gps_date;
			string gps_lat_ref;
			string gps_lat;
			string gps_lon_ref;
			string gps_lon;
			string gps_ele;
			string gps_speed;
			string gps_course;

                        bool has_gps = false;
                        bool has_gps_date = false;

			string xml = xmlbuf;
			for (Exiv2::ExifData::const_iterator i = exifData.begin();
			    i != exifData.end(); ++i) {

			    if(i->count()>1000) continue;	// ignore long ones

			    string key = string(i->key());
			    xml.append("<"+key+">"+dfxml_writer::xmlescape(i->value().toString())+"</"+key+">");

                            // use Date from Photo unless date from GPS is available
			    if(key=="Exif.Photo.DateTimeOriginal"){
				photo_time = i->value().toString();

				/* change "2011/06/25 12:20:11" to "2011-06-25 12:20:11" */
				std::transform(photo_time.begin(),photo_time.end(),photo_time.begin(),slash_to_dash);

				/* change "2011:06:25 12:20:11" to "2011-06-25 12:20:11" */
                                if (photo_time[4] == ':') {
                                    photo_time[4] = '-';
                                }
                                if (photo_time[7] == ':') {
                                    photo_time[7] = '-';
                                }

				/* Change "2011-06-25 12:20:11" to "2011-06-25T12:20:11" */
				size_t first_space = photo_time.find(' ');
				if(first_space!=string::npos) {
                                   photo_time.replace(first_space,1,"T");
                                }
			    } else if(key=="Exif.GPSInfo.GPSTimeStamp") {
                        	has_gps = true;
                                has_gps_date = true;
				gps_time = i->value().toString();
                                // reformat timestamp to standard ISO8601
                                // change "12 20 11" to "12:20:11"
                                if (gps_time.length() == 8) {
                                    if (gps_time[4] == ' ') {
                                        gps_time[4] = ':';
                                    }
                                    if (gps_time[7] == ' ') {
                                        gps_time[7] = ':';
                                    }
                                }
			    } else if(key=="Exif.GPSInfo.GPSDateStamp") {
                        	has_gps = true;
                                has_gps_date = true;
				gps_date = i->value().toString();
                                // reformat timestamp to standard ISO8601
                                // change "2011:06:25" to "2011-06-25"
                                if (gps_date.length() == 10) {
                                    if (gps_date[4] == ':') {
                                        gps_date[4] = '-';
                                    }
                                    if (gps_date[7] == ':') {
                                        gps_date[7] = '-';
                                    }
                                }

			    } else if(key=="Exif.GPSInfo.GPSLongitudeRef"){
                                has_gps = true;
				gps_lon_ref = fix_gps_ref(i->value().toString());
			    } else if(key=="Exif.GPSInfo.GPSLongitude"){
                                has_gps = true;
				gps_lon = fix_gps(i->value().toString());
			    } else if(key=="Exif.GPSInfo.GPSLatitudeRef"){
                                has_gps = true;
				gps_lat_ref = fix_gps_ref(i->value().toString());
			    } else if(key=="Exif.GPSInfo.GPSLatitude"){
                                has_gps = true;
				gps_lat = fix_gps(i->value().toString());
			    } else if(key=="Exif.GPSInfo.GPSAltitude"){
                                has_gps = true;
				gps_ele   = dtos(rational(i->value().toString()));
			    } else if(key=="Exif.GPSInfo.GPSSpeed"){
                                has_gps = true;
				gps_speed = dtos(rational(i->value().toString()));
			    } else if(key=="Exif.GPSInfo.GPSTrack"){
                                has_gps = true;
				gps_course = i->value().toString();
			    }
			}
			xml.append("</exiv2>");

                        // record EXIF
			exif_recorder->write(sp.sbuf.pos0+pos,md5_hex,xml);

                        // record GPS
                        if (has_gps) {
                            if (has_gps_date) {
                                // record the GPS entry using the GPS date
                                gps_recorder->write(sp.sbuf.pos0+pos,md5_hex,
					    gps_date+"T"+gps_time+","+gps_lat_ref+gps_lat+","
					    +gps_lon_ref+gps_lon+","
					    +gps_ele+","+gps_speed+","+gps_course);

                            } else {
                                // record the GPS entry using the date obtained from Photo
                                gps_recorder->write(sp.sbuf.pos0+pos,md5_hex,
					    photo_time+","+gps_lat_ref+gps_lat+","
					    +gps_lon_ref+gps_lon+","
					    +gps_ele+","+gps_speed+","+gps_course);
			    }
			}
		    }
		}
		catch (Exiv2::AnyError &e) { }
		catch (std::exception &e) { }
	    }
	}
    }
}

#endif

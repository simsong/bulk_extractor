#ifndef __AFTIMER_H__
#define __AFTIMER_H__

#include <atomic>
#include <ctime>
#include <cstdio>
#include <cassert>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

#include "utils.h"

/**
 * threadsafe timer.
 */
class aftimer {
    aftimer(const aftimer & s) = delete;
    aftimer & operator=(const aftimer &s) = delete;
    std::chrono::time_point<std::chrono::steady_clock> t0 {};
    std::atomic<bool>     running    {};
    std::atomic<uint64_t> elapsed_ns {}; //  for all times we have started and stopped
    std::atomic<uint64_t> last_ns    {}; // time from when we last did a "start"
public:
    static std::string now_str(std::string prefix="",std::string suffix="");              // return a high-resolution string as now.
    static std::string hms_str(long t);             // turn a number of seconds into h:m:s
    static std::string hms_ns_str(uint64_t ns);     // turn a number of nanoseconds into h:m:s
    static const uint64_t ns_per_s = 1000*1000*1000; // seconds per nanoseconds
    aftimer()  {}

    void start(); // start the timer
    void stop();  // stop the timer
    void lap();   // note the time for elapsed_seconds() below

    uint64_t running_nanoseconds() const;             // for how long have we been running?
    double elapsed_seconds() const;                   // how long timer has been running; timer can be running from the beginning
    uint64_t elapsed_nanoseconds() const;
    uint64_t lap_seconds() const;                     // how long the timer is running this time
    double eta(double fraction_done) const;           // calculate ETA in seconds, given fraction
    std::string elapsed_text() const;                 // how long we have been running
    std::string eta_text(double fraction_done) const; // h:m:s
    std::string eta_time(double fraction_done) const; // the actual time
    std::string eta_date(double fraction_done) const; // the actual date and time
};

/* This code is from:
 * http://social.msdn.microsoft.com/Forums/en/vcgeneral/thread/430449b3-f6dd-4e18-84de-eebd26a8d668
 * and:
 * https://gist.github.com/ugovaretto/5875385
 */

// https://stackoverflow.com/questions/16177295/get-time-since-epoch-in-milliseconds-preferably-using-c11-chrono
inline std::string aftimer::now_str(std::string prefix,std::string suffix) {
    //uint64_t nanoseconds_since_epoch  = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
    //std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    uint64_t microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::stringstream ss;
    ss << std::setprecision(4) << std::fixed << prefix << microseconds_since_epoch/1000 << suffix;
    return ss.str();
}

inline std::string aftimer::hms_str(long t)  {
    char buf[64];
    int days = t / (60 * 60 * 24);

    t = t % (60 * 60 * 24); /* what's left */

    int h = t / 3600;
    int m = (t / 60) % 60;
    int s = t % 60;
    buf[0] = 0;
    switch (days) {
    case 0: snprintf(buf, sizeof(buf), "%2d:%02d:%02d", h, m, s); break;
    case 1: snprintf(buf, sizeof(buf), "%d day, %2d:%02d:%02d", days, h, m, s); break;
    default: snprintf(buf, sizeof(buf), "%d days %2d:%02d:%02d", days, h, m, s);
    }
    return std::string(buf);
}

inline std::string aftimer::hms_ns_str(uint64_t ns)  {
    return hms_str(ns / ns_per_s);
}

inline void aftimer::start() {
    assert (running == false);
    t0 = std::chrono::steady_clock::now();
    running = true;
}

inline uint64_t aftimer::running_nanoseconds() const {
    auto v = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0 );
    return v.count();
}

inline void aftimer::stop() {
    assert (running==true);
    last_ns = running_nanoseconds();
    elapsed_ns += last_ns;
    running = false;
}

inline void aftimer::lap() {
    stop();
    start();
}

inline uint64_t aftimer::elapsed_nanoseconds() const {
    if (running) {
        return elapsed_ns + running_nanoseconds();
    } else {
        return elapsed_ns;
    }
}

inline double aftimer::elapsed_seconds() const {
    return elapsed_nanoseconds() / double(ns_per_s);
}

inline std::string aftimer::elapsed_text() const {
    return hms_str((int)elapsed_seconds());
}

/**
 * returns the number of seconds until the job is complete.
 */
inline double aftimer::eta(double fraction_done) const {
    double t = elapsed_seconds();
    if (t <= 0) return -1;             // can't figure it out
    if (fraction_done <= 0) return -1; // can't figure it out
    return (t * 1.0 / fraction_done - t);
}

/**
 * Retuns the number of hours:minutes:seconds until the job is done.
 */
inline std::string aftimer::eta_text(double fraction_done) const {
    double e = eta(fraction_done);
    if (e < 0) return std::string("n/a"); // can't figure it out
    return hms_str((long)e);
}

/**
 * Returns the time when data is due.
 */
inline std::string aftimer::eta_time(double fraction_done) const {
    time_t t = time_t(eta(fraction_done)) + time(0);
    struct tm tm;
    localtime_r(&t, &tm);
    char buf[64];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

inline std::string aftimer::eta_date(double fraction_done) const {
    time_t t = time_t(eta(fraction_done)) + time(0);
    struct tm tm;
    localtime_r(&t, &tm);
    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

#endif

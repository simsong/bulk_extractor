#include "notify_thread.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
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

int notify_thread::terminal_width( int default_width )
{
#if defined(HAVE_IOCTL) && defined(HAVE_STRUCT_WINSIZE_WS_COL)
    struct winsize ws;
    if ( ioctl( STDIN_FILENO, TIOCGWINSZ, &ws)==0){
        return  ws.ws_col;
    }
#endif
    return default_width;
}



void notify_thread::notifier( struct notify_thread::notify_opts *o)
{
    assert( o->ssp != nullptr);
    const char *cl="";
    const char *ho="";
    const char *ce="";
    const char *cd="";
    int cols = 80;
#ifdef HAVE_LIBTERMCAP
    char buf[65536], *table=buf;
    cols = tgetnum( "co" );
    if ( !o->cfg.opt_legacy) {
        const char *str = ::getenv( "TERM" );
        if ( !str){
            std::cerr << "Warning: TERM environment variable not set." << std::endl;
        } else {
            switch ( tgetent( buf, str)) {
            case 0:
                std::cerr << "Warning: No terminal entry '" << str << "'. " << std::endl;
                break;
            case -1:
                std::cerr << "Warning: terminfo database culd not be found." << std::endl;
                break;
            case 1: // success
                ho = tgetstr( "ho", &table );   // home
                cl = tgetstr( "cl", &table );   // clear screen
                ce = tgetstr( "ce", &table );   // clear to end of line
                cd = tgetstr( "cd", &table );   // clear to end of screen
                break;
            }
        }
    }
#endif

    std::cout << cl;                    // clear screen
    while( true ){

        if (o->phase>1) break;

        // get screen size change if we can!
        cols = terminal_width( cols);
        time_t rawtime = time ( 0);
        struct tm timeinfo = *( localtime( &rawtime ));
        std::map<std::string,std::string> stats = o->ssp->get_realtime_stats();

        stats["elapsed_time"] = o->master_timer->elapsed_text();
        if ( o->fraction_done ) {
            double done = *o->fraction_done;
            stats[FRACTION_READ] = std::to_string( done * 100) + std::string( " %" );
            stats[ESTIMATED_TIME_REMAINING] = o->master_timer->eta_text( done );
            stats[ESTIMATED_DATE_COMPLETION] = o->master_timer->eta_date( done );

            // print the legacy status
            if ( o->cfg.opt_legacy) {
                char buf1[64], buf2[64];
                snprintf( buf1, sizeof( buf1), "%2d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
                snprintf( buf2, sizeof( buf2), "(%.2f%%)", done * 100);
                uint64_t max_offset = strtoll( stats[ scanner_set::MAX_OFFSET ].c_str() , nullptr, 10);
                std::cout << buf1 << " Offset " << max_offset / ( 1000*1000) << "MB "
                          << buf2 << " Done in " << stats[ESTIMATED_TIME_REMAINING]
                          << " at " << stats[ESTIMATED_DATE_COMPLETION] << std::endl;
            }
        }
        if ( !o->cfg.opt_legacy) {
            std::cout << ho << "bulk_extractor      " << asctime( &timeinfo) << "  " << std::endl;
            for( const auto &it : stats ){
                std::cout << it.first << ": " << it.second;
                if ( ce[0] ){
                    std::cout << ce;
                } else {
                    // Space out to the 50 column to erase any junk
                    int spaces = 50 - ( it.first.size() + it.second.size());
                    for( int i=0;i<spaces;i++){
                        std::cout << " ";
                    }
                }
                std::cout << std::endl;
            }
            if ( o->fraction_done ){
                if ( cols>10){
                    double done = *o->fraction_done;
                    int before = ( cols - 3) * done;
                    int after  = ( cols - 3) * ( 1.0 - done );
                    std::cout << std::string( before,'=') << '>' << std::string( after,'.') << '|' << ce << std::endl;
                }
            }
            std::cout << cd << std::endl << std::endl;
        }
        std::this_thread::sleep_for( std::chrono::seconds( o->cfg.opt_notify_rate ));
    }
    for(int i=0;i<5;i++){
        std::cout << std::endl;
    }
    std::cout << "Computing final histograms and shutting down...\n";
    return;                             // end of notifier thread.
}

void notify_thread::launch_notify_thread( struct notify_thread::notify_opts *o)
{
    new std::thread( &notifier, o);    // launch the notify thread
}

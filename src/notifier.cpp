void Phase1::notify_user(image_process::iterator &it)
{
    if (notify_ctr++ >= config.opt_notify_rate){
        time_t t = time(0);
        struct tm tm;
        localtime_r(&t,&tm);
        printf("%2d:%02d:%02d %s ",tm.tm_hour,tm.tm_min,tm.tm_sec,it.str().c_str());

        /* not sure how to do the rest if sampling */
        if (!sampling()){
            printf("(%4.2f%%) Done in %s at %s",
                   it.fraction_done()*100.0,
                   timer.eta_text(it.fraction_done()).c_str(),
                   timer.eta_time(it.fraction_done()).c_str());
        }
        printf("\n");
        fflush(stdout);
        notify_ctr = 0;
    }
}

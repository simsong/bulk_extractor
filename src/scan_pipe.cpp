#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <inttypes.h>

/*
 * Scanner that pipes all data to stdin of a subprocess. This has advantages:
 * - it's easy to write or reuse small standalone progams or plugins
 * - you can write/script in any language you like
 * - you don't need to compile your own bulk_extractor with dependencies
 *
 * and disadvantages:
 * - you can't use bulk_extractors internal functionality to output data on standard form
 * - you can't feed decoded output back into the process recursively
 * - you can't accumulate information across blocks
 *
 */

/* pipe_prog should usually point to an executable, or a script that looks like this:
 * ---
 * #!/bin/bash
 * # set required variables, as necessary
 * exec /path/to/my_program with arguments
 * ---
 */
static char *const pipe_prog[] = {"./pipe_prog", NULL};
static char *const pipe_env[] = {"PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin:/usr/local/sbin", NULL};
static long long int bytesread;

int max(int a, int b)
{
    if (a > b) return a;
    return b;
}

extern "C"
void scan_pipe(const class scanner_params &sp,const recursion_control_block &rcb)
{
    int child_stdin[2];
    int child_stdout[2];
    int child_stderr[2];
    int childpid;
    int cnt, ret, ret2;
    unsigned int written;

    assert(sp.version==canner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "pipe";
	sp.info->flags = scanner_info::SCANNER_DISABLED; // disabled for now
	sp.info->feature_names.insert("pipe");
	bytesread = 0;
	return;
    }
    if(sp.phase==scanner_params::shutdown) {
        // close...
        //char _scan_pipe_buf[33];
        //feature_recorder *pipe_recorder = sp.fs.get_name("pipe");
        //snprintf(_scan_pipe_buf, 32, "Got %lli chars", bytesread);
        //pipe_recorder->write(_scan_pipe_buf);
        return;
    }
    if(sp.phase==scanner_params::scan){

	// set up file descriptors for input and output, and exec command
	bytesread += sp.sbuf.bufsize;
    
	// pipe() returns 0 on success
	if (pipe(child_stdin)) { perror("pipe() stdin"); return; }
	if (pipe(child_stdout)) { perror("pipe() stdout"); return; }
	if (pipe(child_stderr)) { perror("pipe() stderr"); return; }

	childpid = fork();
	if (childpid == -1) return;
	if (childpid == 0) {
	    // child, set up final details and exec process
	    close(0); close(1); close(2);
	    dup2(child_stdin[0], 0); close(child_stdin[0]);
	    dup2(child_stdout[1], 1); close(child_stdout[1]);
	    dup2(child_stderr[1], 2); close(child_stderr[1]);
	    close(child_stdin[1]); close(child_stdout[0]); close(child_stderr[0]);
	    execve(pipe_prog[0], pipe_prog, pipe_env); // should never return
	    printf("execve error: %s: %s\n", pipe_prog[0], strerror(errno));
	    return;
	} else {
	    // main process, write data to child_stdin[1] and read data from child_stdout[0] and child_stderr[0]
	    fd_set rfds, wfds, efds;
	    int maxfd;
	    char _scan_pipe_buf[BUFSIZ];
	    close(child_stdin[0]); close(child_stdout[1]); close(child_stderr[1]);
	    written = 0;
	    feature_recorder *pipe_recorder = sp.fs.get_name("pipe");
	    for (;;) {
		ret2 = waitpid(childpid, &ret, WNOHANG);
		if (ret2 == -1) { perror("waitpid"); return; }
		if (ret2 == childpid) {
		    if (WIFEXITED(ret)) { // || WIFSIGNALED(ret)) {
			if (written < sp.sbuf.bufsize) close(child_stdin[1]);
			close(child_stdout[0]); close(child_stderr[0]);
			return;
		    }
		}
		FD_ZERO(&rfds); 
		FD_ZERO(&wfds); 
		FD_ZERO(&efds);
		// check for output on stdout and stderr
		FD_SET(child_stdout[0], &rfds); FD_SET(child_stderr[0], &rfds);
		// don't try to write any more if we're done already
		if (written < sp.sbuf.bufsize) FD_SET(child_stdin[1], &wfds);
		maxfd = max(child_stdin[1], max(child_stdout[0], child_stderr[0]));
		select(maxfd, &rfds, &wfds, NULL, NULL);
		if (FD_ISSET(child_stdout[0], &rfds)) {
		    cnt = 0;
		    // read until BUFSIZ
		    do {
			ret = read(child_stdout[0], _scan_pipe_buf, BUFSIZ);
			if (ret == -1) {
			    if (errno == EAGAIN || errno == EINTR) {
				continue;
			    }
			    perror("read child_stdout");
			    return;
			}
			break;
		    } while(1);
		    // while (cnt < (BUFSIZ - 1) && _scan_pipe_buf[cnt] != 0 && _scan_pipe_buf[cnt] != '\n');
		    _scan_pipe_buf[BUFSIZ - 1] = 0; // TODO: this might overwrite the final byte!
		    if (ret < BUFSIZ) _scan_pipe_buf[ret] = 0;
		    if (ret > 0) pipe_recorder->write(sp.sbuf.pos0,_scan_pipe_buf,string("STDOUT"));
		}
		if (FD_ISSET(child_stderr[0], &rfds)) {
		    cnt = 0;
		    // read until BUFSIZ
		    do {
			ret = read(child_stderr[0], _scan_pipe_buf, BUFSIZ);
			if (ret == -1) {
			    if (errno == EAGAIN || errno == EINTR) {
				continue;
			    }
			    perror("read child_stderr");
			    return;
			}
			break;
		    } while(1);
		    // while (cnt < (BUFSIZ - 1) && _scan_pipe_buf[cnt] != 0 && _scan_pipe_buf[cnt] != '\n');
		    _scan_pipe_buf[BUFSIZ - 1] = 0; // TODO: this might overwrite the final byte!
		    if (ret < BUFSIZ) _scan_pipe_buf[ret] = 0;
		    if (ret > 0) pipe_recorder->write(sp.sbuf.pos0,_scan_pipe_buf,string("STDERR"));
		}
		if (FD_ISSET(child_stdin[1], &wfds)) {
		    // write until BUFSIZ
		    do {
			ret = write(child_stdin[1], sp.sbuf.buf + written, BUFSIZ);
			if (ret == -1) {
			    if (errno == EAGAIN || errno == EINTR) {
				continue;
			    }
			    perror("write child_stdin");
			    return;
			}
			written += ret;
			break;
		    } while(1);
		    if (written >= sp.sbuf.bufsize) close(child_stdin[1]);
		}
	    }
	}
    }
}


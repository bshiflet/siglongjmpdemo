/*
  gcc -g -Wall setjmpdemo.c -o setjmpdemo && ./setjmpdemo; echo $?
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <setjmp.h> // setjmp/longjmp
bool SegExceptionHandler_registered = false;
sigjmp_buf SegExceptionHandler_sigjmp_buf;


void signalhandler(int sig)
{
    fprintf(stdout, "inside signalhandler: pid=%ld sig=%d name=%s\n", (long)getpid(), sig, strsignal(sig));
    if (sig == SIGSEGV) {
        if (SegExceptionHandler_registered)
            siglongjmp(SegExceptionHandler_sigjmp_buf, 1); // jump to where sigsetjmp() was called
    }
}


void process()
{
    // somewhere in process() something bad happens resulting in SIGSEV
    char *p = NULL;
    *p = 'w';

    // or just manually 'throw'
    siglongjmp(SegExceptionHandler_sigjmp_buf, 1);
}


void process_wrapper()
{
    SegExceptionHandler_registered = true;
    if (sigsetjmp(SegExceptionHandler_sigjmp_buf, 1) == 0)
        process();
    else
        fprintf(stdout, "sendEmail: process() failed, but we're still running!\n");

    SegExceptionHandler_registered = false;
}


int main(int argc, char *argv[])
{
    // Register to catch the signals
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &signalhandler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL); // we want SIGSEGV to call signalhandler

    fprintf(stdout, "Listening for connections...\n");

    process_wrapper();

    fprintf(stdout, "exiting SUCCESS\n");
    return EXIT_SUCCESS;
}

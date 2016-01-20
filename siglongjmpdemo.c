/*
  gcc -g -Wall siglongjmpdemo.c -o siglongjmpdemo && ./siglongjmpdemo; echo $?
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <setjmp.h> // sigsetjmp/siglongjmp
bool ExceptionHandler_registered = false;
sigjmp_buf ExceptionHandler_sigjmp_buf;


void signalhandler(int signal)
{
    fprintf(stdout, "in signalhandler: signal=%d name=%s\n", signal, strsignal(signal));
    if (signal == SIGSEGV || signal == SIGFPE) {
        if (ExceptionHandler_registered)
            siglongjmp(ExceptionHandler_sigjmp_buf, 1); // jump to where sigsetjmp() was called
    }
}


void process(int i)
{
    fprintf(stdout, "in process...\n");

    if (i == 0) {
        // somewhere in process() something bad happens resulting in SIGSEV
        char *p = NULL;
        *p = 'w';
    }

    if (i == 1) {
        // or something resulting in SIGFPE (Floating point exception)
        int y = 0;
        int x = 1 / y;
        y = x; // Avoid gcc warnings
    }

    fprintf(stdout, "in process...done\n");
}


void process_wrapper(int i)
{
    // sigsetjmp returns twice: once when called normally, then again if
    // siglongjmp() is called. The return value differentiates the 2 cases.
    ExceptionHandler_registered = true;
    if (sigsetjmp(ExceptionHandler_sigjmp_buf, 1) == 0)
        process(i);
    else {
        fprintf(stdout, "process failed (send a notification), but we're still running!\n");

        // At this point the stack has been restored to what it was prior to
        // the call to process(), but you may need to restore the heap.
    }

    ExceptionHandler_registered = false;
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
    sigaction(SIGFPE, &sa, NULL); // we want SIGFPE to call signalhandler

    fprintf(stdout, "Ready to process requests...\n");

    int i;
    for (i = 0; i < 3; ++i) {
        process_wrapper(i);
        sleep(1);
    }

    fprintf(stdout, "exiting SUCCESS\n");
    return EXIT_SUCCESS;
}

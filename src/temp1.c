#include <stdio.h>
#include <signal.h>
#include <time.h>

timer_t firstTimerID;
timer_t secondTimerID;
timer_t thirdTimerID;
sigset_t sigmask;       


static void timerHandler( int sig, siginfo_t *si, void *uc )
{
    timer_t *tidp;

    tidp = si->si_value.sival_ptr;

    if ( *tidp == firstTimerID )
      printf("1\n");
   else if ( *tidp == secondTimerID )
printf("2\n");
    else if ( *tidp == thirdTimerID )
        printf("3\n");;
}

static int makeTimer( char *name, timer_t *timerID, int expireMS, int intervalMS )
{
     //sigset_t mask;
    struct sigevent         te;
    struct itimerspec       its;
    struct sigaction        sa;
    int                     sigNo = SIGRTMIN;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(sigNo, &sa, NULL) == -1)
    {
        perror("sigaction");
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    te.sigev_value.sival_ptr = timerID;
    timer_create(CLOCK_REALTIME, &te, timerID);

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = intervalMS * 1000000;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = expireMS * 1000000;
    timer_settime(*timerID, 0, &its, NULL);


    return 1;
}

int main()
 {

     makeTimer("First Timer", &firstTimerID, 2, 2);   //2ms

   makeTimer("Second Timer", &secondTimerID, 10, 10);    //10ms
   makeTimer("Third Timer", &thirdTimerID, 100, 100);  //100ms


 }
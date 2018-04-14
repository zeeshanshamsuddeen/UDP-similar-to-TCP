#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
//#include <sys/siginfo.h> 


#include "packet.h"
#include "common.h"

#define STDIN_FD    0
#define RETRY  120 //milli second 

 #define SIG SIGRTMIN


int next_seqno=0;
int send_base=0;
int window_size = 1;

int sockfd, serverlen;
struct sockaddr_in serveraddr;
//struct itimerval timer; 
tcp_packet *sndpkt;
tcp_packet *recvpkt;
//sigmask was not an array
sigset_t sigmask;       
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)


void resend_packets(int sig, siginfo_t *si, void *uc)
{
    //VLOG(DEBUG,"resend_packets");
    //VLOG(DEBUG,"resend_packets of %d",sig);

    
        //Resend all packets range between 
        //sendBase and nextSeqNum
        printf("yessss\n");


    
}


			struct sigaction sa;
			struct sigevent sev;
			timer_t timerid;
           struct itimerspec its;
                      sigset_t mask;

int main ()
{


			printf("Establishing handler for signal \n");
           sa.sa_flags = SIGALRM;
           sa.sa_sigaction = resend_packets;
           sigemptyset(&sa.sa_mask);
           sigaction(SIGALRM, &sa, NULL);
			/* Handle error */;
			printf("Establishing handler for signal \n");

			sigemptyset(&mask);
           sigaddset(&mask, SIG);
           if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
               errExit("sigprocmask");




			sev.sigev_notify = SIGALRM;
			           sev.sigev_signo = SIG;
           sev.sigev_value.sival_ptr = &timerid;
           timer_create(CLOCK_REALTIME, &sev, &timerid);	
			printf("Establishing handler for signal \n");

		    its.it_value.tv_sec = 120 / 1000;
           its.it_value.tv_nsec = (120 % 1000) * 1000;
           its.it_interval.tv_sec = its.it_value.tv_sec;
           its.it_interval.tv_nsec = its.it_value.tv_nsec;

			printf("Establishing handler for signal aaaaaaa\n");

              if (timer_settime(timerid, 0, &its, NULL) == -1)
                errExit("timer_settime");
            while(1)
            {

            }




    return 0;

}




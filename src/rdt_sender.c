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
//#define WINDOW_SIZE 10    



int next_seqno=0;
int send_base=0;
//int window_size = 3;
int send_packet=0;
int free_space=0;
int temp=0;
int sockfd, serverlen;
struct sockaddr_in serveraddr;
struct itimerval timer; 
struct timeval RTT_start = {0};
struct timeval RTT_stop ={0};
long SampleRTT=0,EstimatedRTT=0,DevRTT=0,temp_timeout=0;
long TimeoutInterval=1000000;  /* Initial value is set to 1 sec  */

tcp_packet *recvpkt;

tcp_packet *sndpkt[WINDOW_SIZE];

//sigmask was not an array
sigset_t sigmask;       
    int turn=0;
int wn_left=0,wn_right=2;
int wn_left_seqno=0;
int endoffile=0;
int portno, len;
int i,j;
int ack_received=0;
int window_send_base=0;

char *hostname;
char buffer[DATA_SIZE];
FILE *fp;   
struct timeval tp;

int count=0;
int ack_to_receive=0;

int timeout=0;
int last_retry=0;
int timer_seqno=0;
int change_timer_flag=0;


void resend_packets(int sig)
{
    //VLOG(DEBUG,"resend_packets");
    //VLOG(DEBUG,"resend_packets of %d",sig);

    if (sig == SIGALRM)
    {
        for(int i=0;i<WINDOW_SIZE-free_space || endoffile==1 ;i++)
            {
                VLOG(DEBUG, "timeout happened :Sending packet %d",sndpkt[i]->hdr.seqno);
                /*  When the WINDOW contains only the endoffile packet  */
                if(sndpkt[i]->hdr.eof==1 && i==0)
                {
                    VLOG(DEBUG,"exit now needed");
                    send_base=0;
                    ack_to_receive=0;
                    sendto(sockfd, sndpkt[i], TCP_HDR_SIZE,  0, (const struct sockaddr *)&serveraddr, serverlen);
                    break;
                }
                else
                {
                    /* When the WINDOW contains data to be sent */
                    if(sndpkt[i]->hdr.data_size!=0)
                    {
                        if(sendto(sockfd, sndpkt[i], sizeof(*sndpkt[i])+sndpkt[i]->hdr.data_size, 0, 
                                ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                        {
                            error("sendto");
                        }    
                    }
                    
                }
               // seqno_to_send_index++;
            }
            VLOG(DEBUG,"TimeoutInterval : %lu",TimeoutInterval);
            temp_timeout = 2 * TimeoutInterval;
            timer.it_interval.tv_sec = temp_timeout / 1000000;    // sets an interval of the timer
            timer.it_interval.tv_usec = temp_timeout % 1000000; 

            /*  Flag is changed so that change_timer_stop is not invoked
                ie. the packets in this WINDOW is not used for
                measurement of TimeoutInterval    */
            change_timer_flag=0;

    }
}


void start_timer()
{
    sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
    setitimer(ITIMER_REAL, &timer, NULL);
    timer.it_value.tv_sec = TimeoutInterval / 1000000;
    timer.it_value.tv_usec = TimeoutInterval % 1000000;
    timer.it_interval.tv_sec = TimeoutInterval / 1000000;    // sets an interval of the timer
    timer.it_interval.tv_usec = TimeoutInterval % 1000000;

    //gettimeofday(&RTT_start,NULL);
}


void stop_timer()
{
    
    sigprocmask(SIG_BLOCK, &sigmask, NULL);

}

void change_timer_start(int sequence)
{
    gettimeofday(&RTT_start,NULL);
    change_timer_flag=1;
    timer_seqno=sequence;
    VLOG(DEBUG,"timer started for %d",timer_seqno);
}

void change_timer_stop()
{
      //  VLOG(DEBUG,"eof : %d\tresponse : %d",recvpkt->hdr.eof,recvpkt->hdr.response);
    VLOG(DEBUG,"timer stopped for %d",timer_seqno);
    gettimeofday(&RTT_stop,NULL);

    SampleRTT = ((RTT_stop.tv_sec-RTT_start.tv_sec)*1000000 + RTT_stop.tv_usec-RTT_start.tv_usec);
    VLOG(DEBUG,"SampleRTT %lu",SampleRTT);
	   //  VLOG(DEBUG," initial EstimatedRTT: %lu\tDevRTT :%lu",EstimatedRTT,DevRTT);

    /*	To calculate RTT of initial packet 	*/
    if(ack_to_receive==0)
    {
        EstimatedRTT = SampleRTT;
        DevRTT=0;
    }
    else
    {
        EstimatedRTT = (1-alpha)*EstimatedRTT + alpha*SampleRTT;
        DevRTT = (1-beta)*DevRTT + beta*(abs(SampleRTT - EstimatedRTT));
      //  VLOG(DEBUG,"EstimatedRTT: %lu\tDevRTT :%lu",EstimatedRTT,DevRTT);
    }


    TimeoutInterval = EstimatedRTT + 4 * DevRTT;
    timer.it_value.tv_sec = TimeoutInterval / 1000000;
    timer.it_value.tv_usec = TimeoutInterval % 1000000;
    timer.it_interval.tv_sec = TimeoutInterval / 1000000;    // sets an interval of the timer
    timer.it_interval.tv_usec = TimeoutInterval % 1000000; 
    VLOG(DEBUG,"timer : %lu--%lu ",timer.it_value.tv_sec,timer.it_value.tv_usec);
    //VLOG(DEBUG,"eof after stoptime : %d",recvpkt->hdr.eof);
    //timer_seqno=next_seqno;
    change_timer_flag=0;
}


/*
 * init_timer: Initialize timeer
 * delay: delay in milli seconds
 * sig_handler: signal handler function for resending unacknoledge packets
 */
void init_timer(int delay, void (*sig_handler)(int)) 
{
    signal(SIGALRM, resend_packets);
    timer.it_interval.tv_sec = 1;    // sets an interval of the timer
    timer.it_interval.tv_usec = 0;  
    timer.it_value.tv_sec = 1;       // sets an initial value
    timer.it_value.tv_usec = 0;

    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);
}

//int i;



int main (int argc, char **argv)
{
    

    /* check command line arguments */
    if (argc != 4) {
        fprintf(stderr,"usage: %s <hostname> <port> <FILE>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    fp = fopen(argv[3], "r");
    if (fp == NULL) {
        error(argv[3]);
    }

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");


    /* initialize server server details */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serverlen = sizeof(serveraddr);

    /* covert host into network byte order */
    if (inet_aton(hostname, &serveraddr.sin_addr) == 0) {
        fprintf(stderr,"ERROR, invalid host %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portno);

    assert(MSS_SIZE - TCP_HDR_SIZE > 0);

    //Stop and wait protocol

    init_timer(RETRY, resend_packets);  
    int read=0;
    int seqno_to_send_index=0;
    free_space=WINDOW_SIZE;
    //sndpkt[0]=make_packet(0);
    //sndpkt[0]->hdr.seqno=-1;

    while (1)
    {
        /*	Fill the free spaces in the WINDOW with contents from FILE endofile is not set,ie.enfoffile has not been sent	*/
        if(free_space>0 && endoffile==0 && read==0)
        {
            /*	Starting from the initial free space to the end of WINDOW_SIZE*/
            for(int i=WINDOW_SIZE-free_space;i<WINDOW_SIZE;i++)
            {
                len = fread(buffer, 1, DATA_SIZE, fp);
                if ( len <= 0)
                {
                    /*	When endoffile is reached, a packet with hdr.eof==1 is set indicating last packet 	*/
                    VLOG(INFO, "End Of File has been reached in %dth position",i);
                    sndpkt[i] = make_packet(0);
                    sndpkt[i]->hdr.eof=1;
                    /*	If there is no packet in WINDOW, set endoffile so that read will no take place again	*/
                    if(i==0)
                    	endoffile=1;
                          
                    read=1;         
                    break;
                }
                
                send_base = next_seqno;
                next_seqno = send_base + len;
                sndpkt[i] = make_packet(len);
                memcpy(sndpkt[i]->data, buffer, len);
                //VLOG(DEBUG,"sndpkt data : %s",sndpkt[i]->data);
                sndpkt[i]->hdr.seqno = send_base;
                //VLOG(DEBUG,"sndpkt seqno : %d\t send_base: %d",sndpkt[i]->hdr.seqno,send_base);

                /* The no.of free spaces is reduced	*/
                free_space--;
                
            
            } 
            read=1;

        }

        /*	Try to send all the packets in the WINDOW that are newly read from file */
     	if(read==1 || endoffile==1)
        {

            //VLOG(DEBUG,"seqno_to_send_index: %d",seqno_to_send_index);
            for(int i=seqno_to_send_index;i<WINDOW_SIZE-free_space || endoffile==1 ;i++)
            {
     			VLOG(DEBUG, "Sending packet %d",sndpkt[i]->hdr.seqno);
	            /*	When the WINDOW contains only the endoffile packet 	*/

	            if(sndpkt[i]->hdr.eof==1 && i==0)
	            {
	                VLOG(DEBUG,"exit now needed");
	                send_base=0;
	                ack_to_receive=0;
	                sendto(sockfd, sndpkt[i], TCP_HDR_SIZE,  0, (const struct sockaddr *)&serveraddr, serverlen);
                    last_retry++;

                	/*	TO send EOF 10 times	*/
                    if(last_retry>10)
                        exit(0);
	                break;
	                    //exit(0);
	            }
	            else
	            {
	            	/* When the WINDOW contains data to be sent	*/
	                if(sndpkt[i]->hdr.data_size!=0)
	                    if(sendto(sockfd, sndpkt[i], sizeof(*sndpkt[i])+sndpkt[i]->hdr.data_size, 0, ( const struct sockaddr *)&serveraddr, serverlen) < 0)
	                        error("sendto");

	                
	            }
                seqno_to_send_index++;
                read=0;

                /*  If there is no present measurement of RTT,
                    the current packet is used   */
                if(change_timer_flag==0)
                    change_timer_start(sndpkt[i]->hdr.seqno);
     		}
            
            
         }   
        while(1)
        {     
	        start_timer();
	        if(recvfrom(sockfd, buffer, MSS_SIZE, 0,
	                    (struct sockaddr *) &serveraddr, (socklen_t *)&serverlen) < 0)
	            error("recvfrom");
	        recvpkt = (tcp_packet *)buffer;

	        VLOG(DEBUG,"seqno: %d\tack_to_receive : %d\tresponse: %d",recvpkt->hdr.seqno,ack_to_receive,recvpkt->hdr.response);

	        /*	If the ACK received is the acknowledgment expected OR ACK of endoffile	*/
	        if(recvpkt->hdr.seqno==ack_to_receive || recvpkt->hdr.eof==1)
	        {
	             //  VLOG(DEBUG,"ack_to_receive before IF first : %d",ack_to_receive);
	            if(recvpkt->hdr.eof==1)
	                exit(0);
	            stop_timer();
	        	ack_received=recvpkt->hdr.seqno;
	            for(int i=0;i<(WINDOW_SIZE-1-free_space);i++)
	            {
	                *sndpkt[i]=*sndpkt[i+1];
	                for(j=0;j<DATA_SIZE;j++)
	                    sndpkt[i]->data[j]=sndpkt[i+1]->data[j];
	            }

	            /*	If no.of free spaces is not larger that WINDOW	*/
	            if(free_space<WINDOW_SIZE)
	            	free_space++;

	            /*	set the ACK to receive next appropriatly	*/
	            if(recvpkt->hdr.eof==0)
		            ack_to_receive=ack_to_receive+DATA_SIZE; 
		        else
		        	ack_to_receive=0;

	            seqno_to_send_index--;
	            read=0;

	            /*  When the ACK received is the ack of the packet which is being used for RTT measurement,
	                the new TimeoutInterval is calculated   */
	            if(recvpkt->hdr.seqno==timer_seqno && change_timer_flag==1)
	                change_timer_stop();

	          //  VLOG(DEBUG,"ack_to_receive after IF first : %d",ack_to_receive);

	     

	        }
	        /*	If the ACK received is ACK of earlier acknowledged packet, then the packet NEXT to that is sent	*/
	        else if(recvpkt->hdr.seqno<ack_to_receive)
	        {
	        	VLOG(DEBUG,"sending seqno : %d",sndpkt[0]->hdr.seqno);
	        	if(sendto(sockfd, sndpkt[0], sizeof(*sndpkt[0])+sndpkt[0]->hdr.data_size, 0,( const struct sockaddr *)&serveraddr, serverlen) < 0)
	              			 error("sendto");

	        }
	        /*	If the ACK received is that of a packet whoso earlier packets are not acknowledged,
	        	all the previous packets in the sender WINDOW are ackowledged, 
	        	because the ACK of the received pacekt will only be sent after all the previous packets are acknowledged	*/
	        else if(recvpkt->hdr.seqno>ack_to_receive && endoffile==0)
	        {
	            stop_timer();
	        	while(ack_to_receive<=recvpkt->hdr.seqno)
	        	{
	        		for(int i=0;i<WINDOW_SIZE-free_space;i++)
	            	{
	            		*sndpkt[i]=*sndpkt[i+1];
		                for(j=0;j<DATA_SIZE;j++)
		                    sndpkt[i]->data[j]=sndpkt[i+1]->data[j];
	                   
	            	}
	                 /* If no.of free spaces is not larger that WINDOW  */
	                    if(free_space<WINDOW_SIZE)
	                        free_space++;
	                    
	                    if(seqno_to_send_index>0)
	                        seqno_to_send_index--;

	            	ack_to_receive=ack_to_receive+DATA_SIZE;
	            	
	        	}
	            ack_received=recvpkt->hdr.seqno;    //new
	        	read=0;
	                  //      VLOG(DEBUG,"ack_to_receive after IF second : %d",ack_to_receive);
	            if(recvpkt->hdr.seqno >= timer_seqno && change_timer_flag==1)
	                change_timer_stop();

	        }
	        /*	When the ACK received is that of the final packet in the WINDOW	*/
	        if(ack_received==send_base)
	            break;
        }



    }

    return 0;

}




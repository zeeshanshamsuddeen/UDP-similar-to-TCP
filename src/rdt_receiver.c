/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<stdio.h>
#include "packet.h"
#include "common.h"
#include <sys/time.h>



/*
 * You ar required to change the implementation to support
 * window size greater than one.
 * In the currenlt implemenetation window size is one, hence we have
 * onlyt one send and receive packet
 */
tcp_packet *recvpkt;
tcp_packet *b_packet[WINDOW_SIZE];
tcp_packet *sndpkt;

int main(int argc, char **argv) {
    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    int optval; /* flag value for setsockopt */
 	int expt_seq=0;
    int b_index=0;
    int file_close=0;
    clientlen = sizeof(clientaddr);
    sndpkt=make_packet(0);
    gettimeofday(&tp1, NULL);
    long start_time;
    int start_time_flag=0;
    int response=0;
    int i,j;
    int last_retry=0;
    int temp=0;
    //int free_space=10;

    FILE *fp;
    char buffer[MSS_SIZE];
    struct timeval tp1;
    struct timeval tp2;

    /* 
     * check command line arguments 
     */
    if (argc != 3) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    fp  = fopen(argv[2], "w");
    if (fp == NULL) {
        error(argv[2]);
    }

    /* 
     * socket: create the parent socket 
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets 
     * us rerun the server immediately after we kill it; 
     * otherwise we have to wait about 20 secs. 
     * Eliminates "ERROR on binding: Address already in use" error. 
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
            (const void *)&optval , sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /* 
     * bind: associate the parent socket with a port 
     */
    if (bind(sockfd, (struct sockaddr *) &serveraddr, 
                sizeof(serveraddr)) < 0) 
        error("ERROR on binding");

 
    while (1) {
        /*
         * recvfrom: receive a UDP datagram from a client
         */
        //VLOG(DEBUG, "waiting from server \n");
        

        /*  Check whether the packet received is the expected packet */
        while(1)
        {
            //VLOG(DEBUG,"going to recieve");
            if (recvfrom(sockfd, buffer, MSS_SIZE, 0,
                    (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen) < 0) 
            {
                error("ERROR in recvfrom");
            }
            recvpkt = (tcp_packet *) buffer;
            //gettimeofday(&tp, NULL);
            //VLOG(DEBUG, "%lu.%lu cont: %d",tp.tv_sec,tp.tv_usec*1000,recvpkt->hdr.temp);
            //VLOG(DEBUG,"temp pkt seqno; %d expt_seq:%d",recvpkt->hdr.seqno,expt_seq);   

            /*	When the packet received is EOF and destination file has not been closed	*/
            if ( recvpkt->hdr.eof == 1 && file_close==0) 
            {
                //VLOG(DEBUG, "End Of File has been reached");
                fclose(fp);
                file_close=1;
                break;
            }
            /*	When the packet received is EOF and destination file is closed	*/
            if(recvpkt->hdr.eof==1 && file_close==1)
            {
            	last_retry++;
            	/*	TO send ACK for EOF 10 times	*/
                if(last_retry>10)
                     exit(0);            	//VLOG(DEBUG,"if1");
            	break;
            }
           // VLOG(DEBUG,"temp packet received of seqno %d",recvpkt->hdr.seqno);
            /*	When the packet received is the expected packet	*/
            if(recvpkt->hdr.seqno==expt_seq)
            {
            	break;
            }
            /*	To place the received packet into the buffer in the case of OUT OF ORDER packets	*/
            else if(recvpkt->hdr.seqno>expt_seq && b_index<WINDOW_SIZE)
            {
            	//VLOG(DEBUG,"ifelse")
            	b_packet[b_index]=make_packet(recvpkt->hdr.data_size);
				*b_packet[b_index]=*recvpkt;

            	//VLOG(DEBUG,"seq:%d\t data_size:%d",b_packet[b_index]->hdr.seqno,b_packet[b_index]->hdr.data_size);
                for(j=0;j<DATA_SIZE;j++)
                {

                    b_packet[b_index]->data[j]=recvpkt->data[j];
                	//VLOG(DEBUG,"j:%d\tbpacketdata:%c\trecvdata:%c",j,b_packet[b_index]->data[j],recvpkt->data[j]);
                }
                b_index++;	
            }
            sndpkt=make_packet(0);
            sndpkt->hdr.response=response;;
            response++;	
            sndpkt->hdr.seqno=expt_seq-DATA_SIZE;
            //VLOG(DEBUG,"sending ack of seqno: %d\tresponse : %d ",sndpkt->hdr.seqno,sndpkt->hdr.response);

            if (sendto( sockfd, sndpkt, sizeof(*sndpkt), 0, (const struct sockaddr *) &clientaddr, clientlen) < 0) 
          		VLOG(DEBUG,"ERROR in sendto");
            	
           
         }


        //VLOG(DEBUG, "pkt recieved->             seqno: %d",recvpkt->hdr.seqno);

        /*	When the received packet is not EOF	*/ 
        if(recvpkt->hdr.eof==0)
        {
            expt_seq=expt_seq+DATA_SIZE;
            fseek(fp, recvpkt->hdr.seqno, SEEK_SET);
            fwrite(recvpkt->data, 1, recvpkt->hdr.data_size, fp);
        }
        else
            expt_seq=0;
        
        /*	To transfer packets from BUFFER to file if needed	*/
        temp=b_index;
        for(i=0;i<b_index;i++)
        {
        	if(b_packet[i]->hdr.seqno==expt_seq)
        	{
        		fseek(fp, b_packet[i]->hdr.seqno, SEEK_SET);
           		fwrite(b_packet[i]->data, 1, b_packet[i]->hdr.data_size, fp);
           		expt_seq=expt_seq+DATA_SIZE;
           		temp--;
        	}
        }
        b_index=temp;
        //VLOG(DEBUG,"check finish")

        /* 
         * sendto: ACK back to the client 
         */
        gettimeofday(&tp2, NULL);
        //VLOG(DEBUG, "%lu.%lu,%d",tp2.tv_sec,tp2.tv_usec,recvpkt->hdr.data_size);
        long elapsed = (tp2.tv_sec-tp1.tv_sec)*1000000 + tp2.tv_usec-tp1.tv_usec;

        if(start_time_flag==0)
        	{
        		start_time=elapsed;
        		start_time_flag=1;
        	}	

        VLOG(DEBUG,"%lu,%d",elapsed-start_time,recvpkt->hdr.data_size);
        
        //if(recvpkt->hdr.data_size==0)
        //	exit(0);
        sndpkt = make_packet(0);
        sndpkt->hdr.ackno = expt_seq;
        sndpkt->hdr.seqno=recvpkt->hdr.seqno;
        sndpkt->hdr.ctr_flags = ACK;
        sndpkt->hdr.eof=recvpkt->hdr.eof;
        sndpkt->hdr.response=response;
        response++;
        //int length=sizeof(*sndpkt);
        //VLOG(DEBUG,"size %d\tTCP_HEADER_SIZE %lu",length,TCP_HDR_SIZE);
        //resp++;
        //VLOG(DEBUG,"seqno: %d\tresponse: %d ",sndpkt->hdr.seqno,sndpkt->hdr.response);

        
        if (sendto(sockfd, sndpkt, sizeof(*sndpkt), 0, 
                (const struct sockaddr *) &clientaddr, clientlen) < 0) {
            error("ERROR in sendto");
        }
    }

    return 0;
}

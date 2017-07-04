enum packet_type {
    DATA,
    ACK,
};

typedef struct {
    int seqno;
    int ackno;
    int ctr_flags;
    
    int eof;
    //int turn;
    int response;
    int data_size; /*   data_size should always be at the last   */
}tcp_header;

#define MSS_SIZE    1500
#define TCP_HDR_SIZE    sizeof(tcp_header)
//DATA_SIZE should be MSS_SIZE-TCP_HDR_SIZE
#define DATA_SIZE   4
#define WINDOW_SIZE 10
#define alpha 0.125
#define beta 0.25

typedef struct {
    tcp_header  hdr;
    char    data[0];
}tcp_packet;

tcp_packet* make_packet(int seq);

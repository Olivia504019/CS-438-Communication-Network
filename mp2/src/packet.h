#ifndef PACKET_H_
#define PACKET_H_

#include <iostream>
#include <string.h>

#define MAX_DATA_SIZE 1024

/*
    Protocol Design
    Assumption: no packet loss.
    packet: header [3 bit flag (SYN / ACK / FIN), 
                10 bit sequence number (seq),
                10 bit acknowledgment number (ack)]
            + segment length 10 bit
            + payload = total packet length <= 1024
    States:
    1. Start data transmission
        sender -> receiver (seq, ack) = (0, 200)    (ACK, len = 200)
        sender <- receiver (seq, ack) = (200, 400)  (ACK, len = 200)
        sender -> receiver (seq, ack) = (400, 600)  (ACK, len = 200)
        sender <- receiver (seq, ack) = (600, 800)  (ACK, len = 200)
        ...
    2. Close connection
        sender -> receiver (seq, ack) = (1000, 1200) (ACK, FIN, len = 200)
        sender <- receiver (seq, ack) = (1200, 1000) (ACK, len = 0)
*/

class Packet{
public:
    bool SYN, ACK, FIN;
    int seq_num, ack_num, data_len;
    char data[MAX_DATA_SIZE];
    Packet(){
        memset(data, 0, sizeof(MAX_DATA_SIZE));
    };
    Packet(bool, bool, bool, int, int, int, char[]);
    bool operator<(const Packet &_packet)const{
        return seq_num > _packet.seq_num;
    }
};

#endif  // PACKET_H_
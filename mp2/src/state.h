#ifndef STATE_H_
#define STATE_H_

#include "socket.h"
#include <queue>
#include <stdio.h>
#include <arpa/inet.h>

#define MAX_DATA_SIZE 1024

enum class Event{
    timeout,
    new_ACK,
    duplicate_ACK
};

enum class CongestionControlState{
    SlowStart,
    CongestionAvoidance,
    FastRecovery
};

class CongestionControlParam{
public:
    int congestion_window, congestion_window_accumulate;
    int slow_start_threshold, duplicate_ACK; // CW, SST
};

class CongestionControlStateMachine{
private:
    Socket socket;
    struct sockaddr_in address;
    FILE *fp;
    int seq_num, last_ack_seq_num;
    char buf[MAX_DATA_SIZE];
    CongestionControlState newACK();
    CongestionControlState timeout();
    CongestionControlState dupACK();
    void _sendPacketFromFile(void);
    void _resendCongestionWindow(void);
    void _sendPacketAsAllowedByCongestionWindow(void);
public:
    std::queue<Packet> read_file_buffer_queue;
    CongestionControlParam param;
    CongestionControlState state;
    unsigned long long int bytes_to_transfer;
    CongestionControlStateMachine(Socket&, struct sockaddr_in, FILE *, unsigned long long int);
    std::queue<Packet> wait_ack_queue;
    void processEvent(Event);
};

#endif  // STATE_H_
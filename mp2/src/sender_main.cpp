#include "socket.h"
#include "packet.h"
#include "state.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <utility>

void reliablyTransfer(char* host_name, unsigned short int host_udp_port, char* file_name, unsigned long long int bytes_to_transfer) {
    struct sockaddr_in address;
    Socket socket;
    Packet packet;
    int last_ack_seq_num = -1;
    std::pair<Packet, bool> p;
    FILE *fp;

    address = Socket::getSocketAddressFromHostname(host_name, host_udp_port);

    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        packet = Packet(false, false, true, 0, 1, 0, "");
        while (true){
            socket.sendData(address, packet);
            p = socket.receiveData(address);
            packet = p.first;
            if (!p.second && packet.FIN) break;
        }
        exit(1);
    }

    CongestionControlStateMachine state_machine(socket, address, fp, bytes_to_transfer);

    while(true) {
        //std::cout << "state_machine.wait_ack_queue size = " << state_machine.wait_ack_queue.size() << "\n";
        if (state_machine.wait_ack_queue.empty() && state_machine.bytes_to_transfer == 0 && state_machine.read_file_buffer_queue.empty())
            break;
        p = socket.receiveData(address);
        packet = p.first;
        //std::cout << (state_machine.state == CongestionControlState::SlowStart?"slowstart":(state_machine.state == CongestionControlState::CongestionAvoidance?"conjestion avoidance":"fast recovery")) << ", received packet = " << packet.seq_num << ", waiting for: " << state_machine.wait_ack_queue.front().ack_num << "\n";
        if (p.second){
            //std::cout << "timeout" << "\n";
            state_machine.processEvent(Event::timeout);
        }
        else if (packet.seq_num >= state_machine.wait_ack_queue.front().ack_num){
            //std::cout << "new ack" << "\n";
            while (!state_machine.wait_ack_queue.empty() && packet.seq_num >= state_machine.wait_ack_queue.front().ack_num){
                last_ack_seq_num = packet.seq_num;
                state_machine.processEvent(Event::new_ACK);
            }
        }
        else if (packet.seq_num == last_ack_seq_num){
            //std::cout << "dup ack" << "\n";
            state_machine.processEvent(Event::duplicate_ACK);
        }
    }
    //std::cout << "FIN\n";
    fclose(fp);
    packet = Packet(false, false, true, 0, 1, 0, "");
    while (true){
        socket.sendData(address, packet);
        p = socket.receiveData(address);
        packet = p.first;
        if (!p.second && packet.FIN) break;
    }
    return;
}

int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return 0;
}



#include "socket.h"
#include "packet.h"
#include <utility>
#include <stdio.h>
#include <arpa/inet.h>
#include <queue>
#include <string.h>

#define MAX_DATA_SIZE 1024
#define MAX_BUFFER_SIZE 100000

void reliablyReceive(unsigned short int port, char* destination_file) {
    struct sockaddr_in address;
    Socket socket;
    Packet packet;
    int seq_num = 0, data_len = 0;
    std::priority_queue<Packet> pq;
    FILE *fp;
    std::pair<Packet, bool> p;

    socket.bind(port);
    fp = fopen(destination_file, "wb");
    if (fp == NULL) {
        printf("Could not open file.");
        exit(1);
    }
    // seq_num: 目前預計下一個要收到的packet seq_num
    while (true) {
        p = socket.receiveData(address);
        if (p.second){
            continue;
        }
        packet = p.first;
        //std::cout << "收到: " << packet.seq_num << ", 目前需要" << seq_num << "\n";
        // 要結束的時候
        if (packet.FIN) {
            packet = Packet(false, true, true, 0, 1, 0, "");
            socket.sendData(address, packet);
            break;
        }
        //正常情況下
        else {
            // 收到之前收過的，不用回傳任何東西
            if (packet.seq_num < seq_num) {
                ;
            }
            // 收到目前剛好需要的
            else if (packet.seq_num == seq_num) {
                seq_num += packet.data_len;
                data_len = packet.data_len;
                fwrite(packet.data, sizeof(char), packet.data_len, fp);
                fflush(fp);
                // 檢查pq裡有沒有剛好是後面可以拿來用的
                while (!pq.empty() && pq.top().seq_num <= seq_num) {
                    packet = pq.top();
                    if (packet.seq_num == seq_num) {
                        seq_num += packet.data_len;
                        data_len = packet.data_len;
                        fwrite(packet.data, sizeof(char), packet.data_len, fp);
                        fflush(fp);
                    }
                    pq.pop();
                }
            }
            // 收到比需要的更大的，先丟進pq裡
            else {
                if (pq.size() <= MAX_BUFFER_SIZE)
                    pq.push(packet);
            }
            packet = Packet(false, true, false, seq_num, seq_num + data_len, 0, "");
            socket.sendData(address, packet);
        }
    }
    fclose(fp);
    return;
}

int main(int argc, char** argv) {
    unsigned short int port;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    port = (unsigned short int) atoi(argv[1]);

    reliablyReceive(port, argv[2]);
    return 0;
}


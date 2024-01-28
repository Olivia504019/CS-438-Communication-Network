#include "socket.h"
#include "state.h"
#include <math.h>
#include <queue>
#include <stdio.h>
#include <arpa/inet.h>

#define MAX_DATA_SIZE 1024


CongestionControlStateMachine::CongestionControlStateMachine(Socket &_socket, struct sockaddr_in _address, FILE *_fp, unsigned long long int _bytes_to_transfer): socket(_socket), address(_address), fp(_fp), bytes_to_transfer(_bytes_to_transfer){
    state = CongestionControlState::SlowStart;
    last_ack_seq_num = -1;
    seq_num = 0;
    param.congestion_window = 100;
    param.congestion_window_accumulate = 0;
    param.slow_start_threshold = 500;
    param.duplicate_ACK = 0;
    for (int i = 0;i < param.congestion_window; i++)
        _sendPacketFromFile();
}

void CongestionControlStateMachine::processEvent(Event event){
    switch (event){
        case Event::timeout:
            state = timeout();
            return;
        case Event::duplicate_ACK:
            state = dupACK();
            return;
        case Event::new_ACK:
            state = newACK();
            return;
    }
    return;
}

CongestionControlState CongestionControlStateMachine::timeout(){
    param.slow_start_threshold = param.congestion_window / 2;
    param.congestion_window_accumulate = 0;
    param.congestion_window = 1;
    param.duplicate_ACK = 0;
    // resend cw_base packet ??
    _resendCongestionWindow();
    return CongestionControlState::SlowStart;
}

CongestionControlState CongestionControlStateMachine::dupACK(){
    param.congestion_window_accumulate = 0;
    switch (state){
        case CongestionControlState::SlowStart:
            param.duplicate_ACK += 1;
            if (param.duplicate_ACK == 3){
                param.slow_start_threshold = param.congestion_window / 2;
                param.congestion_window = param.slow_start_threshold + 3;
                // resend cw_base -> clear wait ack buffer queue
                _resendCongestionWindow();
                return CongestionControlState::FastRecovery;
            }
            return CongestionControlState::SlowStart;

        case CongestionControlState::CongestionAvoidance:
            param.duplicate_ACK += 1;
            if (param.duplicate_ACK == 3){
                param.slow_start_threshold = param.congestion_window / 2;
                param.congestion_window = param.slow_start_threshold + 3;
                _resendCongestionWindow();
                return CongestionControlState::FastRecovery;
            }
            return CongestionControlState::CongestionAvoidance;

        case CongestionControlState::FastRecovery:
            param.congestion_window += 1;
            // transmit packet as allowed
            _sendPacketAsAllowedByCongestionWindow();
            return CongestionControlState::FastRecovery;
    }
}

CongestionControlState CongestionControlStateMachine::newACK(){
    param.duplicate_ACK = 0;
    last_ack_seq_num = wait_ack_queue.front().seq_num;
    wait_ack_queue.pop();
    switch (state){
        case CongestionControlState::SlowStart:
            param.congestion_window ++;
            _sendPacketAsAllowedByCongestionWindow();
            if (param.congestion_window >= param.slow_start_threshold)
                return CongestionControlState::CongestionAvoidance;
            return CongestionControlState::SlowStart;

        case CongestionControlState::CongestionAvoidance:
            param.congestion_window_accumulate ++;
            if (param.congestion_window_accumulate == param.congestion_window){
                param.congestion_window ++;
                param.congestion_window_accumulate = 0;
            }
            _sendPacketAsAllowedByCongestionWindow();
            return CongestionControlState::CongestionAvoidance;

        case CongestionControlState::FastRecovery:
            param.congestion_window = param.slow_start_threshold;
            // transmit packet as allowed
            _sendPacketAsAllowedByCongestionWindow();
            return CongestionControlState::CongestionAvoidance;
    }
}

void CongestionControlStateMachine::_resendCongestionWindow(void){
    while (!read_file_buffer_queue.empty()){
        wait_ack_queue.push(read_file_buffer_queue.front());
        read_file_buffer_queue.pop();
    }
    while (!wait_ack_queue.empty()) {
        read_file_buffer_queue.push(wait_ack_queue.front());
        wait_ack_queue.pop();
    }
    _sendPacketAsAllowedByCongestionWindow();
    return;
}

void CongestionControlStateMachine::_sendPacketAsAllowedByCongestionWindow(void){
    // std::cout << "cwnd = " << param.congestion_window << "\n";
    if (bytes_to_transfer == 0 && read_file_buffer_queue.empty())
        return;
    for (int i=0;wait_ack_queue.size() < param.congestion_window; i++)            
        _sendPacketFromFile();
    return;
}

void CongestionControlStateMachine::_sendPacketFromFile(void){
    int len;
    char buf[MAX_DATA_SIZE];
    Packet packet;
    memset(buf, 0, sizeof(buf));
    //std::cout << "bytes left = " << bytes_to_transfer << ", buffer queue size = " << read_file_buffer_queue.size() << "\n";
    if (bytes_to_transfer > 0 && read_file_buffer_queue.empty()){
        len = fread(buf, sizeof(char), std::min(bytes_to_transfer, (unsigned long long int)MAX_DATA_SIZE), fp);
        if (len == 0){
            //std::cout << "read failure\n";
            bytes_to_transfer = 0;
            return;
        }
        bytes_to_transfer -= len;
        packet = Packet(false, true, false, seq_num, seq_num + len, len, buf);
        seq_num += len;
    }
    else if (!read_file_buffer_queue.empty()){
        packet = read_file_buffer_queue.front();
        read_file_buffer_queue.pop();
        seq_num = packet.seq_num + packet.data_len;
    }
    else return;
    if (packet.seq_num > last_ack_seq_num){
        socket.sendData(address, packet);
        wait_ack_queue.push(packet);
    }
    return;
}
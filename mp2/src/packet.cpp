#include "packet.h"

Packet::Packet(bool _SYN, bool _ACK, bool _FIN, int _seq_num, int _ack_num, int _data_len, char _data[1024]): SYN(_SYN), ACK(_ACK), FIN(_FIN), seq_num(_seq_num), ack_num(_ack_num), data_len(_data_len){
    memcpy(data, _data, _data_len);
}
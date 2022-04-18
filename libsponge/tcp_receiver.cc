#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    bool receivedSYN = 1;
    if (_in_listen) { // if state is LISTEN an syn is true, set seq 
        if (!header.syn) {
            _reassembler.stream_out().error();
            return;
        }
        _in_listen = false;
        _isn = header.seqno;
        receivedSYN = 0;
    }

    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    uint64_t stream_index = unwrap(header.seqno, _isn, checkpoint) - receivedSYN;

    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_in_listen) {
        return nullopt;
    }
    // here state is SYN_RECE or FIN_RECE
    // if SYN_RECE : ack_no is SYN + bytes has be writen;
    // if FIN_RECE : ack_no is SYN + bytes has be writen + FIN;
    uint64_t abs_ack = _reassembler.stream_out().bytes_written() + 1 + _reassembler.stream_out().input_ended();
    return wrap(abs_ack, _isn); 
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().remaining_capacity(); 
}

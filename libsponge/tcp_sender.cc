#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <algorithm>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity), _retransmission_timeout(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { 
    // return _outgoing_segment.size();
    return _bytes_in_flight;
}
inline void TCPSender::send_syn_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    // set syn flag and seq no
    segment.header().syn = true;
    // send it, hold it until receive ack
    _segments_out.emplace(segment);
    _segments_outstanding.emplace(std::make_pair(0, segment));
    // update state;
    _state = SYN_SENT;
    _next_seqno = 1;
    _bytes_in_flight = 1;
    // start clock
    _alarm_run = true;
}

void TCPSender::fill_window() {
    if (_state == CLOSED) {
        send_syn_segment();
        return;
    }
    uint64_t curr_window_size = _window_size ? _window_size : 1;
    while (_state == SYN_ACKED && curr_window_size > bytes_in_flight()) {
        TCPSegment segment;
        segment.header().seqno = next_seqno();
        size_t readsz = curr_window_size - bytes_in_flight();

        if(_stream.input_ended() && readsz > _stream.buffer_size()) {
            segment.header().fin = true;
            _stream.end_input();
            _state = FIN_SENT;
            readsz -= 1;
        } else if (_stream.buffer_empty()) {
            return;
        }
        readsz = min(readsz, TCPConfig::MAX_PAYLOAD_SIZE);
        string payload = _stream.read(readsz);
        // string_view str = payload;
        
        segment.payload() = Buffer(std::move(payload));
        _segments_out.push(segment);

        _segments_outstanding.push(std::make_pair(_next_seqno, segment));
        _bytes_in_flight += segment.payload().str().size();
        _next_seqno += segment.payload().str().size();
 
        if (_state == FIN_SENT) {
            _bytes_in_flight += 1;
            _next_seqno += 1;
        }
    }
        
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size); 
    _ack_no = unwrap(ackno, _isn, _stream.bytes_read());
    _window_size = window_size;
    _retransmission_timeout = _initial_retransmission_timeout;

    if (_ack_no > _next_seqno) {
        return;
    }

    if (_state == SYN_SENT || _state == SYN_ACKED) {   
        while (!_segments_outstanding.empty()) {
           size_t seqno = _segments_outstanding.front().first;
           size_t seg_size = _segments_outstanding.front().second.payload().size();
           if (_segments_outstanding.front().second.header().syn) {
               seg_size += 1;
           }
           if (seqno + seg_size > _ack_no) {
               break;
           }
           // cout << "dequeue retrans segment with size:" << seg_size << endl;
           _bytes_in_flight -= (seg_size);
           _segments_outstanding.pop();
           _remaining_time = 0;
           _retransmission_count = 0;

        }
        if (_bytes_in_flight < _next_seqno)
            _state = SYN_ACKED; 
        fill_window();
    }
    if (_state == FIN_SENT) {
        while (!_segments_outstanding.empty()) {
           size_t seqno = _segments_outstanding.front().first;
           size_t seg_size = _segments_outstanding.front().second.payload().size();
           if (_segments_outstanding.front().second.header().fin) {
               seg_size += 1;
           }
           if (seqno + seg_size > _ack_no) {
               break;
           }
           // cout << "dequeue retrans segment with size:" << seg_size << endl;
           _bytes_in_flight -= (seg_size);
           _segments_outstanding.pop();
           _remaining_time = 0;
        }
        if (_bytes_in_flight == 0) {
            _state = FINS_ACKED;
        }
 
        // cloase connection
    }
    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    if (_alarm_run == false) {
        return;
    }
    _remaining_time += ms_since_last_tick;
    // printf("remaining time is:%u, init timeout is%u\n", _remaining_time, _initial_retransmission_timeout);
    if (!_segments_outstanding.empty() && _remaining_time >= _retransmission_timeout) {
        // consecutive_retransmissions();
        if (_state == SYN_SENT || _window_size != 0) {
            _retransmission_timeout *= 2;
        }
        auto [_, retrans_seg] = _segments_outstanding.front();
        _segments_out.push(retrans_seg);
        _retransmission_count++;
        _remaining_time = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_count; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    _segments_out.push(segment);
}

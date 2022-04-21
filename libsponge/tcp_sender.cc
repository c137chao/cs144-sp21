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
    // set tcpsegment header
    segment.header().seqno = next_seqno();
    segment.header().syn = true;

    // send it, and hold it until receive ack
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
        // if doesn't set connection, send syn segment
        return send_syn_segment(); 
    }
    // if _window_size is 0, try send a byte payload data
    uint64_t curr_window_size = _window_size;
    if (curr_window_size == 0) {
        curr_window_size = 1;
    }
    // send data entil window is full or _stream.eof() or buffer empty
    while (_state == SYN_ACKED && curr_window_size > bytes_in_flight()) {
        TCPSegment segment;
        segment.header().seqno = next_seqno();

        size_t readsz = curr_window_size - bytes_in_flight();
    
        // if write ended, check should we set fin flag
        if(_stream.input_ended() && readsz > _stream.buffer_size()) {
            segment.header().fin = true;
            _state = FIN_SENT;
            readsz--;
        } else if (_stream.buffer_empty()) {
            return;
        }
 
        readsz = min(readsz, TCPConfig::MAX_PAYLOAD_SIZE);
   
        string payload = _stream.read(readsz);
        segment.payload() = Buffer(std::move(payload));

        _segments_out.emplace(segment);
        _segments_outstanding.emplace(_next_seqno, segment);

        _bytes_in_flight += segment.length_in_sequence_space();
        _next_seqno += segment.length_in_sequence_space();
    }   
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size); 
    auto received_ackno = unwrap(ackno, _isn, _stream.bytes_read());

    if (received_ackno > _next_seqno) {
        return;  // an invalid ackno
    }
    // update window size, reset retransmission timeout
    _window_size = window_size;
    _retransmission_timeout = _initial_retransmission_timeout;

    // pop all segment that received by receiver
    while (!_segments_outstanding.empty()) {
        size_t seqno = _segments_outstanding.front().first;
        size_t segment_size = _segments_outstanding.front().second.length_in_sequence_space();
        
        if (seqno + segment_size > received_ackno) {
            break;
        }
        _bytes_in_flight -= (segment_size);
        _segments_outstanding.pop();
        _remaining_time = 0;
        _retransmission_count = 0;
    }

    if (_state == SYN_SENT && _bytes_in_flight < _next_seqno) {
        _state = SYN_ACKED;
    }
    if (_state == SYN_ACKED) {   
        fill_window();
    }
    // } else if (_state == FIN_SENT) {
    //    _state = FINS_ACKED;
    // }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    if (_alarm_run == false) {
        return;
    }
    _remaining_time += ms_since_last_tick;

    if (!_segments_outstanding.empty() && _remaining_time >= _retransmission_timeout) {
        if (_window_size != 0) { 
            // if netword congestion
            _retransmission_timeout *= 2;
        }
        _segments_out.emplace(_segments_outstanding.front().second);
        _retransmission_count++;
        _remaining_time = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_count; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    _segments_out.emplace(segment);
}

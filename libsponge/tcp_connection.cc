#include "tcp_connection.hh"

#include <iostream>
#include <limits>
#include <algorithm>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity(); 
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight(); 
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes(); 
}

size_t TCPConnection::time_since_last_segment_received() const { 
    return _time_since_last_segment_received; 
}

void TCPConnection::kill_connection() {
    // {
    //     std::queue<TCPSegment> empty_out;
    //     std::swap(empty_out, _segments_out);
    // }
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _actived = false;
    _linger_after_streams_finish = false;
}

void TCPConnection::unclean_shutdown() { 
    send_rst_segment();
    kill_connection();
}

/*
Prereq #1 _recever.stream_out().input_ended();
Prereq #2 _sender.stream_in().eof() && _sender.segments_out().empty();
Prereq #3 _sender.bytes_in_flight() == 0;
Prereq #4
    send fin and recv ack
    recv fin and send ack
*/
void TCPConnection::clean_shutdown() {
    if (inbound_stream().input_ended()) { // receiver has been ended
        // cerr << ">> : inbound_stream has reach eof\n";
        if (!_sender.stream_in().input_ended()) {
            _linger_after_streams_finish = false;
        } else if (_sender.bytes_in_flight() == 0) {
            if (not _linger_after_streams_finish or _time_since_last_segment_received >= 10 * _cfg.TIMEOUT_DFLT) {
                _actived = false;
            }
        }
    }
}

void TCPConnection::fetch_segment() {
    while(!_sender.segments_out().empty()) {
        TCPSegment segment = _sender.segments_out().front();
        _sender.segments_out().pop();

        if (_receiver.ackno().has_value()) {
            segment.header().ack = true;
            segment.header().ackno = _receiver.ackno().value();
            segment.header().win = min(static_cast<size_t>(std::numeric_limits<uint16_t>::max()), _receiver.window_size());
        }

       _segments_out.push(segment);
    }
}

void TCPConnection::send_segment() {
    _sender.fill_window();

    fetch_segment();
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
    // DUMMY_CODE(seg); 
    if (!active()) {
        return;
    }

    _time_since_last_segment_received = 0;

    if (seg.header().ack == true) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    if (seg.header().rst) {
        kill_connection();
        return;
    }

    if (TCPState::state_summary(_receiver) != TCPReceiverStateSummary::LISTEN  or TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_ACKED
        or seg.header().syn) {
        _sender.fill_window();
    }
    
    if (seg.length_in_sequence_space() != 0) {
        // syn, fin, payload
        _receiver.segment_received(seg);  
        if (_sender.segments_out().empty()){
            _sender.send_empty_segment();
        }
    } else {
        // pure ack or ask connection is still alive?
        if (_receiver.ackno().has_value() and seg.header().seqno == _receiver.ackno().value() - 1) {
            _sender.send_empty_segment();      
        }

    }
    fetch_segment();
    clean_shutdown();
}

bool TCPConnection::active() const { return _actived; }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    size_t write_size = _sender.stream_in().write(data);
    send_segment();
    return write_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
     if (active() == false) {
        return;
    }
    _time_since_last_segment_received += ms_since_last_tick;

    _sender.tick(ms_since_last_tick);

    auto retrans_count = _sender.consecutive_retransmissions();

    if (retrans_count > TCPConfig::MAX_RETX_ATTEMPTS) {
        while (!_sender.segments_out().empty()) {
            _sender.segments_out().pop();
        }
        while (!_segments_out.empty()) {
            _segments_out.pop();
        }
        send_rst_segment();
        kill_connection();
    } else {
        fetch_segment();
        clean_shutdown();
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    send_segment();
}

void TCPConnection::connect() {
    send_segment();
}

void TCPConnection::send_rst_segment() {
    _sender.send_empty_segment();
    TCPSegment segment = _sender.segments_out().front();
    _sender.segments_out().pop();
    segment.header().rst = true;

    if (_receiver.ackno().has_value()) {
        segment.header().ack = true;
        segment.header().ackno = _receiver.ackno().value();
        segment.header().win = _receiver.window_size();
    }
 
    _segments_out.push(segment);
    _actived = false;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            send_rst_segment();
            kill_connection();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

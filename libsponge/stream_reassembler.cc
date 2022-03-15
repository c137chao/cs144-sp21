#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) 
    :_reassemble_buffer(Buffer()), _expected(0),  _eof_index(-1), _unassembled_bytes(0), _output(capacity), _capacity(capacity)
{}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    if(eof){
        _eof_index = data.size() + index;
    }
    if(index + data.size() < _expected ) {
        return;  // ignore a repeated segment
    }
    if(index < _expected) {
        _reassemble_buffer.insert(_unassembled_bytes, _expected, data.substr(_expected - index));
    } else {
        _reassemble_buffer.insert(_unassembled_bytes, index, data);
    }
    _reassemble_buffer.send  (_output, _expected, _unassembled_bytes);
    if(_expected == _eof_index) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

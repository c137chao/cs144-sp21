#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) 
    : slide_window(my_queue(capacity)), _capacity(capacity), _bytes_written(0),
     _bytes_read(0), _end_input(false), _end_output(false), _error(false)
{ /* DUMMY_CODE(capacity); */ }

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data); 
    if(_end_input) {
        return 0;
    }
    size_t idx = 0;

    while(idx < data.size() && (!slide_window.full()) ) {
        slide_window.push(data[idx++]);
    }
    _bytes_written += idx;

    return idx;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    // cout << "peek_output " << len << "Bytes\n";
    const size_t sz = std::min(len, slide_window.size());
    return std::move(slide_window.top(sz));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    // DUMMY_CODE(len); 
    const size_t sz = std::min(len, slide_window.size());

    slide_window.pop(sz);
    _bytes_read += sz;

    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len)
    if(_end_output) {
        return string();
    }
    string ret = peek_output(len);
    pop_output(len);

    return ret;
}

void ByteStream::end_input() {
    _end_input = true;
}

bool ByteStream::input_ended() const { 
    return _end_input; 
}

size_t ByteStream::buffer_size() const { 
    return slide_window.size(); 
}

bool ByteStream::buffer_empty() const { 
    return slide_window.empty(); 
}

bool ByteStream::eof() const { 
    return _end_input && buffer_empty(); 
}

size_t ByteStream::bytes_written() const { 
    return _bytes_written; 
}

size_t ByteStream::bytes_read() const { 
    return _bytes_read; 
}

size_t ByteStream::remaining_capacity() const { 
    return _capacity - slide_window.size(); 
}

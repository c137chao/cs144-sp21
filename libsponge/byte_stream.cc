#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) 
    : _byte_buffer(my_queue(capacity)), _capacity(capacity), _bytes_written(0),
     _bytes_read(0), _end_input(false), _end_output(false), _error(false)
{ /* DUMMY_CODE(capacity); */ }

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data); 
    if (input_ended()) {
        return 0;
    }
    size_t size = min(data.size(), _byte_buffer.remain_capcity());
    for (size_t i = 0; i < size; i++) {
        _byte_buffer.push(data[i]);
    }

    _bytes_written += size;
    // cout << ">>Bytestream write: " << _bytes_written << "bits, data:" << data << endl;

    return size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    const size_t sz = std::min(len, _byte_buffer.size());
    return std::move(_byte_buffer.top(sz));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    // DUMMY_CODE(len); 
    const size_t sz = std::min(len, _byte_buffer.size());

    _byte_buffer.pop(sz);
    _bytes_read += sz;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    if (_end_output) {
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
    return _byte_buffer.size(); 
}

bool ByteStream::buffer_empty() const { 
    return _byte_buffer.empty(); 
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
    return _byte_buffer.capacity() - _byte_buffer.size(); 
}

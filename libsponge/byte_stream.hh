#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH

#include <string>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

//! \brief An in-order byte stream.

//! Bytes are written on the "input" side and read from the "output"
//! side.  The byte stream is finite: the writer can end the input,
//! and then no more bytes can be written.

struct my_queue{

  std::string _data;

  size_t _capacity;
  size_t _write;
  size_t _read;

// constructer
  my_queue(size_t cap)
    : _data(std::string(cap + 1, '0')), _capacity(cap + 1), _write(0), _read(0) {}
// API
  // if queue is empty, return true
  bool empty() const {
    return _write == _read;
  }
  // if queue if full, return true
  bool full()  const {
    return ((_write + 1) % _capacity) == _read ; 
  }
  // return elem count of queue
  size_t size() const { 
    return ( _write - _read  + _capacity ) % _capacity; 
  }
  // return total capcity of queue
  size_t capacity() const {
    return _capacity;
  }
  // push a char to the back of queue
  void push(char c) {
    if(full()){
      throw std::out_of_range("push err: queue is full");
    }
    _data[_write] = c; 
    _write = (_write + 1) % _capacity;
  }
  // pop a char from the front of queue
  void pop() {
    if(empty()) {
      throw std::out_of_range("pop err: queue is empty");
    }
    _read = (_read + 1) % _capacity;
  }

  void pop(const size_t n) {
    if(size() < n) {
      throw std::out_of_range("pop(n) err: queue hasn't enough elem");
    }
      _read = (_read + n) % _capacity;
  }

  char top() const {
    if(empty()) {
      throw std::out_of_range("top err: queue is empty");
    }
    return _data[_read];
  }

  std::string top(const size_t n) const {
    if(size() < n) {
      throw std::out_of_range("top(n) err: queue hasn't enough elem");
    }

    size_t sz = std::min(n, size());
    if(_read + sz < _capacity) {
      return std::move(std::string(_data, _read, sz));
    }else{
      return std::move(std::string(_data, _read, _capacity - _read) + std::string(_data, 0, sz - (_capacity - _read)));
    }

  }
};


class ByteStream {
  private:
    // Your code here -- add private members as necessary.
    my_queue slide_window;
    std::size_t _capacity;
    std::size_t _bytes_written;
    std::size_t _bytes_read;
    bool _end_input;
    bool _end_output;
    // Hint: This doesn't need to be a sophisticated data structure at
    // all, but if any of your tests are taking longer than a second,
    // that's a sign that you probably want to keep exploring
    // different approaches.

    bool _error{};  //!< Flag indicating that the stream suffered an error.

  public:
    //! Construct a stream with room for `capacity` bytes.
    ByteStream(const size_t capacity);

    //! \name "Input" interface for the writer
    //!@{

    //! Write a string of bytes into the stream. Write as many
    //! as will fit, and return how many were written.
    //! \returns the number of bytes accepted into the stream
    size_t write(const std::string &data);

    //! \returns the number of additional bytes that the stream has space for
    size_t remaining_capacity() const;

    //! Signal that the byte stream has reached its ending
    void end_input();

    //! Indicate that the stream suffered an error.
    void set_error() { _error = true; }
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! Peek at next "len" bytes of the stream
    //! \returns a string
    std::string peek_output(const size_t len) const;

    //! Remove bytes from the buffer
    void pop_output(const size_t len);

    //! Read (i.e., copy and then pop) the next "len" bytes of the stream
    //! \returns a string
    std::string read(const size_t len);

    //! \returns `true` if the stream input has ended
    bool input_ended() const;

    //! \returns `true` if the stream has suffered an error
    bool error() const { return _error; }

    //! \returns the maximum amount that can currently be read from the stream
    size_t buffer_size() const;

    //! \returns `true` if the buffer is empty
    bool buffer_empty() const;

    //! \returns `true` if the output has reached the ending
    bool eof() const;
    //!@}

    //! \name General accounting
    //!@{

    //! Total number of bytes written
    size_t bytes_written() const;

    //! Total number of bytes popped
    size_t bytes_read() const;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_BYTE_STREAM_HH

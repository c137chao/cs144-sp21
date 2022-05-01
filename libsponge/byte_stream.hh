#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH

#include <queue>
#include <deque>
#include <string>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

//! \brief An in-order byte stream.

class MyBuffer {
  private:
    std::shared_ptr<std::string> _storage{};
    size_t _starting_offset{};
    size_t _ending_offset{};

  public:
    MyBuffer() = default;
    MyBuffer(const std::string &str) : _storage(std::make_shared<std::string>(str)) {}
    //! \brief Construct by taking ownership of a string
    MyBuffer(std::string &&str) noexcept : _storage(std::make_shared<std::string>(std::move(str))) {}

    //! \name Expose contents as a std::string_view
    //!@{
    std::string_view str() const {
        if (not _storage) {
            return {};
        }
        return {_storage->data() + _starting_offset, _storage->size() - _starting_offset};
    }

    operator std::string_view() const { return str(); }
    //!@}

    //! \brief Get character at location `n`
    uint8_t at(const size_t n) const { return str().at(n); }

    //! \brief Size of the string
    size_t size() const { return str().size(); }

    //! \brief Make a copy to a new std::string
    std::string copy() const { return std::string(str()); }

    //! \brief Discard the first `n` bytes of the string (does not require a copy or move)
    //! \note Doesn't free any memory until the whole string has been discarded in all copies of the Buffer.
    void remove_prefix(const size_t n);

    void remove_suffix(const size_t n);
};

class ByteStream {
  private:
    // Your code here -- add private members as necessary.
    std::deque<char> _buffer_list;
    
    std::size_t _capacity;
    std::size_t _bytes_written{};
    std::size_t _bytes_read{};
    // std::size_t _size{};
  
    bool _end_input {false};
    bool _end_output{false};
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

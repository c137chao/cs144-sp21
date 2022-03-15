#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <map>
#include <iostream>

// A structure which store and op of segments
// if we want to alter struture of buffer, we don't need to alter StreamReassembler
struct Buffer {
  std::map <size_t, std::string> _data;

  Buffer() : _data() {}

  bool empty() const { return _data.empty();  }

  bool merge_forward(size_t& unreassemble, size_t& index, std::string& data) {
    auto upper_iter = _data.lower_bound(index); // >=

    // merge forward
    if(upper_iter != _data.end()) {
      
      if(upper_iter->first + upper_iter->second.size() <= index + data.size()) {
        unreassemble -= upper_iter->second.size();
        _data.erase(upper_iter); 
        merge_forward(unreassemble, index, data);

      } else if (upper_iter->first == index) {
        return false;
      } else if (upper_iter->first < index + data.size()) {
        data = data.substr(0, upper_iter->first - index);
      }

    }
    return true;
  }

  bool merge_backward(size_t& index, std::string& data) {
    auto lower_iter = _data.upper_bound(index);
    if(lower_iter != _data.begin()) {
      lower_iter--;
      if(lower_iter != _data.end()) {
        if(lower_iter->first + lower_iter->second.size() > index) {
          if(lower_iter->first + lower_iter->second.size() > index + data.size())
            return false;
          data = data.substr(lower_iter->first + lower_iter->second.size() - index);
        }
      }
    }
    return true;
  }

  void insert(size_t& unreassemble, size_t index, std::string data) {
    if( _data.empty() || (merge_forward(unreassemble, index, data) && merge_backward(index, data)) ) {
      unreassemble += data.size();
      _data.insert({index, data});
    }
  }

  void send(ByteStream& out, size_t& expected, size_t& unreassemble) {
    auto iter = _data.begin();

    while(iter != _data.end() && iter->first <= expected) {
      std::string data = iter->second;
      size_t index = iter->first;
      auto temp = iter;
      if(index + data.size() > expected) {

        if(index < expected) {
          data = data.substr(expected - index);
        }
        if(data.size() > out.remaining_capacity()) {
          data = data.substr(0, out.remaining_capacity());
        }
        ++iter;
        out.write(data);
        expected += data.size();
        unreassemble -= data.size();
        }
      
      _data.erase(temp);
    }
  }
  //...
};


//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    Buffer _reassemble_buffer;      // store received substring

    size_t _expected;    // The index of bytes which not yet reassembled
    size_t _eof_index;   // the last byte of string
    size_t _unassembled_bytes;

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

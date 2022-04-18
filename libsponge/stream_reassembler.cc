#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) 
    :_reassemble_buffer(std::map<size_t, string>()), _expected(0),  _eof_index(-1), _unassembled_bytes(0), _output(capacity), _capacity(capacity)
{}

inline
bool repeated_interval(size_t left_begin, size_t left_length, size_t right_begin, size_t right_length) {
  return right_begin + right_length <= left_begin + left_length;
}

bool StreamReassembler::merge(size_t* index, std::string* data) {
    // find first elem which great equal than index
      auto merge_iter = _reassemble_buffer.lower_bound(*index); 

      // merge forward
      // abandon all repeated segment in reassemble buffer
      while(merge_iter != _reassemble_buffer.end() && \
            repeated_interval(*index, data->size(), merge_iter->first, merge_iter->second.size())) {

          _unassembled_bytes -= merge_iter->second.size();
          auto temp_iter = merge_iter;
          merge_iter++;
          _reassemble_buffer.erase(temp_iter); 
      }

      if (merge_iter != _reassemble_buffer.end()) {
        if (merge_iter->first == *index) {
          return false;
        } else if (merge_iter->first < *index + data->size()) {
          *data = std::move(data->substr(0, merge_iter->first - *index));
        }
      }

      // merge backward
      if (merge_iter != _reassemble_buffer.begin()) {
        --merge_iter;
        if (merge_iter->first + merge_iter->second.size() > *index) {
          if (merge_iter->first + merge_iter->second.size() > *index + data->size()) {
              return false;
          }
          *data = std::move(data->substr(merge_iter->first + merge_iter->second.size() - *index));
          *index = merge_iter->first + merge_iter->second.size();
        }
      }

      return true;
}

void StreamReassembler::insert(size_t index, std::string data) {
    if (_reassemble_buffer.empty() || merge(&index, &data)) {
      // cout << "\t>>insert:" << data << " at index:" << index << " expected: "<< _expected << endl;
      _unassembled_bytes += data.size();
      _reassemble_buffer.emplace(index, data);
    }
  }

void StreamReassembler::send_to_bytestream() {
    auto iter = _reassemble_buffer.begin();

    while (iter != _reassemble_buffer.end() && iter->first <= _expected) {
      std::string data = iter->second;
      size_t     index = iter->first;
      auto temp = iter;
      ++iter;

      if (index + data.size() > _expected) {
        if (index < _expected) {
          data = data.substr(_expected - index);
        }
        if (data.size() > _output.remaining_capacity()) {
          data = data.substr(0, _output.remaining_capacity());
        }

        _output.write(data);
        _expected += data.size();
        _unassembled_bytes -= data.size();
      }

    _reassemble_buffer.erase(temp);
    }
  }

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // index = 0, data = “”， should send. not ignore
    if(eof){
        _eof_index = data.size() + index;
    }
    if(index + data.size() > _expected ) {
        // cout << ">push_substring:" << data << " at index:" << index << endl;
        insert(index, data);
        send_to_bytestream();
    }

    if (_expected == _eof_index) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

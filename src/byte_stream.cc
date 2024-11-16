#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
: capacity_( capacity ) 
 {}

bool Writer::is_closed() const
{
  // Your code here.
  return close_;
}

void Writer::push( string data )
{
  // Your code here.
  if(has_error()) return;
  if(is_closed() || data.empty() || available_capacity() == 0) return;
  if(data.length() > available_capacity()) 
    data.resize(available_capacity());

  bytes_buffered_+=data.length();
  bytes_pushed_ += data.length();
  stream_.push(data);
  return;
}

void Writer::close()
{
  // Your code here.
  close_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - bytes_buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return close_ && bytes_buffered_ == 0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_; 
}

string_view Reader::peek() const
{
  // Your code here.
  if(bytes_buffered_ == 0) return {};

  return string_view(stream_.front()).substr(to_delete);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  bytes_buffered_ -= len;
  bytes_popped_ += len;

  while(len != 0U)
  {
    const uint64_t& size = stream_.front().length() - to_delete;
    if(len < size) 
    {
      to_delete += len;
      break;
    }
    stream_.pop();
    len -= size;
    to_delete = 0;
  }

}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_buffered_;
}

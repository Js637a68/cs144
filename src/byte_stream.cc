#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
: capacity_( capacity ) 
, buffer()
, end_write(false)
, push_count(0)
, pop_count(0)
 {}

bool Writer::is_closed() const
{
  // Your code here.
  return end_write;
}

void Writer::push( string data )
{
  // Your code here.
  for(auto c : data)
  {
    if(buffer.size() >= capacity_) 
      return;
    buffer.push_back(c);
    push_count++;
  }
  return;
}

void Writer::close()
{
  // Your code here.
  end_write = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return push_count;
}

bool Reader::is_finished() const
{
  // Your code here.
  return buffer.empty() && end_write;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return pop_count; 
}

string_view Reader::peek() const
{
  // Your code here.
  if(buffer.empty() || has_error()) 
    return {};
  return string_view(&buffer.front(), 1);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if(len > buffer.size())
    return;

  for(uint64_t i = 0; i < len; ++i)
  {
    pop_count++;
    buffer.pop_front();
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer.size();
}

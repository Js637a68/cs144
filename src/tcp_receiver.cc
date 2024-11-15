#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(writer().has_error() && writer().is_closed()) return;
  if(message.RST) 
  {
    reader().set_error();
    return;
  }

  if(!zero_point.has_value())
  {
    if(!message.SYN) return;
    zero_point.emplace(message.seqno);
  }

  const uint64_t checkpoint = writer().bytes_pushed() + 1;
  const uint64_t SYN_absolute_seqno = message.seqno.unwrap(zero_point.value(), checkpoint);
  const uint64_t stream_index = SYN_absolute_seqno - 1 + static_cast<uint64_t>( message.SYN );
  
  reassembler_.insert(stream_index , move(message.payload), message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  uint64_t MAX_UINT16 = (1LL << 16) - 1;
  uint16_t wsize{ static_cast<uint16_t>(writer().available_capacity() >= MAX_UINT16 ? MAX_UINT16 : writer().available_capacity())};
  bool RST{writer().has_error()};

  if(zero_point.has_value()) 
  {
    uint64_t n =  writer().bytes_pushed() + 1LL + static_cast<uint64_t>( writer().is_closed()) ;
    return {Wrap32::wrap(n, zero_point.value()), wsize, RST};
  }
  return {nullopt, wsize, RST};
}

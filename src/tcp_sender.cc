#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return outstanding_count;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmission_count;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  uint64_t new_window_size_ = window_size_ == 0 ? 1 : window_size_;
  while(new_window_size_ > outstanding_count)
  {

    if(FIN) return;
    auto msg {make_empty_message()};

    if(input_.has_error()) {msg.RST = true; transmit(msg); return;}
    if(!SYN)
    {
      msg.SYN = true;
      SYN = true;
    }
    auto& payload = msg.payload;

    uint64_t len = min(TCPConfig::MAX_PAYLOAD_SIZE, new_window_size_ - outstanding_count);
    while(len > 0 && reader().bytes_buffered() != 0)
    {
      string_view data = reader().peek();
      auto l = min(data.length(), len);
      len -= l;
      data = data.substr(0,l);
      payload += data;
      input_.reader().pop(l);
    }

    if ( !FIN && new_window_size_ > outstanding_count + msg.sequence_length()  && reader().is_finished() ) 
    {
      msg.FIN = true;
      FIN = true;
    }
    if(msg.SYN && new_window_size_ == 1 && reader().is_finished())
    {
      msg.FIN = true;
      FIN = true;
    }
    

    
    if(msg.sequence_length() == 0) return;
    transmit(msg);

    if(!Timer_.is_start()) 
    {
      Timer_.start();
      Timer_.reload(initial_RTO_ms_);
    }
    outstanding_count += msg.sequence_length();
    next_ab_seqno += msg.sequence_length();
    outstanding_message_.emplace( move(msg) );
  }


}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return {Wrap32::wrap(next_ab_seqno, isn_), false, {}, false, input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if(msg.RST) input_.set_error();
  if(input_.has_error()) return;
  if(!msg.ackno.has_value()) { return;}

  auto ab_ackno = msg.ackno.value().unwrap(isn_, next_ab_seqno);
  if(ab_ackno > next_ab_seqno) return;

  window_size_ = msg.window_size;

  bool is_reset = false;
  while(!outstanding_message_.empty())
  {
    auto& mess = outstanding_message_.front();

    if( mess.sequence_length() + window_left > ab_ackno) break;
    window_left += mess.sequence_length();
    outstanding_count -= mess.sequence_length();
    outstanding_message_.pop();
    is_reset = true;
  }

  if(is_reset)
  {
    Timer_.reload(initial_RTO_ms_);
    retransmission_count = 0;
    Timer_.stop();
    if(!outstanding_message_.empty()) Timer_.start();
  }
  
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if(!Timer_.is_start()) return;
  Timer_.tick(ms_since_last_tick);
  if(!Timer_.is_expired()) return;
  if(outstanding_count == 0) return;

  transmit( outstanding_message_.front() );
  if ( window_size_ != 0 ) {
    retransmission_count += 1;
    Timer_.double_RTO();
  }
  Timer_.start();
}

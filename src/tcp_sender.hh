#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class RetransmissionTimer
{
public:
  void reload(uint64_t initial_RTO_ms_) { RTO_ms_ = initial_RTO_ms_;; }
  void tick(uint64_t ms_since_last_tick) { timer_ += ms_since_last_tick; }
  void reset() { timer_ = 0; }
  bool is_expired() { return timer_ >= RTO_ms_; }
  void double_RTO() { RTO_ms_ += RTO_ms_; }
  void start() { start_ = true; reset(); }
  void stop() { start_ = false; }
  bool is_start() { return start_; }

private:
  bool start_{};
  uint64_t timer_{};
  uint64_t RTO_ms_{};
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  std::queue<TCPSenderMessage> outstanding_message_{};
  uint64_t retransmission_count{}; 
  uint64_t outstanding_count{};
  uint16_t window_size_{1};

  RetransmissionTimer Timer_{};

  bool SYN{};
  bool FIN{};

  uint64_t next_ab_seqno{};
  uint64_t window_left{};
};

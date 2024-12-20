#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

ARPMessage NetworkInterface::make_arp( const uint16_t opcode,
                     const EthernetAddress target_ethernet_address,
                     const uint32_t& target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = ethernet_address_;;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_hop_ip = next_hop.ipv4_numeric();

  if(ARP_cache_.contains(next_hop_ip)) 
  {
    auto & dst {ARP_cache_[next_hop_ip].first};
    return transmit( { { dst, ethernet_address_, EthernetHeader::TYPE_IPv4 }, serialize(dgram) } );
  }

  wait_to_send[next_hop_ip].emplace_back(dgram);
  if(ARP_timer_.contains(next_hop_ip)) return;
  ARP_timer_[next_hop_ip] = 0;
  const ARPMessage arp_request { make_arp(ARPMessage::OPCODE_REQUEST, {}, next_hop_ip) };
  transmit( { { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP }, serialize(arp_request)} );

}

  /*
  EthernetFrame = { header{dst, src, type }, payload}
  ARPMessage = make_arp(  uint16_t opcode, 
                          EthernetAddress sender_ethernet_address,
                          uint32_t sender_ip_address,
                          EthernetAddress target_ethernet_address,
                          uint32_t target_ip_address)

  */
//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) return;
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) 
  {
    InternetDatagram ipv4_datagram;
    if ( parse( ipv4_datagram, frame.payload ) ) 
    {
      datagrams_received_.emplace( move( ipv4_datagram ) );
    }
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_ARP ) 
  {
    ARPMessage msg;
    parse( msg, frame.payload );
    const uint32_t peer_ip { msg.sender_ip_address };
    const EthernetAddress peer_eth { msg.sender_ethernet_address };
    ARP_cache_[peer_ip] = { peer_eth, EXPIR_TIME};

    if ( msg.opcode == ARPMessage::OPCODE_REQUEST and msg.target_ip_address == ip_address_.ipv4_numeric() ) 
    {
      const ARPMessage arp_reply { make_arp( ARPMessage::OPCODE_REPLY, peer_eth, peer_ip) };
      transmit( { { peer_eth, ethernet_address_, EthernetHeader::TYPE_ARP }, serialize( arp_reply ) } );
    }

    if(wait_to_send.contains(peer_ip)) 
    {
      auto&& list = wait_to_send[peer_ip];
      for(auto& dgram:list) 
        transmit( { { peer_eth, ethernet_address_, EthernetHeader::TYPE_IPv4 }, serialize(dgram) } );
      wait_to_send.erase(peer_ip);
      ARP_timer_.erase(peer_ip);
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  erase_if(ARP_cache_, [&](auto& it) noexcept {
    if(it.second.second <= ms_since_last_tick) return true;
    it.second.second -= ms_since_last_tick;
    return false;
  });

  erase_if(ARP_timer_, [&](auto& it) {
    it.second += ms_since_last_tick;
    return it.second >= ARP_TIMEOUT;
  });
}

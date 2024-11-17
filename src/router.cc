#include "router.hh"

#include <bit>
#include <cstddef>
#include <iostream>
#include <optional>
#include <ranges>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  routing_table_[route_prefix].push_back( {prefix_length, interface_num, next_hop} );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( const auto& interface : _interfaces ) {
    auto&& datagrams_received { interface->datagrams_received() };
    while ( not datagrams_received.empty() ) {
      InternetDatagram datagram { move( datagrams_received.front() ) };
      datagrams_received.pop();

      if ( datagram.header.ttl <= 1 ) {
        continue;
      }
      datagram.header.ttl -= 1;
      datagram.header.compute_checksum();

      const optional<info>& mp { match( datagram.header.dst ) };
      if ( not mp.has_value() ) {
        continue;
      }
      const auto& [_, num, next_hop] { mp.value() };
      _interfaces[num]->send_datagram( datagram,
                                       next_hop.value_or( Address::from_ipv4_numeric( datagram.header.dst ) ) );
    }
  }
}

auto Router::match( uint32_t addr ) const noexcept -> optional<info>
{
  optional<info> res =nullopt;
  for(auto& item:routing_table_)
  {
    for(auto& i:item.second)
    {
      uint8_t umask = 32 - get<0>(i);
      uint32_t di = ( addr&~( (1LL << umask) - 1) );
      if(item.first != di ) continue;
      if (res == nullopt || get<0>(res.value()) < get<0>(i)) res = i;
    }
  }
  return res;
}
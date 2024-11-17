#include "router.hh"

#include <iostream>
#include <limits>

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

  // Your code here.
  router_[make_pair(route_prefix, prefix_length)] = make_pair(next_hop, interface_num);

}

  /*
   *   0                   1                   2                   3
   *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *  |Version|  IHL  |Type of Service|          Total Length         |
   *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *  |         Identification        |Flags|      Fragment Offset    |
   *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *  |  Time to Live |    Protocol   |         Header Checksum       |
   *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *  |                       Source Address                          |
   *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *  |                    Destination Address                        |
   *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *  |                    Options                    |    Padding    |
   *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for(auto& p:_interfaces)
  {
    auto&& interface = p.get();
    while(!datagrams_received_.empty())
    {
      auto&& dgram = datagrams_received_.front();
      if(dgram.header.ttl <= 1) continue;

      dgram.header.ttl--;
      
      const optional<RouteInfo>& mp { match( datagram.header.dst ) };
      if(!mp.has_value()) continue;

      const auto& [next_hop, num] { mp.value() };
      _interfaces[num].send_datagram(dgram, next_hop.value());
    }
  }
}

optional<RouteInfo> Router::match(uint32_t ipv4)
{
  map<uint8_t, RouteInfo> ans_set;
  for(auto& router : router_)
  {
    auto& [netmask, info] = router;
    if(netmask.first == (ipv4 && ~(1U<<(32-netmask.second) - 1) )) ans_set.insert(move(info));
  }
  if(ans_set.empty()) return nullopt;
  return {ans_set.cbegin()->seocnd};
}

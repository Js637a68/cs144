#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{

  // Your code here.
  const uint64_t a = raw_value_ - zero_point.raw_value_;
  uint64_t res = (checkpoint & ~(BASE_-1)) | a;

  if(res >= BASE_ && res > checkpoint+BASE_/2) res -= BASE_;
  if(res+BASE_/2 < checkpoint) res+=BASE_;
  return res;
}

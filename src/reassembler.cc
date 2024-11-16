#include "reassembler.hh"
#include <iostream>

using namespace std;

auto Reassembler::split(uint64_t x)
{
  auto it = odt.lower_bound(Node_t{x, 0, ""});
  if (it != odt.end() && it->l == x) return it;
  if(it == odt.begin()) return it;
  --it;
  uint64_t l = it->l, r = it->r;
  string v = it->v;
  if(r > x)
  {
    odt.erase(it);
    odt.insert(Node_t(l, x, v.substr(0,x-l)));
    return odt.insert(Node_t(x, r, v.substr(x-l))).first;
  }
  return ++it;
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring)
{
  // Your code here.
  if(writer().has_error()) return;

  auto do_close = [&]{
    if(!end_index_.empty() && end_index_.front() == writer().bytes_pushed())
      output_.writer().close();
  };

  if(writer().is_closed() || writer().available_capacity() == 0U) return;

  if(data.empty() && is_last_substring && end_index_.empty()) 
  {
    end_index_.push_back(first_index);
    return do_close();
  }

  if(!end_index_.empty() && first_index >= end_index_.front()) return do_close();

  uint64_t first_unassembled_index = writer().bytes_pushed();
  uint64_t first_unacceptable_index = first_unassembled_index + writer().available_capacity();

  if(first_index >= first_unacceptable_index ||first_index + data.length() <= first_unassembled_index) return;

  if(first_index < first_unassembled_index) 
  {
    data.erase(0, first_unassembled_index - first_index);
    first_index = first_unassembled_index;
  }

  if(is_last_substring && end_index_.empty()) end_index_.push_back(first_index + data.length());

  if(first_index + data.length() > first_unacceptable_index) data.resize(first_unacceptable_index - first_index);

  uint64_t l = first_index, r = first_index + data.length();

  auto it_r = split(r);
  auto it_l = split(l);
  for(auto i = it_l; i != it_r; ++i) bytes_pending_ -= i->r - i->l;
  odt.erase(it_l, it_r);

  odt.insert({l,r,data});
  bytes_pending_ += r-l;

  auto it = odt.begin();
  while(it->l == writer().bytes_pushed() && it != odt.end())
  {
    bytes_pending_ -= it->r -it->l;
    output_.writer().push(it->v);
    it = odt.erase(it);
  }

  do_close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}

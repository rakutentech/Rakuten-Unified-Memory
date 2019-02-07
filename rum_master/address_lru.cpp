#include "address_lru.hpp"

#include <utility>
#include <iostream>


#include <algorithm>

#include "util.h"

size_t AddressLRU::count_ = 0;

AddressLRU::AddressLRU()
{
  count_++;
  clear();
  init();
}

AddressLRU::AddressLRU(const AddressLRU &rhs)
{
  count_++; 
  clear();
  init();
}
//------------------------------------------------------------------------------
AddressLRU::~AddressLRU()
{
  count_--;
  printf("AddressLRU instance remaining %ld\n", count_);
  reportStats();
  printf("AddressLRU end\n");
}
//------------------------------------------------------------------------------
void AddressLRU::init(size_t size)
{
  size_limit_ = size;
}
//------------------------------------------------------------------------------
AddressLRU::Addr AddressLRU::update(Addr address){
  // lock mutex before accessing
    
  Addr stale;

  cache_mutex.lock();
  
  count_access_++;

  // if the address is already in there, put it to the back
  if(std::find(history_.begin(), history_.end(), address) != history_.end()){
    printf("History: found existing (%lx)\n", address);
    history_.remove(address);
    history_.push_back(address);
    cache_mutex.unlock();
    return 0;
  }
  else{
    //  printf("History: push back %lx\n", address);
    history_.push_back(address);  
  }
  

  if (history_.size() > size_limit_) {
    stale = history_.front();
    history_.pop_front();
    count_stale_++; // stats
  } else {
    stale = 0; // not stale
  }

  cache_mutex.unlock();
  
  return stale;
}
//------------------------------------------------------------------------------
void AddressLRU::clear(void)
{
  cache_mutex.lock();
  history_.clear();

  count_access_ = 0;
  count_stale_ = 0;
  cache_mutex.unlock();
}
//------------------------------------------------------------------------------
size_t AddressLRU::size(void) const
{
  return history_.size();
}
//------------------------------------------------------------------------------
void AddressLRU::reportStats(void) const
{
  printf("AddressLRU statistics: stale: %ld / %ld", count_stale_, count_access_);
  if (count_access_)
    printf(" ( %lf percent)", count_stale_ / count_access_ * 100);
  printf("\n");
}
//------------------------------------------------------------------------------
bool AddressLRU::contains(Addr address){
  return (std::find(history_.begin(), history_.end(), address) != history_.end());
}
//------------------------------------------------------------------------------
void AddressLRU::purge(Addr start, Addr end){
  std::list<Addr>::iterator cache_iter;
  std::list<Addr> victims;
  cache_mutex.lock();
  
  // there may be multiple pages from the interval!
  //printf("PURGE: %lx -> %lx (%ld bytes)\n", start, end, (end - start));
  for(cache_iter = history_.begin() ; cache_iter != history_.end() ; cache_iter++){
    //printf("\t%lx\n", *cache_iter);
    if(*cache_iter >= start && *cache_iter < end){
      //printf("\t\t%lx\n", *cache_iter);
      victims.push_back(*cache_iter);     
    }
  }
  
  for(cache_iter = victims.begin() ; cache_iter != victims.end() ; cache_iter++){
      history_.remove(*cache_iter);
  }
  cache_mutex.unlock();
}
//------------------------------------------------------------------------------
void AddressLRU::dump(){
  std::list<Addr>::iterator cache_iter;
  
  cache_mutex.lock();
 
  int count  = 0;
  printf("CACHE_OCCUPANCY : %ld\n", history_.size());
  for(cache_iter = history_.begin() ; cache_iter != history_.end() ; cache_iter++){
    printf("[%d] %lx\n",count++, *cache_iter);
  }
  cache_mutex.unlock();
}
//------------------------------------------------------------------------------
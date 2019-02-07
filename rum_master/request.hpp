#ifndef REQUEST_
#define REQUEST_

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "histogram.hpp"

/*
TODO support multiple request for reading or writing to the same address
*/
/*
record time stamp for each add. grow vector(pair(addr,ts)) --> keep order
record time stamp for each erase, add unordoreed map(addr,ts)
note: can add ts be the same? probably not because of mutex
calculate duration for each erase
output to file: order index, addr, duration
*/

template <class T, bool Logging>
class Request
{
public:
  typedef uint64_t Addr;
  typedef std::chrono::high_resolution_clock Clock;
  typedef Clock::time_point TimePoint;
  //typedef std::vector< std::pair< size_t, TimePoint> > TimeTrace;

  Request()
  : histogram_(32, "request")
  { count_++; }
  Request(const Request&)
  : histogram_(32, "request")
  { count_++; }
  ~Request() {
    count_--;
    std::cout << "Request instances remaining: " << count_ 
              << " member_max_ " << member_max_
              << std::endl;
    if (Logging) report("request.txt");
  }

  static size_t howMany() { return count_; }

  /**
   * \return In add mode, return 0 always. In erase mode, return 1 if value erase, 0 otherwise.
   * TODO allow multiple instance of same val.
   */
  size_t add0erase1(const T val, const size_t mode)
  {
    // mutex to protect access (shared across threads)
    static std::mutex mutex;
    // lock mutex for access
    std::lock_guard<std::mutex> lock(mutex);

    //member_max_ = std::max(member_max_, member.size()); // stats
    //histogram_.push(member.size());

    if (mode==0) {
      member.emplace(val);
      if (Logging) order_.push_back(std::make_pair(val, Clock::now()));
      return 0;
    } else {
      if (Logging) erase_.emplace(std::make_pair(val, Clock::now()));
      return member.erase(val);
    }
  }

  /*
  void add(const T val)
  {
    // mutex to protect access (shared across threads)
    static std::mutex mutex;

    // lock mutex for access
    std::lock_guard<std::mutex> lock(mutex);

    member.emplace(val);
  }
  */

  bool missing(const T val) const
  {
    return (member.count(val) == 0);
  }

  /*
  size_t erase(const T val)
  {
    // mutex to protect access (shared across threads)
    static std::mutex mutex;
    // lock mutex for access
    std::lock_guard<std::mutex> lock(mutex);

    return member.erase(val);
  }
  */

  size_t size(void) const
  {
    return member.size();
  }

  size_t max(void) const
  {
    return member_max_;
  }

  void report(const std::string &file_name) const
  {
    //output to file: order index, addr, duration
    std::cout << "Request<>::" << __func__ << std::endl;
    std::cout << "Number of requests: " << order_.size() << std::endl;
    std::ofstream out_file(file_name);
    if (out_file.is_open()) {
        for (size_t row = 0; row < order_.size(); row++) {
          auto address = order_[row].first;
          auto t0 = order_[row].second;
          //TimePoint t1 = erase_[address];
          //std::unordered_map< T, TimePoint >::iterator got = erase_.find(address);
          auto got = erase_.find(address);
          double duration_usec = 0;
          if (got != erase_.end()) {
            std::chrono::duration<double, std::micro> fp_usec = got->second-t0;
            duration_usec = fp_usec.count();
          }
          out_file << row << " " << address << " " << duration_usec << std::endl;
        }
        out_file.close();
    } else {
        std::cout << "Could not open " << file_name << " for writing" << std::endl;
    }
  }

private:
  std::unordered_set<T> member;
  size_t member_max_;

  std::vector< std::pair< T, TimePoint> > order_; // DEBUG
  std::unordered_map< T, TimePoint > erase_; // DEBUG
  static size_t count_; // DEBUG
  UIntHistogram histogram_;
};

template <typename T, bool U>
size_t Request<T, U>::count_ = 0;

#endif

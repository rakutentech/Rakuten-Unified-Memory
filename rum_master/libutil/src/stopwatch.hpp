#ifndef STOPWATCH_HPP_
#define STOPWATCH_HPP_

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

/**
 * A stop watch with start, stop and elapsed functions.
 */
class StopWatch
{
  public:
    typedef std::chrono::high_resolution_clock Clock;

    StopWatch()
    : running_(false)
    {}

    void start(void)
    {
      running_ = true;
      t2_ = t1_ = Clock::now();
    }

    double elapsed_usec(void) const
    {
      std::chrono::duration<double, std::micro> fp_usec;
      fp_usec = (running_ ? Clock::now() : t2_) - t1_;
      return fp_usec.count();
    }

    void stop(void)
    {
      running_ = false;
      t2_ = Clock::now();
    }

    friend std::ostream& operator<<(std::ostream&, const StopWatch&);

  protected:
  private:
    bool running_;
    Clock::time_point t1_;
    Clock::time_point t2_;
};

std::ostream& operator<<(std::ostream& os, const StopWatch& obj)
{
  os << obj.elapsed_usec();
  return os;
}

#endif // STOPWATCH_HPP_

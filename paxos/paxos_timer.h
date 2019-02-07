/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   paxos_timer.h
 * Author: Adapted from the following 
 * https://codereview.stackexchange.com/questions/40473/portable-periodic-one-shot-timer-implementation
 *
 * Created on 2018/10/03, 11:20
 */

#ifndef __PAXOS_TIMER_H__
#define __PAXOS_TIMER_H__

#include <algorithm>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <set>
#include <cstdint>

#include <thread>
#include <mutex>
#include <condition_variable>

class TimerThread
{
public:
    // Each Timer is assigned a unique ID of type timer_id
    using timer_id = std::uint64_t;

    // Valid IDs are guaranteed not to be this value
    static timer_id constexpr no_timer = timer_id(0);

    //
    // Callback storage

    // Function object we actually use
    using handler_type = std::function<void()>;

    // Function object that we boil down to handler_type with std::bind
    template<typename... Args>
    using bound_handler_type = std::function<void(Args...)>;

    // Values that are a large-range millisecond count
    using millisec = std::int64_t;

    // Constructor does not start worker until there is a Timer
    explicit TimerThread();

    // Destructor is thread safe, even if a timer
    // callback is running. All callbacks are guaranteed
    // to have returned before this destructor returns
    ~TimerThread();

    // Create timer using milliseconds
    // The delay will be called msDelay milliseconds from now
    // If msPeriod is nonzero, call the callback again every
    // msPeriod milliseconds
    // All timer creation functions eventually call this one
    timer_id addTimer(millisec msDelay,
                      millisec msPeriod,
                      handler_type handler);

    // Create timer using std::chrono delay and period
    // Optionally binds additional arguments to the callback
    template<typename SRep, typename SPer,
             typename PRep, typename PPer,
             typename... Args>
    timer_id addTimer(
            typename std::chrono::duration<SRep, SPer> const& delay,
            typename std::chrono::duration<PRep, PPer> const& period,
            bound_handler_type<Args...> handler,
            Args&& ...args);

    // Create timer using millisecond units delay and period
    // Optionally binds additional arguments to the callback
    template<typename... Args>
    timer_id addTimer(millisec msDelay,
                      millisec msPeriod,
                      bound_handler_type<Args...> handler,
                      Args&& ...args);

    // setInterval API like browser javascript
    // Call handler every `period` milliseconds,
    // starting `period` milliseconds from now
    // Optionally binds additional arguments to the callback
    timer_id setInterval(handler_type handler,
                         millisec period);

    // setTimeout API like browser javascript
    // Call handler once `timeout` ms from now
    timer_id setTimeout(handler_type handler,
                        millisec timeout);

    // setInterval API like browser javascript
    // Call handler every `period` milliseconds,
    // starting `period` milliseconds from now
    template<typename... Args>
    timer_id setInterval(bound_handler_type<Args...> handler,
                         millisec period,
                         Args&& ...args);

    // setTimeout API like browser javascript
    // Call handler once `timeout` ms from now
    // binds extra arguments and passes them to the
    // timer callback
    template<typename... Args>
    timer_id setTimeout(bound_handler_type<Args...> handler,
                        millisec timeout,
                        Args&& ...args);

    // Destroy the specified timer
    //
    // Synchronizes with the worker thread if the
    // callback for this timer is running, which
    // guarantees that the handler for that callback
    // is not running before clearTimer returns
    //
    // You are not required to clear any timers. You can
    // forget their timer_id if you do not need to cancel
    // them.
    //
    // The only time you need this is when you want to
    // stop a timer that has a repetition period, or
    // you want to cancel a timeout that has not fired
    // yet
    //
    // See clear() to wipe out all timers in one go
    //
    bool clearTimer(timer_id id);
    
    
    // returns true if the specified timer is active
    bool exists(timer_id id);

    // Destroy all timers, but preserve id uniqueness
    // This carefully makes sure every timer is not
    // executing its callback before destructing it
    void clear();

    // Peek at current state
    std::size_t size() const noexcept;
    bool empty() const noexcept;

    // Returns lazily initialized singleton
    static TimerThread& global();

private:
    using Lock = std::mutex;
    using ScopedLock = std::unique_lock<Lock>;
    using ConditionVar = std::condition_variable;

    using Clock = std::chrono::steady_clock;
    using Timestamp = std::chrono::time_point<Clock>;
    using Duration = std::chrono::milliseconds;

    struct Timer
    {
        explicit Timer(timer_id id = 0);
        Timer(Timer&& r) noexcept;
        Timer& operator=(Timer&& r) noexcept;

        Timer(timer_id id,
                Timestamp next,
                Duration period,
                handler_type handler) noexcept;

        // Never called
        Timer(Timer const& r) = delete;
        Timer& operator=(Timer const& r) = delete;

        timer_id id;
        Timestamp next;
        Duration period;
        handler_type handler;

        // You must be holding the 'sync' lock to assign waitCond
        std::unique_ptr<ConditionVar> waitCond;

        bool running;
    };

    // Comparison functor to sort the timer "queue" by Timer::next
    struct NextActiveComparator
    {
        bool operator()(Timer const& a, Timer const& b) const noexcept
        {
            return a.next < b.next;
        }
    };

    // Queue is a set of references to Timer objects, sorted by next
    using QueueValue = std::reference_wrapper<Timer>;
    using Queue = std::multiset<QueueValue, NextActiveComparator>;
    using TimerMap = std::unordered_map<timer_id, Timer>;

    void timerThreadWorker();
    bool destroy_impl(ScopedLock& lock,
                      TimerMap::iterator i,
                      bool notify);

    // Inexhaustible source of unique IDs
    timer_id nextId;

    // The Timer objects are physically stored in this map
    TimerMap active;

    // The ordering queue holds references to items in `active`
    Queue queue;

    // One worker thread for an unlimited number of timers is acceptable
    // Lazily started when first timer is started
    // TODO: Implement auto-stopping the timer thread when it is idle for
    // a configurable period.
    mutable Lock sync;
    ConditionVar wakeUp;
    std::thread worker;
    bool done;
    
    timer_id current_clearing_timer;
};

template<typename SRep, typename SPer,
         typename PRep, typename PPer,
         typename... Args>
TimerThread::timer_id TimerThread::addTimer(
        typename std::chrono::duration<SRep, SPer> const& delay,
        typename std::chrono::duration<PRep, PPer> const& period,
        bound_handler_type<Args...> handler,
        Args&& ...args)
{
    millisec msDelay =
            std::chrono::duration_cast<
            std::chrono::milliseconds>(delay).count();

    millisec msPeriod =
            std::chrono::duration_cast<
            std::chrono::milliseconds>(period).count();

    return addTimer(msDelay, msPeriod,
                    std::move(handler),
                    std::forward<Args>(args)...);
}

template<typename... Args>
TimerThread::timer_id TimerThread::addTimer(
        millisec msDelay,
        millisec msPeriod,
        bound_handler_type<Args...> handler,
        Args&& ...args)
{
    return addTimer(msDelay, msPeriod,
                    std::bind(std::move(handler),
                              std::forward<Args>(args)...));
}

// Javascript-like setInterval
template<typename... Args>
TimerThread::timer_id TimerThread::setInterval(
        bound_handler_type<Args...> handler,
        millisec period,
        Args&& ...args)
{
    return setInterval(std::bind(std::move(handler),
                                 std::forward<Args>(args)...),
                       period);
}

// Javascript-like setTimeout
template<typename... Args>
TimerThread::timer_id TimerThread::setTimeout(
        bound_handler_type<Args...> handler,
        millisec timeout,
        Args&& ...args)
{
    return setTimeout(std::bind(std::move(handler),
                                std::forward<Args>(args)...),
                      timeout);
}

#endif // __PAXOS_TIMER_H__
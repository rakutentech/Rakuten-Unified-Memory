/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   paxos_timer.cpp
 * Author: 
 * 
 * Created on 2018/10/03, 11:20
 */

#include "paxos_timer.h"

#include <cassert>

void TimerThread::timerThreadWorker()
{
    ScopedLock lock(sync);

    while (!done)
    {
        if (queue.empty())
        {
            // Wait for done or work
            wakeUp.wait(lock, [this] {
                return done || !queue.empty();
            });
            continue;
        }

        auto queueHead = queue.begin();
        Timer& timer = *queueHead;
        auto now = Clock::now();
        if (now >= timer.next)
        {
            queue.erase(queueHead);

            // Mark it as running to handle racing destroy
            timer.running = true;

            // Call the handler outside the lock
            //printf("DO_CALLBACK\n");
            lock.unlock();
            timer.handler();
            lock.lock();
            //printf("CALLBACK_DONE\n");

            if (timer.running)
            {
                timer.running = false;

                // If it is periodic, schedule a new one
                if (timer.period.count() > 0)
                {
                    timer.next = timer.next + timer.period;
                    queue.emplace(timer);
                } else {
                    // Not rescheduling, destruct it
                    active.erase(timer.id);
                }
            }
            else
            {
                // timer.running changed!
                //
                // Running was set to false, destroy was called
                // for this Timer while the callback was in progress
                // (this thread was not holding the lock during the callback)
                // The thread trying to destroy this timer is waiting on
                // a condition variable, so notify it
                timer.waitCond->notify_all();

                // The clearTimer call expects us to remove the instance
                // when it detects that it is racing with its callback
                active.erase(timer.id);
            }
        } else {
            // Wait until the timer is ready or a timer creation notifies
            wakeUp.wait_until(lock, timer.next);
        }
    }
}

TimerThread::TimerThread()
    : nextId(no_timer + 1)
    , queue()
    , done(false)
{
}

TimerThread::~TimerThread()
{
    ScopedLock lock(sync);

    // The worker might not be running
    if (worker.joinable())
    {
        done = true;
        lock.unlock();
        wakeUp.notify_all();

        // If a timer handler is running, this
        // will make sure it has returned before
        // allowing any deallocations to happen
        worker.join();

        // Note that any timers still in the queue
        // will be destructed properly but they
        // will not be invoked
    }
}

TimerThread::timer_id TimerThread::setInterval(
        handler_type handler, millisec period)
{
    return addTimer(period, period, std::move(handler));
}

TimerThread::timer_id TimerThread::setTimeout(
        handler_type handler, millisec timeout)
{
    return addTimer(timeout, 0, std::move(handler));
}

TimerThread::timer_id TimerThread::addTimer(
        millisec msDelay,
        millisec msPeriod,
        handler_type handler)
{
    ScopedLock lock(sync);

    // Lazily start thread when first timer is requested
    if (!worker.joinable())
        worker = std::thread(&TimerThread::timerThreadWorker, this);

    // Assign an ID and insert it into function storage
    auto id = nextId++;
    auto iter = active.emplace(id, Timer(id,
            Clock::now() + Duration(msDelay),
            Duration(msPeriod),
            std::move(handler)));

    // Insert a reference to the Timer into ordering queue
    Queue::iterator place = queue.emplace(iter.first->second);

    // We need to notify the timer thread only if we inserted
    // this timer into the front of the timer queue
    bool needNotify = (place == queue.begin());

    lock.unlock();

    if (needNotify)
        wakeUp.notify_all();

    return id;
}

bool TimerThread::exists(timer_id id){
    ScopedLock lock(sync);
    return active.count(id);
}

bool TimerThread::clearTimer(timer_id id)
{
    printf("clear timer %ld\n", id);
    ScopedLock lock(sync);
    auto i = active.find(id);
    if(current_clearing_timer == id){
        // we are in the handler
        return true;
    }
    current_clearing_timer = id;
    return destroy_impl(lock, i, true);
}

void TimerThread::clear()
{
    ScopedLock lock(sync);
    while (!active.empty())
    {
        destroy_impl(lock, active.begin(),
                     queue.size() == 1);
    }
}

std::size_t TimerThread::size() const noexcept
{
    ScopedLock lock(sync);
    return active.size();
}

bool TimerThread::empty() const noexcept
{
    ScopedLock lock(sync);
    return active.empty();
}

// NOTE: if notify is true, returns with lock unlocked
bool TimerThread::destroy_impl(ScopedLock& lock,
                               TimerMap::iterator i,
                               bool notify)
{
    assert(lock.owns_lock());

    if (i == active.end())
        return false;

    Timer& timer = i->second;

    if (timer.running)
    {
        // A callback is in progress for this Timer,
        // so flag it for deletion in the worker
        timer.running = false;

        // Assign a condition variable to this timer
        timer.waitCond.reset(new ConditionVar);

        // Block until the callback is finished
        if (std::this_thread::get_id() != worker.get_id()) 
        {
            printf("Timer wait for the callback to end\n");
            timer.waitCond->wait(lock);
            printf("callback ended\n");
        }
    }
    else
    {
        queue.erase(timer);
        active.erase(i);

        if (notify)
        {
            lock.unlock();
            wakeUp.notify_all();
        }
    }

    return true;
}

TimerThread& TimerThread::global()
{
    static TimerThread singleton;
    return singleton;
}

// TimerThread::Timer implementation

TimerThread::Timer::Timer(timer_id id)
    : id(id)
    , running(false)
{
}

TimerThread::Timer::Timer(Timer&& r) noexcept
    : id(std::move(r.id))
    , next(std::move(r.next))
    , period(std::move(r.period))
    , handler(std::move(r.handler))
    , running(std::move(r.running))
{
}

TimerThread::Timer::Timer(timer_id id,
                          Timestamp next,
                          Duration period,
                          handler_type handler) noexcept
    : id(id)
    , next(next)
    , period(period)
    , handler(std::move(handler))
    , running(false)
{
}
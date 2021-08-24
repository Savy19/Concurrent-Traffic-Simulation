#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <vector>
#include <future>
#include <mutex>
#include <memory>
#include "TrafficObject.h"
#include "TrafficLight.h"

// forward declarations to avoid include cycle
class Street;
class Vehicle;

#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <mutex>
#include <algorithm>
#include <queue>



template<class T> 
class MessageQueue
{
public:

    T receive()
    {
        // perform vector modification under the lock
        std::unique_lock<std::mutex> uLock(_mutex);
        _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable

        // remove last vector element from queue
        T message = std::move(_messages.back());
        _messages.pop_back();

        return message; // will not be copied due to return value optimization (RVO) in C++
    }

    void send(T &&msg)
    {
        // simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // perform vector modification under the lock
        std::lock_guard<std::mutex> uLock(_mutex);

        // add vector to queue
        std::cout << "   Message #" << msg << " will be added to the queue" << std::endl;
        _messages.push_back(std::move(msg));
        _cond.notify_one(); // notify client after pushing new Vehicle into vector
    }

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::deque<T> _messages; 
};

int main()
{
    // create monitor object as a shared pointer to enable access by multiple threads
    std::shared_ptr<MessageQueue<int>> queue(new MessageQueue<int>);

    std::cout << "Spawning threads..." << std::endl;
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i)
    {
        int message = i;
        futures.emplace_back(std::async(std::launch::async, &MessageQueue<int>::send, queue, std::move(message)));
    }

    std::cout << "Collecting results..." << std::endl;
    while (true)
    {
        int message = queue->receive();
        std::cout << "   Message #" << message << " has been removed from the queue" << std::endl;
    }

    std::for_each(futures.begin(), futures.end(), [](std::future<void> &ftr) {
        ftr.wait();
    });

    std::cout << "Finished!" << std::endl;

    return 0;
}

// auxiliary class to queue and dequeue waiting vehicles in a thread-safe manner
class WaitingVehicles
{
public:
    // getters / setters
    int getSize();

    // typical behaviour methods
    void pushBack(std::shared_ptr<Vehicle> vehicle, std::promise<void> &&promise);
    void permitEntryToFirstInQueue();

private:
    std::vector<std::shared_ptr<Vehicle>> _vehicles;          // list of all vehicles waiting to enter this intersection
    std::vector<std::promise<void>> _promises; // list of associated promises
    std::mutex _mutex;
};

class Intersection : public TrafficObject
{
public:
    // constructor / desctructor
    Intersection();

    // getters / setters
    void setIsBlocked(bool isBlocked);

    // typical behaviour methods
    void addVehicleToQueue(std::shared_ptr<Vehicle> vehicle);
    void addStreet(std::shared_ptr<Street> street);
    std::vector<std::shared_ptr<Street>> queryStreets(std::shared_ptr<Street> incoming); // return pointer to current list of all outgoing streets
    void simulate();
    void vehicleHasLeft(std::shared_ptr<Vehicle> vehicle);
    bool trafficLightIsGreen();

private:

    // typical behaviour methods
    void processVehicleQueue();

    // private members
    std::vector<std::shared_ptr<Street>> _streets;   // list of all streets connected to this intersection
    WaitingVehicles _waitingVehicles; // list of all vehicles and their associated promises waiting to enter the intersection
    bool _isBlocked;                  // flag indicating wether the intersection is blocked by a vehicle

    TrafficLight _trafficLight;
};

#endif

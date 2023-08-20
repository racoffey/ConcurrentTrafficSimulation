#include <iostream>
#include <random>
#include <chrono>
#include <algorithm>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename TrafficLightPhase>
TrafficLightPhase MessageQueue<TrafficLightPhase>::receive()
//T MessageQueue::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    std::cout << "Message queue receive activated." << std::endl;
    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // When there is something in the queue remove last vector element from queue
    TrafficLightPhase msg = std::move(_queue.back());
    _queue.pop_back();
    std::cout << "Message being returned from queue" << std::endl;
    std::cout << "New length of queue after removing = " << _queue.size() << std::endl;

    // And return it
    return msg;
}

template <typename TrafficLightPhase>
void MessageQueue<TrafficLightPhase>::send(TrafficLightPhase &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::cout << "Traffic Light # message is being added to the queue." << std::endl;
    std::cout << "Message queue size " << &_queue << std::endl;
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    //std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
    _queue.push_back(std::move(msg));
    std::cout << "New length of queue after adding = " <<  std::endl;
    _cond.notify_one(); // notify client after pushing new Message into vector
    
}   


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _type = ObjectType::objectTrafficLight;

    // Initialise message queue
    std::shared_ptr<MessageQueue<TrafficLightPhase>> _messageQueue(new MessageQueue<TrafficLightPhase>());
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    std::cout << "Wait for green is started." << std::endl;

    while (true) 
    {
        int message = _messageQueue->receive();
        if (message == TrafficLightPhase::green) {
            std::cout << "Traffic light #" << _id << " has changed colour to " << message <<  std::endl;
            break;
        }
        
    }
    return;
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{

    std::cout << "Returning current phase." << std::endl;
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public
    // method „simulate“ is called. To do this, use the thread queue in the base class.    

    // Launch cyclethroughPhases function in a thread
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // print id of the current thread
    std::unique_lock<std::mutex> lck(_mutex);
    std::cout << "Traffic Light #" << _id << "::cycleThroughPhases: thread id = " << std::this_thread::get_id() << std::endl;
    lck.unlock();

    // Initialise cycle duration
    _cycleDuration = rand()%(max-min + 1) + min;
    std::cout << "Cycle duration is " << _cycleDuration << std::endl;

    // Start green cycle
    while (true) {
        // Reset cycle timers
        _cycleTimer = std::chrono::system_clock::now();
        std::cout << "Traffic Light #" << _id << " is resetting timer." << std::endl;

        if (_currentPhase == green){
            std::cout << "Traffic Light #" << _id << " is currently green." << std::endl;
        } else {
            std::cout << "Traffic Light #" << _id << " is currently red." << std::endl;
        }
        

        //_currentPhase = green;
        //int randNum = rand()%(max-min + 1) + min;


        // Start red cycle
        while(true) {
            //Check if cycle duration is reached
            _currentDuration = (static_cast<int>((std::chrono::system_clock::now() - _cycleTimer).count()))/1000;
            std::cout << "Traffic Light #" << _id << " current duration is " << _currentDuration << std::endl;
            // If timer is expired then switch colour and break cycle
            if (_currentDuration > _cycleDuration) {
                if (_currentPhase == red) {
                    std::cout << "Traffic Light #" << _id << " is changed to green." << std::endl;
                    _currentPhase = green;
                    break;
                } else {
                    std::cout << "Traffic Light #" << _id << " is changed to red." << std::endl;
                    _currentPhase = red;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // TO DO: Send update method to the message queue
       //_futures.emplace_back(std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _messageQueue, std::move(_currentPhase)));
        std::cout << "Traffic Light #" << _id << " is sending message to queue." << std::endl;
        _futures.emplace_back(std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _messageQueue, std::move(_currentPhase)));
        std::cout << "Traffic Light #" << _id << " has" << _futures.size() << " items in future vector." << std::endl;
    }

    std::cout << "Traffic Light #" << _id << " is waiting to close all threads." << std::endl;
    std::for_each(_futures.begin(), _futures.end(), [](std::future<void> &ftr) {
        ftr.wait();
    });
    std::cout << "Traffic Light #" << _id << " is finnished." << std::endl;
}

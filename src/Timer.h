#ifndef __TIMER_H__
#define __TIMER_H__

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

// class Timer {
// 	std::atomic<bool> active{true};
	
//     public:
//         void setTimeout(auto function, int delay);
//         void setInterval(auto function, int interval);
//         void stop();

// };

// void Timer::setTimeout(auto function, int delay) {
//     active = true;
//     std::thread t([=]() {
//         if(!active.load()) return;
//         std::this_thread::sleep_for(std::chrono::milliseconds(delay));
//         if(!active.load()) return;
//         function();
//     });
//     t.detach();
// }

// void Timer::setInterval(auto function, int interval) {
//     active = true;
//     std::thread t([=]() {
//         while(active.load()) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(interval));
//             if(!active.load()) return;
//             function();
//         }
//     });
//     t.detach();
// }

// void Timer::stop() {
//     active = false;
// }

class Timer {
public:
  std::atomic<bool> active {true};
  template <typename Callable>
  void setTimeout(Callable&& function, int delay) {
    active = true;
    std::thread t([=]() {
      if(!active.load()) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(delay));  
      if(!active.load()) return;
      function();
    });
    t.detach();
  }
template <typename Callable>
  void setInterval(Callable&& function, int interval) {
    active = true;
    std::thread t([=]() {
      while(active.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        if(!active.load()) return;
        function(); 
      }
    });
    t.detach();
  }

  void stop() {
    active = false;
  }

};


#endif // __TIMER_H__
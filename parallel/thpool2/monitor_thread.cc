//------------------------------------------------------------------------------
// Copyright 2019 H2O.ai
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------
/* enable nice() function in unistd.h */
#define _XOPEN_SOURCE
#include <iostream>
#include <unistd.h>
#include "thpool2/monitor_thread.h"
#include "thpool2/thread_worker.h"   // idle_job
#include "utils/exceptions.h"
namespace dt2 {


monitor_thread::monitor_thread(idle_job* wc)
  : controller(wc),
    running(true),
    is_active(false)
{
  thread = std::thread(&monitor_thread::run, this);
}


monitor_thread::~monitor_thread() {
  stop_running();
  if (thread.joinable()) thread.join();
}


void monitor_thread::run() noexcept {
  constexpr auto SLEEP_TIME = std::chrono::milliseconds(20);
  // Reduce this thread's priority to a minimum.
  // See http://man7.org/linux/man-pages/man2/nice.2.html
  int ret = nice(+19);
  if (ret == -1) {
    std::cout << "[errno " << errno << "] "
              << "when setting nice value of monitor thread\n";
  }
  _set_thread_num(size_t(-1));

  std::unique_lock<std::mutex> lock(mutex);
  while (running) {
    // Sleep state
    while (!is_active && running) {
      sleep_state_cv.wait(lock);
    }

    // Wake state
    while (is_active && running) {
      lock.unlock();
      try {
      } catch(...) {
        controller->catch_exception();
      }
      lock.lock();
      sleep_state_cv.wait_for(lock, SLEEP_TIME);
    }
  }
}


void monitor_thread::set_active(bool a) {
  {
    std::lock_guard<std::mutex> lock(mutex);
    is_active = a;
  }
  sleep_state_cv.notify_one();
}


void monitor_thread::stop_running() {
  std::lock_guard<std::mutex> lock(mutex);
  running = false;
  sleep_state_cv.notify_one();
}



}  // namespace dt

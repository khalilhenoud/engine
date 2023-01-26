/**
 * @file framerate_controller.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef FRAMERATE_CONTROLLER_H
#define FRAMERATE_CONTROLLER_H

#include <chrono>
#include <thread>


struct 
framerate_controller {

  void 
  start()
  {
    time_start = std::chrono::high_resolution_clock::now();
  }

  uint64_t 
  end()
  {
    auto previous_framerate = m_current_framerate;

    time_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = time_end - time_start;
    std::chrono::milliseconds in_millisec = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    uint64_t divider = in_millisec.count();
    divider = divider == 0 ? 1 : divider;
    m_current_framerate = 1000ll / divider;

    if (m_locked && m_target_milliseconds > divider)
      std::this_thread::sleep_for(std::chrono::milliseconds(m_target_milliseconds - divider));

    return previous_framerate;
  }

  uint64_t 
  get_framerate()
  {
    return m_current_framerate;
  }

  void 
  lock_framerate(uint64_t target)
  {
    m_locked = true;
    m_target_milliseconds = 1000 / target;
  }

  void 
  unlock_framerate()
  {
    m_locked = false;
  }

  //////////////////////////////////////////////////////////////////////////////
  uint64_t m_target_milliseconds = 0;
  uint64_t m_current_framerate = 0;
  bool m_locked = false;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_start;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_end;
};

#endif

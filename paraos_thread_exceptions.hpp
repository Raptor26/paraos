/// @file paraos_thread_exceptions.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_THREAD_EXCEPTIONS_HPP
#define PARAOS_THREAD_EXCEPTIONS_HPP

#include "paraos_exceptions.hpp"

namespace paraos {
class thread_exception : public paraos::exception {
 public:
  thread_exception(
      string_type reason_, string_type file_name_, numeric_type line_number_)
      : paraos::exception(reason_, file_name_, line_number_) {}
};

class thread_not_created_exception : public paraos::thread_exception {
 public:
  thread_not_created_exception(
      string_type file_name_, numeric_type line_number_)
      : paraos::thread_exception(
            ETL_ERROR_TEXT("Thread: not created", "thread"), file_name_,
            line_number_) {}
};

class thread_no_event_loop_interface_exception
    : public paraos::thread_exception {
 public:
  thread_no_event_loop_interface_exception(
      string_type file_name_, numeric_type line_number_)
      : paraos::thread_exception(
            ETL_ERROR_TEXT("Thread: no event loop interface", "thread"),
            file_name_, line_number_) {}
};
}  // namespace paraos

#endif /* PARAOS_THREAD_EXCEPTIONS_HPP */

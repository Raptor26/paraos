/// @file paraos_exceptions.hpp
/// @author Matvey Simakov <simakov.matvey@mail.ru>
///
/// @brief
///
/// @version 0.1
/// @date 03-02-2025
///
/// @copyright Copyright (c) 2025 StilSoft
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef PARAOS_EXCEPTIONS_HPP
#define PARAOS_EXCEPTIONS_HPP

#include <exception>
#include <string_view>

#include "etl/exception.h"
#include "paraos_attr.h"
#include "paraos_check.h"

namespace paraos {

/// @brief String type for error messages and filepaths where error occurs.
using error_string_type = const char *;

using error_numeric_type = int;

/// @brief Get error text relying on definition of `PARAOS_VERBOSE_ERRORS`
/// macro.
///
/// @param[in] verbose_text: Extended error message.
///
/// @param[in] terse_text: Short error message.
///
/// @return verbose_text if `PARAOS_VERBOSE_ERRORS` is defined, otherwise -
/// terse_text.
inline auto GetErrorText(
    error_string_type verbose_text, error_string_type terse_text)
    -> error_string_type {
#ifdef PARAOS_VERBOSE_ERRORS
  PARAOS_ATTR_UNUSED_VAR(terse_text);
  return verbose_text;
#else
  PARAOS_ATTR_UNUSED_VAR(verbose_text);
  return terse_text;
#endif
}

/// @brief Base class for exceptions in paraos library.
///
/// @note paraos::exception is based on std::exceptions and etl::exception, so
/// it can be caught by reference to `paraos::exception` as well as by reference
/// to `std::exception` and `etl::exception`.
///
/// Intentional multiple inheritance: paraos::exception must be catchable as both
/// std::exception and etl::exception. This is a documented false positive for
/// clang-tidy's misc-multiple-inheritance check.
// NOLINTBEGIN(misc-multiple-inheritance)
class exception : public std::exception, public etl::exception {
 public:
  exception(
      const error_string_type reason, const error_string_type file,
      paraos::error_numeric_type line_number)
      : etl::exception{reason, file, line_number} {}

  ~exception() override = default;

  // NOLINTNEXTLINE(modernize-use-trailing-return-type)
  [[nodiscard]] const char *what() const noexcept override {
    return etl::exception::what();
  }

  exception(const exception &) = default;
  auto operator=(const exception &) -> exception & = default;
  exception(exception &&) = default;
  auto operator=(exception &&) -> exception & = default;
};
// NOLINTEND(misc-multiple-inheritance)
}  // namespace paraos

#endif /* PARAOS_EXCEPTIONS_HPP */

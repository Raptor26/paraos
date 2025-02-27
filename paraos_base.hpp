/// @file paraos_deferred_delete.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
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

#ifndef PARAOS_DEFERRED_DELETE_HPP
#define PARAOS_DEFERRED_DELETE_HPP

namespace paraos {

using base_callback = void (*)();

class Base {
 public:
  explicit Base(base_callback callback_ptr = nullptr)
      : callback_ptr_{callback_ptr} {}

  virtual ~Base() noexcept {
    if (callback_ptr_ != nullptr) {
      callback_ptr_();
    }
  }

  /// @brief Five rule.
  Base(Base &&other) = delete;
  auto operator=(Base &&other) -> Base & = delete;
  auto operator=(const Base &other) -> Base & = delete;
  Base(const Base &other) = delete;

 private:
  base_callback callback_ptr_;
};

}  // namespace paraos

#endif /* PARAOS_DEFERRED_DELETE_HPP */

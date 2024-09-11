/*
 * Copyright (C) 2015  Intel Corporation. All rights reserved.
 *
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PARAOS_FUNCTOR_HPP
#define PARAOS_FUNCTOR_HPP

#include <type_traits>

#define FUNCTOR_TYPEDEF(name, rettype, ...) \
  typedef Functor<rettype, ##__VA_ARGS__> name

#define FUNCTOR_DECLARE(name, rettype, ...) Functor<rettype, ##__VA_ARGS__> name

#define FUNCTOR_BIND(obj, func, rettype, ...) \
  Functor<rettype, ##__VA_ARGS__>::bind<      \
      std::remove_reference<decltype(*obj)>::type, func>(obj)

#define FUNCTOR_BIND_MEMBER(func, rettype, ...) \
  Functor<rettype, ##__VA_ARGS__>::bind<        \
      std::remove_reference<decltype(*this)>::type, func>(this)

template <class RetType, class... Args>
class Functor {
 public:
  constexpr Functor(void *obj, RetType (*method)(void *obj, Args...))
      : obj_(obj), method_(method) {}

  // Allow to construct an empty Functor
  constexpr Functor(decltype(nullptr)) : Functor(nullptr, nullptr) {}

  constexpr Functor() : Functor(nullptr, nullptr) {}

  // Call the method on the obj this Functor is bound to
  RetType operator()(Args... args) const { return method_(obj_, args...); }

  // Compare if the two Functors are calling the same method in the same
  // object
  inline bool operator==(const Functor<RetType, Args...> &rhs) {
    return obj_ == rhs.obj_ && method_ == rhs.method_;
  }
  inline bool operator!=(const Functor<RetType, Args...> &rhs) {
    return obj_ != rhs.obj_ || method_ != rhs.method_;
  }

  // Allow to check if there's a method set in the Functor
  explicit operator bool() const { return method_ != nullptr; }

  template <class T, RetType (T::*method)(Args...)>
  static constexpr Functor bind(T *obj) {
    return {obj, method_wrapper<T, method>};
  }

 private:
  void *obj_;
  RetType (*method_)(void *obj, Args...);

  template <class T, RetType (T::*method)(Args...)>
  static RetType method_wrapper(void *obj, Args... args) {
    T *t = static_cast<T *>(obj);
    return (t->*method)(args...);
  }
};

#endif /* PARAOS_FUNCTOR_HPP */

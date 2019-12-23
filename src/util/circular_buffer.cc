module;

#include <cassert>

export module util.circular_buffer;

import <array>;

export template <typename T, int buffer_size>
class circular_buffer {
 public:
  T& front() { return contents_[front_]; }
  const T& front() const { return contents_[front_]; }

  constexpr bool empty() const { return front_ == back_; }
  constexpr int size() const {
    return front_ <= back_ ? back_ - front_ : buffer_size - front_ + back_;
  }
  constexpr int capacity() const { return buffer_size - 1; }

  T pop() {
    assert(!empty());
    T result = std::move(contents_[front_]);
    front_++;
    if (front_ == buffer_size) front_ -= buffer_size;
    return result;
  }

  template <typename U>
  void push(U&& value) {
    assert(size() < capacity());
    contents_[back_] = std::forward<U>(value);
    back_++;
    if (back_ == buffer_size) back_ -= buffer_size;
  }

 private:
  std::array<T, buffer_size> contents_;
  int front_ = 0, back_ = 0;
};

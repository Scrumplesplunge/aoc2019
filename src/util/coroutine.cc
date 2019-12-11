export module util.coroutine;

import <chrono>;
import <experimental/coroutine>;
import <memory>;
import <optional>;
import <thread>;
import <vector>;

export class executor {
 public:
  using time_point = std::chrono::steady_clock::time_point;
  using task = std::function<void()>;

  virtual void schedule_at(time_point time, task f) = 0;

  void schedule(task f) {
    schedule_at(std::chrono::steady_clock::now(), std::move(f));
  }

  template <typename Rep, typename Period>
  void schedule_in(std::chrono::duration<Rep, Period> duration, task f) {
    schedule_at(std::chrono::steady_clock::now() + duration, std::move(f));
  }

 protected:
  constexpr executor() = default;
  virtual ~executor() = default;
  executor(const executor&) = default;
  executor& operator=(const executor&) = default;
  executor(executor&&) = default;
  executor& operator=(executor&&) = default;
};

export inline thread_local executor* current_executor;

export class [[nodiscard]] set_executor {
 public:
  set_executor(executor* e) : prev_(current_executor) { current_executor = e; }
  ~set_executor() { current_executor = prev_; }
  set_executor(const set_executor&) = delete;
  set_executor& operator=(const set_executor&) = delete;
  set_executor(set_executor&&) = delete;
  set_executor& operator=(set_executor&&) = delete;
 private:
  executor* const prev_;
};

export class series_executor final : public executor {
 public:
  struct work_item {
    time_point time;
    task resume;
    friend constexpr inline bool operator>(const work_item& l,
                                           const work_item& r) {
      return l.time > r.time;
    }
  };

  void schedule_at(time_point time, task f) override {
    work_.push_back({time, std::move(f)});
    std::push_heap(std::begin(work_), std::end(work_), std::greater());
  }

  void run() {
    set_executor set(this);
    while (!work_.empty()) {
      std::pop_heap(std::begin(work_), std::end(work_), std::greater());
      auto item = std::move(work_.back());
      work_.pop_back();
      auto now = std::chrono::steady_clock::now();
      std::this_thread::sleep_for(item.time - now);
      item.resume();
    }
  }

 private:

  // Work items in heap order.
  std::vector<work_item> work_;
};

export class time_awaiter {
 public:
  constexpr time_awaiter(std::chrono::steady_clock::time_point time)
      : time_(time) {}

  bool await_ready() const { return std::chrono::steady_clock::now() >= time_; }

  constexpr void await_resume() const {}

  void await_suspend(std::experimental::coroutine_handle<> handle) {
    current_executor->schedule_at(
        time_, [handle = std::move(handle)]() mutable { handle.resume(); });
  }

 private:
  std::chrono::steady_clock::time_point time_;
};

export constexpr inline auto operator co_await(
    std::chrono::steady_clock::time_point time) {
  return time_awaiter{time};
}

export template <typename Rep, typename Period>
constexpr inline auto operator co_await(
    std::chrono::duration<Rep, Period> duration) {
  return time_awaiter{std::chrono::steady_clock::now() + duration};
}

export template <typename T>
class promise {
 private:
  struct result {
    void wait(std::experimental::coroutine_handle<> handle) {
      if (contents) {
        handle.resume();
      } else {
        reader = std::move(handle);
      }
    }

    std::optional<T> contents;
    std::experimental::coroutine_handle<> reader;
  };

 public:
  constexpr promise() = default;

  // Coroutine await.
  constexpr bool await_ready() const { return result_->contents.has_value(); }
  constexpr T await_resume() const& { return std::move(*result_->contents); }
  void await_suspend(std::experimental::coroutine_handle<> handle) const {
    if (result_->contents) {
      handle.resume();
    } else {
      result_->reader = std::move(handle);
    }
  }

  // Coroutine promise type.
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;
  struct promise_type {
    constexpr auto get_return_object() { return promise{result}; }

    constexpr auto initial_suspend() {
      return std::experimental::suspend_never{};
    }

    constexpr auto final_suspend() {
      return std::experimental::suspend_always{};
    }

    constexpr void unhandled_exception() { std::terminate(); }

    template <typename U,
              typename = std::enable_if_t<std::is_constructible_v<T, U>>>
    constexpr void return_value(U&& value) {
      result->contents.emplace(std::forward<U>(value));
      if (result->reader) result->reader.resume();
    }

    std::shared_ptr<result> result = std::make_shared<promise::result>();
  };

 private:
  explicit promise(std::shared_ptr<result> result)
      : result_(std::move(result)) {}
  std::shared_ptr<result> result_;
};

export template <typename T>
class generator {
 public:
  constexpr generator() = default;
  ~generator() { if (handle_) handle_.destroy(); }

  // Non-copyable.
  generator(const generator&) = delete;
  generator& operator=(const generator&) = delete;

  // Movable.
  constexpr generator(generator&& other)
      : handle_(std::exchange(other.handle_, {})) {}
  constexpr generator& operator=(generator&& other) {
    if (handle_) handle_.destroy();
    handle_ = std::move(other.handle_);
    other.handle_ = {};
    return *this;
  }

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;
  struct promise_type {
    constexpr auto get_return_object() {
      return generator{handle::from_promise(*this)};
    }

    constexpr auto initial_suspend() {
      return std::experimental::suspend_always{};
    }

    constexpr auto final_suspend() {
      return std::experimental::suspend_always{};
    }

    constexpr void unhandled_exception() {
      std::rethrow_exception(std::current_exception());
    }

    template <typename U,
              typename = std::enable_if_t<std::is_constructible_v<T, U>>>
    constexpr auto yield_value(U&& value) {
      result.emplace(std::forward<U>(value));
      return std::experimental::suspend_always{};
    }

    constexpr void return_void() {}

    std::optional<T> result;
  };

  constexpr bool done() { return handle_.done(); }

  constexpr T& value() const {
    assert(!handle_.done());
    return *handle_.promise().result;
  }

  constexpr void next() {
    assert(!handle_.done());
    handle_.resume();
  }

  class sentinel {};

  class iterator {
   public:
    iterator(const iterator&) = delete;
    iterator& operator=(const iterator&) = delete;
    iterator(iterator&& other) : handle_(std::exchange(other.handle_, {})) {}
    iterator& operator=(iterator&& other) {
      if (handle_) handle_.destroy();
      handle_ = std::exchange(other.handle_, {});
    }
    ~iterator() { if (handle_) handle_.destroy(); }
    T& operator*() const { return *handle_.promise().result; }
    T* operator->() const { return &*handle_.promise().result; }
    iterator& operator++() {
      assert(!handle_.done());
      handle_.resume();
      return *this;
    }
   private:
    friend class generator;
    iterator(handle handle) : handle_(std::move(handle)) {}

    friend inline constexpr bool operator!=(const iterator& i, sentinel) {
      return !i.handle_.done();
    }

    handle handle_;
  };

  iterator begin() {
    next();
    return iterator(std::exchange(handle_, {}));
  }

  sentinel end() { return {}; }

 private:
  explicit generator(handle handle) : handle_(std::move(handle)) {}

  handle handle_ = {};
};

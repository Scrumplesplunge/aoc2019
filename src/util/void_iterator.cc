export module util.void_iterator;

export class void_iterator {
 public:
  struct sink {
    template <typename T>
    sink& operator=(T&&) {
      return *this;
    }
  };

  constexpr void_iterator() = default;
  constexpr sink& operator*() { return sink_; }
  constexpr sink* operator->() { return &sink_; }
  constexpr const sink& operator*() const { return sink_; }
  constexpr const sink* operator->() const { return &sink_; }

 private:
  sink sink_;
};

export constexpr void_iterator& operator++(void_iterator& i) { return i; }
export constexpr void_iterator& operator++(void_iterator& i, int) { return i; }

export constexpr bool operator==(void_iterator, void_iterator) {
  return true;
}
export constexpr bool operator!=(void_iterator, void_iterator) {
  return false;
}

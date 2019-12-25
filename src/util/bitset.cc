export module util.bitset;

export template <typename Storage>
struct bitset {
  struct reference {
    bitset* output;
    int index;

    constexpr operator bool() const { return (*(const bitset*)output)[index]; }
    constexpr reference& operator=(bool b) {
      if (b) {
        output->data |= 1u << index;
      } else {
        output->data &= ~(1u << index);
      }
      return *this;
    }
  };

  Storage data;
  constexpr bool operator[](int x) const { return (data >> x) & 1; }
  constexpr reference operator[](int x) { return reference{this, x}; }
  constexpr unsigned long to_ulong() const { return data; };
  constexpr int count() const {
    unsigned temp = data;
    int count = 0;
    while (temp != 0) {
      temp &= temp - 1;
      count++;
    }
    return count;
  }
};

#ifndef MINHEAP_HPP
#define MINHEAP_HPP

#include "vector.hpp"
#include <utility>

template <typename T, typename Comp = std::less<T>> struct MinHeap {
private:
  Vector<T> data;
  Comp cmp;

  size_t parent(size_t i) { return (i - 1) / 2; }
  size_t left(size_t i) { return 2 * i + 1; }
  size_t right(size_t i) { return 2 * i + 2; }

  void siftUp(size_t i) {
    while (i > 0 && cmp(data[i], data[parent(i)])) {
      std::swap(data[i], data[parent(i)]);
      i = parent(i);
    }
  }

  void siftDown(size_t i) {
    size_t n = data.size();
    while (true) {
      size_t smallest = i;
      size_t l = left(i);
      size_t r = right(i);

      if (l < n && cmp(data[l], data[smallest]))
        smallest = l;
      if (r < n && cmp(data[r], data[smallest]))
        smallest = r;

      if (smallest == i)
        break;

      std::swap(data[i], data[smallest]);
      i = smallest;
    }
  }

public:
  void push(const T &val) {
    data.push_back(val);
    siftUp(data.size() - 1);
  }

  T pop() {
    T top = data[0];
    data[0] = data[data.size() - 1];
    data.pop_back();
    if (!data.empty())
      siftDown(0);
    return top;
  }

  const T &top() const { return data[0]; }

  bool empty() const { return data.empty(); }
  size_t size() const { return data.size(); }
};

#endif // MINHEAP_HPP
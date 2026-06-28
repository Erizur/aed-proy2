#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T> class Vector {
  T *data;
  size_t big_size;
  size_t cur_size;

public:
  using iterator = T *;
  using const_iterator = const T *;

  Vector<T>() {
    big_size = 2;
    cur_size = 0;
    data = new T[big_size];
  };

  Vector<T>(const Vector<T> &vec) {
    big_size = vec.capacity();
    cur_size = vec.size();
    data = new T[big_size];

    for (size_t i = 0; i < cur_size; i++) {
      data[i] = vec[i];
    }
  };

  Vector<T>(Vector<T> &&vec) noexcept : Vector<T>() {
    std::swap(data, vec.data);
    std::swap(big_size, vec.big_size);
    std::swap(cur_size, vec.cur_size);
  }

  ~Vector<T>() {
    delete[] std::exchange(data, nullptr);
  };

  void resize(size_t a) {
    if (a < big_size)
      return;
    big_size = a;
    T *new_data = new T[big_size];

    for (size_t i = 0; i < cur_size; i++) {
      new_data[i] = data[i];
    }

    delete[] data;
    data = new_data;
  };

  void push_back(T elm) {
    while (cur_size >= big_size)
      resize(big_size * 2);
    data[cur_size] = elm;
    cur_size++;
  };

  T pop_back() {
    if (cur_size == 0)
      throw std::out_of_range("Cannot pop_back from an empty Vector");

    T d = data[cur_size - 1];
    cur_size--;
    return d;
  }

  void insert(size_t index, T elm) {
    while (cur_size >= big_size)
      resize(big_size * 2);

    for (size_t i = cur_size; i > index; i--) {
      data[i] = data[i - 1];
    }

    data[index] = elm;
    cur_size++;
  }

  void erase(size_t index) {
    if (index >= cur_size)
      throw std::out_of_range("Vector index out of bounds");

    for (size_t i = index + 1; i < cur_size; i++) {
      data[i - 1] = data[i];
    }

    cur_size--;
  }

  T operator[](size_t idx) const {
    if (idx >= cur_size)
      throw std::out_of_range("Vector index out of bounds");
    return data[idx];
  }

  T& operator[](size_t idx) {
    if (idx >= cur_size)
      throw std::out_of_range("Vector index out of bounds");
    return data[idx];
  }

  Vector<T> &operator=(const Vector<T> &vec) {
    while (vec.size() >= big_size)
      resize(big_size * 2);

    cur_size = vec.size();
    for (size_t i = 0; i < cur_size; i++) {
      data[i] = vec[i];
    }
    return *this;
  }

  Vector<T> &operator=(Vector<T> &&vec) noexcept {
    std::swap(data, vec.data);
    std::swap(big_size, vec.big_size);
    std::swap(cur_size, vec.cur_size);
    return *this;
  }

  [[nodiscard]] const size_t capacity() const { return big_size; };
  [[nodiscard]] const size_t size() const { return cur_size; };

  [[nodiscard]] const T front() const { return data[0]; };
  [[nodiscard]] const T back() const { return data[cur_size - 1]; };

  [[nodiscard]] const bool empty() const { return cur_size < 1; };

  [[nodiscard]] iterator begin() { return data; }
  [[nodiscard]] const_iterator begin() const { return data; }

  [[nodiscard]] iterator end() { return data + cur_size; }
  [[nodiscard]] const_iterator end() const { return data + cur_size; }
};

#endif // VECTOR_HPP

#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <stdexcept>

template <typename T> struct QueueNode {
  T val;
  QueueNode<T> *next;
  QueueNode(T data) : val(data), next(nullptr){};
};

template <typename T> struct Queue {
protected:
  QueueNode<T> *first, *last;

public:
  Queue() : first(nullptr), last(nullptr) {};

  ~Queue() {
    while (first) {
      QueueNode<T> *next = first->next;
      delete first;
      first = next;
    }
  }

  Queue(const Queue &other) : first(nullptr), last(nullptr) {
    QueueNode<T> *src = other.first;
    while (src) {
      enqueue(src->val);
      src = src->next;
    }
  }

  void enqueue(T d) {
    if (empty()) {
      first = last = new QueueNode<T>(d);
    } else {
      QueueNode<T> *t = new QueueNode<T>(d);
      last->next = t;
      last = t;
    }
  }

  T dequeue() {
    if (empty())
      throw std::runtime_error("dequeue from empty queue");

    T v = first->val;
    if (first == last) {
      delete first;
      first = last = nullptr;
    } else {
      QueueNode<T> *p = first;
      first = p->next;
      delete p;
    }
    return v;
  }

  T front() const {
    if (!first)
      throw std::runtime_error("invalid get from empty queue");
    return first->val;
  }

  T back() const {
    if (!last)
      throw std::runtime_error("invalid get from empty queue");
    return last->val;
  }

  Queue &operator=(const Queue &other) {
    if (this == &other)
      return *this;
    Queue tmp(other);
    std::swap(first, tmp.first);
    std::swap(last, tmp.last);
    return *this;
  }

  bool empty() const { return first == nullptr && last == nullptr; }
};

#endif // QUEUE_HPP

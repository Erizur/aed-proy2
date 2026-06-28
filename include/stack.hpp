#ifndef STACK_HPP
#define STACK_HPP

#include <stdexcept>

template <typename T> struct StackNode {
  T val;
  StackNode *next;
  StackNode(T data) : val(data), next(nullptr) {};
};

template <typename T> struct Stack {
protected:
  StackNode<T> *topNode;

public:
  Stack() : topNode(nullptr) {};

  Stack(const Stack &other) : topNode(nullptr) {
    if (other.topNode == nullptr)
      return;

    topNode = new StackNode<T>(other.topNode->val);
    StackNode<T> *src = other.topNode->next;
    StackNode<T> *dst = topNode;
    while (src) {
      dst->next = new StackNode<T>(src->val);
      dst = dst->next;
      src = src->next;
    }
  }

  ~Stack() {
    while (topNode) {
      StackNode<T> *next = topNode->next;
      delete topNode;
      topNode = next;
    }
  }

  void push(T d) {
    StackNode<T> *c = new StackNode<T>(d);
    c->next = topNode;
    topNode = c;
  }

  T pop() {
    StackNode<T> *c = topNode;
    if (!c)
      throw std::runtime_error("invalid pop from empty stack");
    topNode = c->next;
    T val = c->val;
    delete c;
    return val;
  }

  T top() const {
    StackNode<T> *c = topNode;
    if (!c)
      throw std::runtime_error("invalid get from empty stack");
    return c->val;
  }

  Stack &operator=(const Stack &other) {
    if (this == &other)
      return *this;
    Stack tmp(other);
    std::swap(topNode, tmp.topNode);
    return *this;
  }

  bool empty() const { return topNode == nullptr; }
};

#endif // STACK_HPP

#ifndef DOLIST_HPP
#define DOLIST_HPP

#include <cstddef>
#include <utility>

template <typename T> struct DoNode {
  T data;
  DoNode *next = nullptr;
  DoNode *prev = nullptr;

  DoNode(T d) { data = d; }
};

template <typename T> struct DoIterator {
  DoNode<T> *node;
  DoIterator<T>(DoNode<T> *n) : node(n) {}

  T &operator*() { return node->data; }
  DoIterator<T> &operator++() {
    if (node)
      node = node->next;
    return *this;
  }
  DoIterator operator++(int) {
    DoIterator temp = *this;
    ++(*this);
    return temp;
  }

  DoIterator &operator--() {
    if (node)
      node = node->prev;
    return *this;
  }

  bool operator!=(const DoIterator<T> &other) const {
    return node != other.node;
  }
  bool operator==(const DoIterator<T> &other) const {
    return node == other.node;
  }
};

template <typename T> struct DoList {
protected:
  size_t count = 0;

public:
  using iterator = DoIterator<T>;
  using const_iterator = const DoIterator<T>;

  DoNode<T> *head = nullptr;
  DoNode<T> *tail = nullptr;

  // C++ seems to complain about this...
  DoList<T>() = default;

  DoList<T>(DoList<T> &&list) noexcept : DoList<T>() {
    std::swap(head, list.head);
    std::swap(tail, list.tail);
    std::swap(count, list.count);
  }

  DoList(const DoList<T> &list) : head(nullptr), tail(nullptr), count(0) {
    DoNode<T> *cur = list.head;
    while (cur) {
      push_back(cur->data);
      cur = cur->next;
    }
  }

  ~DoList<T>() { clear(); }

  [[nodiscard]] size_t size() const { return count; };
  [[nodiscard]] bool empty() const { return count < 1; };
  [[nodiscard]] T front() { return head->data; };
  [[nodiscard]] T back() { return tail->data; };

  [[nodiscard]] iterator begin() { return iterator(head); };
  [[nodiscard]] const_iterator begin() const { return iterator(head); };

  [[nodiscard]] iterator end() { return iterator(nullptr); };
  [[nodiscard]] const_iterator end() const { return iterator(nullptr); };

  void push_back(T data) {
    DoNode<T> *temp = new DoNode<T>(data);
    if (empty()) {
      tail = head = temp;
    } else {
      temp->prev = tail;
      tail->next = temp;
      tail = temp;
    }
    count++;
  }

  void pop_back() {
    if (empty())
      return;

    DoNode<T> *temp = tail;
    if (tail == head) {
      tail = head = nullptr;
    } else {
      tail = tail->prev;
      tail->next = nullptr;
    }
    delete temp;
    count--;
  }

  void push_front(T data) {
    DoNode<T> *temp = new DoNode<T>(data);
    if (empty()) {
      head = tail = temp;
    } else {
      temp->next = head;
      head->prev = temp;
      head = temp;
    }
    count++;
  }

  void pop_front() {
    if (empty())
      return;

    DoNode<T> *temp = head;
    if (head == tail) {
      head = tail = nullptr;
    } else {
      head = head->next;
      head->prev = nullptr;
    }
    delete temp;
    count--;
  }

  void insert(T data, size_t pos) {
    if (pos > count || pos < 1)
      return;
    if (pos == 1) {
      push_front(data);
      return;
    }

    if (pos == count + 1) {
      push_back(data);
      return;
    }

    size_t idx = 1;
    DoNode<T> *cur = head;
    while (idx < pos) {
      cur = cur->next;
      idx++;
    }

    DoNode<T> *temp = new DoNode<T>(data);
    temp->next = cur;
    temp->prev = cur->prev;
    cur->prev->next = temp;
    cur->prev = temp;
    count++;
  }

  void erase(size_t pos) {
    if (pos > count || pos < 1)
      return;
    if (pos == 1) {
      pop_front();
      return;
    }

    if (pos == count) {
      pop_back();
      return;
    }

    size_t idx = 1;
    DoNode<T> *cur = head;
    while (idx < pos) {
      cur = cur->next;
      idx++;
    }

    cur->prev->next = cur->next;
    cur->next->prev = cur->prev;
    delete cur;
    count--;
  }

  void clear() {
    while (tail != nullptr) {
      DoNode<T> *temp = tail;
      if (head == tail) {
        head = tail = nullptr;
      } else {
        tail = tail->prev;
      }
      delete temp;
    }
    count = 0;
  }

  DoList<T> &operator=(const DoList<T> &other) {
    if (this != &other) {
      DoList<T> temp(other);
      std::swap(head, temp.head);
      std::swap(tail, temp.tail);
      std::swap(count, temp.count);
    }
    return *this;
  }

  DoList<T> &operator=(DoList<T> &&list) noexcept {
    clear();
    std::swap(head, list.head);
    std::swap(tail, list.tail);
    std::swap(count, list.count);
    return *this;
  }
};

#endif // DOLIST_HPP
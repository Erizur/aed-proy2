#include "catch.hpp"
#include "queue.hpp"

TEST_CASE("Queue: enqueue y front/back", "[queue]") {
  Queue<int> q;
  q.enqueue(10);
  q.enqueue(20);
  q.enqueue(30);

  REQUIRE(q.front() == 10);
  REQUIRE(q.back() == 30);
}

TEST_CASE("Queue: dequeue (FIFO)", "[queue]") {
  Queue<int> q;
  q.enqueue(1);
  q.enqueue(2);
  q.enqueue(3);

  REQUIRE(q.dequeue() == 1);
  REQUIRE(q.dequeue() == 2);
  REQUIRE(q.dequeue() == 3);
  REQUIRE(q.empty());
}

TEST_CASE("Queue: dequeue vacío lanza excepción", "[queue]") {
  Queue<int> q;
  REQUIRE_THROWS(q.dequeue());
}

TEST_CASE("Queue: front/back vacío lanza excepción", "[queue]") {
  Queue<int> q;
  REQUIRE_THROWS(q.front());
  REQUIRE_THROWS(q.back());
}

TEST_CASE("Queue: empty", "[queue]") {
  Queue<int> q;
  REQUIRE(q.empty());
  q.enqueue(1);
  REQUIRE_FALSE(q.empty());
  q.dequeue();
  REQUIRE(q.empty());
}

TEST_CASE("Queue: constructor de copia", "[queue]") {
  Queue<int> original;
  original.enqueue(1);
  original.enqueue(2);
  original.enqueue(3);

  Queue<int> copy(original);
  REQUIRE(copy.front() == 1);
  REQUIRE(copy.back() == 3);

  copy.dequeue();
  REQUIRE(original.front() == 1);
  REQUIRE(copy.front() == 2);
}

TEST_CASE("Queue: operador de asignación", "[queue]") {
  Queue<int> a;
  a.enqueue(10);
  a.enqueue(20);

  Queue<int> b;
  b = a;
  REQUIRE(b.front() == 10);
  REQUIRE(b.back() == 20);
  b.dequeue();
  REQUIRE(a.front() == 10);
}

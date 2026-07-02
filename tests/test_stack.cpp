#include "catch.hpp"
#include "stack.hpp"

TEST_CASE("Stack: push y top", "[stack]") {
  Stack<int> stack;
  stack.push(10);
  stack.push(20);
  stack.push(30);

  REQUIRE(stack.top() == 30);
}

TEST_CASE("Stack: pop", "[stack]") {
  Stack<int> stack;
  stack.push(1);
  stack.push(2);
  stack.push(3);

  REQUIRE(stack.pop() == 3);
  REQUIRE(stack.pop() == 2);
  REQUIRE(stack.pop() == 1);
  REQUIRE(stack.empty());
}

TEST_CASE("Stack: pop vacío lanza excepción", "[stack]") {
  Stack<int> stack;
  REQUIRE_THROWS(stack.pop());
}

TEST_CASE("Stack: top vacío lanza excepción", "[stack]") {
  Stack<int> stack;
  REQUIRE_THROWS(stack.top());
}

TEST_CASE("Stack: empty", "[stack]") {
  Stack<int> stack;
  REQUIRE(stack.empty());
  stack.push(1);
  REQUIRE_FALSE(stack.empty());
  stack.pop();
  REQUIRE(stack.empty());
}

TEST_CASE("Stack: constructor de copia", "[stack]") {
  Stack<int> original;
  original.push(1);
  original.push(2);
  original.push(3);

  Stack<int> copy(original);
  REQUIRE(copy.top() == 3);
  copy.pop();
  REQUIRE(copy.top() == 2);
  REQUIRE(original.top() == 3);
}

TEST_CASE("Stack: operador de asignación", "[stack]") {
  Stack<int> a;
  a.push(10);
  a.push(20);

  Stack<int> b;
  b = a;
  REQUIRE(b.top() == 20);
  b.pop();
  REQUIRE(a.top() == 20);
}

#include "catch.hpp"
#include "vector.hpp"

TEST_CASE("Vector: push_back y acceso", "[vector]") {
  Vector<int> vec;

  vec.push_back(10);
  vec.push_back(20);
  vec.push_back(30);

  REQUIRE(vec.size() == 3);
  REQUIRE(vec[0] == 10);
  REQUIRE(vec[1] == 20);
  REQUIRE(vec[2] == 30);
}

TEST_CASE("Vector: pop_back", "[vector]") {
  Vector<int> vec;
  vec.push_back(1);
  vec.push_back(2);

  int v = vec.pop_back();
  REQUIRE(v == 2);
  REQUIRE(vec.size() == 1);
}

TEST_CASE("Vector: pop_back vacío lanza excepción", "[vector]") {
  Vector<int> vec;
  REQUIRE_THROWS_AS(vec.pop_back(), std::out_of_range);
}

TEST_CASE("Vector: insert y erase", "[vector]") {
  Vector<int> vec;
  vec.push_back(1);
  vec.push_back(3);

  SECTION("insert en medio") {
    vec.insert(1, 2);
    REQUIRE(vec.size() == 3);
    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
  }

  SECTION("erase") {
    vec.erase(0);
    REQUIRE(vec.size() == 1);
    REQUIRE(vec[0] == 3);
  }

  SECTION("erase fuera de rango") {
    REQUIRE_THROWS_AS(vec.erase(5), std::out_of_range);
  }
}

TEST_CASE("Vector: front, back, empty", "[vector]") {
  Vector<int> vec;
  REQUIRE(vec.empty());

  vec.push_back(42);
  vec.push_back(99);
  REQUIRE_FALSE(vec.empty());
  REQUIRE(vec.front() == 42);
  REQUIRE(vec.back() == 99);
}

TEST_CASE("Vector: resize automático", "[vector]") {
  Vector<int> vec;
  for (int i = 0; i < 100; i++) {
    vec.push_back(i);
  }
  REQUIRE(vec.size() == 100);
  REQUIRE(vec[99] == 99);
  REQUIRE(vec.capacity() >= 100);
}

TEST_CASE("Vector: constructor de copia", "[vector]") {
  Vector<int> original;
  original.push_back(1);
  original.push_back(2);
  original.push_back(3);

  Vector<int> copy(original);
  REQUIRE(copy.size() == 3);
  REQUIRE(copy[0] == 1);
  REQUIRE(copy[2] == 3);
}

TEST_CASE("Vector: iteradores", "[vector]") {
  Vector<int> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  int sum = 0;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    sum += *it;
  }
  REQUIRE(sum == 6);
}

TEST_CASE("Vector: operador[] fuera de rango", "[vector]") {
  Vector<int> vec;
  vec.push_back(1);
  REQUIRE_THROWS_AS(vec[5], std::out_of_range);
}

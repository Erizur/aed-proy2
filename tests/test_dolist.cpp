#include "catch.hpp"
#include "dolist.hpp"

TEST_CASE("DoList: push_back y push_front", "[dolist]") {
  DoList<int> list;

  SECTION("push_back agrega al final") {
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    REQUIRE(list.size() == 3);
    REQUIRE(list.front() == 1);
    REQUIRE(list.back() == 3);
  }

  SECTION("push_front agrega al inicio") {
    list.push_front(3);
    list.push_front(2);
    list.push_front(1);
    REQUIRE(list.size() == 3);
    REQUIRE(list.front() == 1);
    REQUIRE(list.back() == 3);
  }
}

TEST_CASE("DoList: pop_back y pop_front", "[dolist]") {
  DoList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  SECTION("pop_back") {
    list.pop_back();
    REQUIRE(list.size() == 2);
    REQUIRE(list.back() == 2);
  }

  SECTION("pop_front") {
    list.pop_front();
    REQUIRE(list.size() == 2);
    REQUIRE(list.front() == 2);
  }

  SECTION("pop hasta vaciar") {
    list.pop_back();
    list.pop_back();
    list.pop_back();
    REQUIRE(list.empty());
    list.pop_back();
    REQUIRE(list.empty());
  }
}

TEST_CASE("DoList: insert y erase por posición", "[dolist]") {
  DoList<int> list;
  list.push_back(10);
  list.push_back(30);

  SECTION("insert en posición 2") {
    list.insert(20, 2);
    REQUIRE(list.size() == 3);
    auto it = list.begin();
    REQUIRE(*it == 10); ++it;
    REQUIRE(*it == 20); ++it;
    REQUIRE(*it == 30);
  }

  SECTION("erase en posición 1") {
    list.erase(1);
    REQUIRE(list.size() == 1);
    REQUIRE(list.front() == 30);
  }
}

TEST_CASE("DoList: iteradores", "[dolist]") {
  DoList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  int sum = 0;
  for (auto &val : list) sum += val;
  REQUIRE(sum == 6);
}

TEST_CASE("DoList: constructor de copia", "[dolist]") {
  DoList<int> original;
  original.push_back(1);
  original.push_back(2);
  original.push_back(3);

  DoList<int> copy(original);
  REQUIRE(copy.size() == 3);
  REQUIRE(copy.front() == 1);
  REQUIRE(copy.back() == 3);

  copy.pop_back();
  REQUIRE(original.size() == 3);
  REQUIRE(copy.size() == 2);
}

TEST_CASE("DoList: move constructor", "[dolist]") {
  DoList<int> original;
  original.push_back(1);
  original.push_back(2);

  DoList<int> moved(std::move(original));
  REQUIRE(moved.size() == 2);
  REQUIRE(moved.front() == 1);
}

TEST_CASE("DoList: clear", "[dolist]") {
  DoList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.clear();
  REQUIRE(list.empty());
  REQUIRE(list.size() == 0);
}

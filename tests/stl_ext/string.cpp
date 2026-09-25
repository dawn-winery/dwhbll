#include <dwhbll/macros/testing.h>

import dwhbll.stl_ext;
import dwhbll.testing;
import std;

using namespace dwhbll::test;
using namespace dwhbll::stl_ext;

namespace string {

[[=test]]
[[=name("escape_non_printable")]]
void test_escape_non_printable()
{
    REQUIRE_EQ(escape_non_printable("hello world"), "hello world");
    REQUIRE_EQ(escape_non_printable("hello\nworld"), "hello\\nworld");
    REQUIRE_EQ(escape_non_printable("hello\rworld"), "hello\\rworld");
    REQUIRE_EQ(escape_non_printable("hello\tworld"), "hello\\x9world");
    REQUIRE_EQ(escape_non_printable(std::string("\0", 1)), "\\x0");
}

[[=test]]
[[=name("replace_all")]]
void test_replace_all()
{
    REQUIRE_EQ(replace_all("hello world", "world", "there"), "hello there");
    REQUIRE_EQ(replace_all("aaa", "a", "b"), "bbb");
    REQUIRE_EQ(replace_all("foo foo foo", "foo", "bar"), "bar bar bar");

    REQUIRE_EQ(replace_all("hello", "x", "y"), "hello");
    REQUIRE_EQ(replace_all("", "x", "y"), "");
    REQUIRE_EQ(replace_all("hello", "", "x"), "xhxexlxlxox");
}

[[=test]]
[[=name("split")]]
void test_split()
{
    REQUIRE_EQ(split("a,b,c", ","), (std::vector<std::string>{"a", "b", "c"}));
    REQUIRE_EQ(split("a--b--c", "--"), (std::vector<std::string>{"a", "b", "c"}));

    REQUIRE_EQ(split(",a,b,", ","), (std::vector<std::string>{"", "a", "b", ""}));
    REQUIRE_EQ(split("a,,b", ","), (std::vector<std::string>{"a", "", "b"}));
    REQUIRE_EQ(split("", ","), std::vector<std::string>{""});

    REQUIRE_EQ(split("hello", ""), std::vector<std::string>{"hello"});
}

[[=test]]
[[=name("escape_string")]]
void test_escape_string()
{
    REQUIRE_EQ(escape_string("\"\\\b\f\n\r\t"), "\\\"\\\\\\b\\f\\n\\r\\t");

    REQUIRE_EQ(escape_string(std::string("\x01\x1f", 2)), "\\u0001\\u001f");
    REQUIRE_EQ(escape_string("hello world"), "hello world");
}

[[=test]]
[[=name("match_glob")]]
void test_match_glob()
{
    REQUIRE(match_glob("hello", "hello"));
    REQUIRE(!match_glob("hello", "world"));

    REQUIRE(match_glob("hello", "h?llo"));
    REQUIRE(!match_glob("hello", "h?ll"));

    REQUIRE(match_glob("hello", "*"));
    REQUIRE(match_glob("hello", "he*"));
    REQUIRE(match_glob("hello", "*llo"));
    REQUIRE(match_glob("hello", "h*o"));
    REQUIRE(match_glob("hello", "h*l*o"));

    REQUIRE(!match_glob("hello", "he?"));
    REQUIRE(!match_glob("hello", "world*"));
    REQUIRE(!match_glob("", "a*"));

    REQUIRE(match_glob("", ""));
    REQUIRE(match_glob("", "*"));
    REQUIRE(!match_glob("", "?"));
}

[[=test]]
[[=name("matches_patterns")]]
void test_matches_patterns()
{
    REQUIRE(matches_patterns("hello", {}));

    REQUIRE(matches_patterns("hello world", {"world"}));
    REQUIRE(matches_patterns("hello world", {"foo", "world"}));
    REQUIRE(!matches_patterns("hello world", {"foo", "bar"}));

    REQUIRE(matches_patterns("hello.cpp", {"*.cpp"}));
    REQUIRE(matches_patterns("hello.cpp", {"*.h", "*.cpp"}));
    REQUIRE(!matches_patterns("hello.cpp", {"*.h"}));

    REQUIRE(!matches_patterns("hello.cpp", {"hello.?"}));
    REQUIRE(matches_patterns("hello.cpp", {"hello.?pp"}));

    REQUIRE(matches_patterns("some/path/foo.cpp", {"foo"}));
    REQUIRE(!matches_patterns("some/path/foo.cpp", {"bar"}));
}

}

TEST_REGISTER_FILE();

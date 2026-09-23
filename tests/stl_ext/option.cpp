#include <dwhbll/stl_ext/option.h>
#include <dwhbll/stl_ext/result.h>
#include <dwhbll/testing/testing.h>

using namespace dwhbll::stl_ext;
using namespace dwhbll::test;

namespace stl_ext::option {

[[=test]]
void init_and_set() {
    auto opt = Option<bool>();
    REQUIRE(opt.is_none());
    REQUIRE_FALSE(opt.is_some());

    opt = true;

    REQUIRE_FALSE(opt.is_none());
    REQUIRE(opt.is_some());
    REQUIRE(opt.unwrap());

    opt = None;

    REQUIRE(opt.is_none());
    REQUIRE_FALSE(opt.is_some());
}

[[=test]]
void init_with_value() {
    auto opt = Option<uint32_t>(0xDEADBEEF);

    REQUIRE(opt.is_some());
    REQUIRE_FALSE(opt.is_none());
    REQUIRE(opt.unwrap() == 0xDEADBEEF);
}

[[=test]]
void init_with_none() {
    auto opt = Option<int>(None);

    REQUIRE_FALSE(opt.is_some());
    REQUIRE(opt.is_none());
}

[[=test]]
void copy_other() {
    auto opt1a = Option<std::string>();
    auto opt1b = Option<std::string>("Steaven");
    auto opt2a = Option(opt1a);
    auto opt2b = Option(opt1b);

    REQUIRE_EQ(opt1a, opt2a);
    REQUIRE_EQ(opt1b, opt2b);
    REQUIRE(opt2a.is_none());
    REQUIRE(opt2b.is_some());
    REQUIRE(opt2b.unwrap() == "Steaven");
}

[[=test]]
void set_other() {
    auto opt1 = Option<float>(123.f);
    auto opt2 = Option<float>();

    REQUIRE(opt2.is_none());

    opt2 = opt1;

    REQUIRE(opt2.is_some());
    REQUIRE(opt2.unwrap() == 123.f);
    REQUIRE_EQ(opt1, opt2);
}

[[=test]]
void is_some_and() {
    auto opt1 = Option<std::string>();
    auto opt2 = Option<std::string>("canonical hole");

    auto longer_than_5 = [](std::string val) {
        return val.size() > 5;
    };

    auto shorter_than_5 = [](std::string val) {
        return val.size() < 5;
    };

    auto res1 = opt1.is_some_and(longer_than_5);
    auto res2 = opt2.is_some_and(longer_than_5);
    auto res3 = opt2.is_some_and(shorter_than_5);

    REQUIRE_FALSE(res1);
    REQUIRE(res2);
    REQUIRE_FALSE(res3);
}

[[=test]]
void is_none_or() {
    auto opt1 = Option<std::string>();
    auto opt2 = Option<std::string>("canonical hole");

    auto longer_than_5 = [](std::string val) {
        return val.size() > 5;
    };

    auto shorter_than_5 = [](std::string val) {
        return val.size() < 5;
    };

    auto res1 = opt1.is_none_or(longer_than_5);
    auto res2 = opt2.is_none_or(longer_than_5);
    auto res3 = opt2.is_none_or(shorter_than_5);

    REQUIRE(res1);
    REQUIRE(res2);
    REQUIRE_FALSE(res3);
}

[[=test]]
void unwrap_or() {
    auto opt1 = Option<uint16_t>();
    auto opt2 = Option<uint16_t>(727);

    auto res1 = opt1.unwrap_or(272);
    auto res2 = opt2.unwrap_or(272);

    REQUIRE_EQ(res1, 272);
    REQUIRE_EQ(res2, 727);
}

[[=test]]
void unwrap_or_else() {
    auto opt1 = Option<int>();
    auto opt2 = Option<int>(-0xFFFF);

    auto some_math = [] () {
        return 696969;
    };

    auto res1 = opt1.unwrap_or_else(some_math);
    auto res2 = opt2.unwrap_or_else(some_math);

    REQUIRE_EQ(res1, 696969);
    REQUIRE_EQ(res2, -0xFFFF);
}

[[=test]]
void unwrap_or_default() {
    auto opt1 = Option<std::string>();
    auto opt2 = Option<std::string>(":xdd:");
    auto opt3 = Option<Option<bool>>();
    auto opt4 = Option<Option<bool>>(true);

    REQUIRE(opt1.unwrap_or_default() == std::string{});
    REQUIRE(opt2.unwrap_or_default() == ":xdd:");
    REQUIRE(opt3.unwrap_or_default() == Option<bool>{});
    REQUIRE(opt3.unwrap_or_default().unwrap_or_default() == bool{});
    REQUIRE(opt4.unwrap_or_default() == Option<bool>(true));
    REQUIRE(opt4.unwrap_or_default().unwrap_or_default());
}

[[=test]]
void unwrap_unchecked() {
    auto opt = Option<std::string>("Present Day, Present Time");
    REQUIRE(opt.unwrap_unchecked() == "Present Day, Present Time");
}

[[=test]]
void map() {
    struct Amogus {
        std::string imposter;
        bool sus;
    };

    auto toggle_sus = [] (Amogus amogus) -> Amogus {
        amogus.sus = !amogus.sus;
        return amogus;
    };

    auto opt1 = Option<Amogus>();
    auto opt2 = Option<Amogus>(Amogus{"John", false});

    auto res1 = opt1.map(toggle_sus);
    auto res2 = opt2.map(toggle_sus);

    REQUIRE(res1.is_none());
    REQUIRE(res2.is_some());
    REQUIRE_EQ(res2.unwrap().imposter, "John");
    REQUIRE(res2.unwrap().sus);
}

[[=test]]
void inspect() {
    auto opt1 = Option<double>();
    auto opt2 = Option<double>(123123123.123f);
    auto m = 0;

    auto grab_val = [&m] (double val) {
        m = val;
    };

    auto res1 = opt1.inspect(grab_val);
    auto res2 = opt2.inspect(grab_val);
    REQUIRE_EQ(opt1, res1);
    REQUIRE_EQ(opt2, res2);
    REQUIRE_EQ(m, res2.unwrap());
}

[[=test]]
void map_or() {
    auto opt1 = Option<char>();
    auto opt2 = Option<char>('3');

    const auto sad = ":(";
    const auto add_eyes = [] (char c) {
        return std::format(":{}", c);
    };

    auto res1 = opt1.map_or(sad, add_eyes);
    auto res2 = opt2.map_or(sad, add_eyes);

    REQUIRE_EQ(res1, ":(");
    REQUIRE_EQ(res2, ":3");
}

[[=test]]
void map_or_else() {
    auto opt1 = Option<std::pair<uint8_t, uint8_t>>();
    auto opt2 = Option<std::pair<uint8_t, uint8_t>>(std::make_pair(240, 60));

    auto get_pair = [] ()-> std::pair<uint8_t, uint8_t> {
        return std::pair<uint8_t, uint8_t>(0, 0);
    };
    auto swap_pair = [] (std::pair<uint8_t, uint8_t> pair) -> std::pair<uint8_t, uint8_t> {
        return std::make_pair(pair.second, pair.first);
    };

    auto res1 = opt1.map_or_else(get_pair, swap_pair);
    auto res2 = opt2.map_or_else(get_pair, swap_pair);

    REQUIRE_EQ(res1, std::make_pair(0, 0));
    REQUIRE_EQ(res2, std::make_pair(60, 240));
}

[[=test]]
void ok_or() {
    auto opt1 = Option<int64_t>();
    auto opt2 = Option<int64_t>(INT64_MIN);

    auto res1 = Result<int64_t, std::string>(Err("none"));
    auto res2 = Result<int64_t, std::string>(Ok(INT64_MIN));

    REQUIRE_EQ(opt1.ok_or("none"), res1);
    REQUIRE_EQ(opt2.ok_or("none"), res2);
}

[[=test]]
void ok_or_else() {
    auto opt1 = Option<wchar_t>();
    auto opt2 = Option<wchar_t>(L'Ä');

    auto get_wchar_t = [] () {
        return L'\0';
    };

    auto res1 = opt1.ok_or_else(get_wchar_t);
    auto res2 = opt2.ok_or_else(get_wchar_t);

    REQUIRE(res1.is_err());
    REQUIRE(res2.is_ok());
    REQUIRE_EQ(res2.unwrap(), L'Ä');
}

[[=test]]
void and_() {
    auto opt1 = Option<int>();
    auto opt2 = Option<int>(42);

    REQUIRE(opt1.and_(opt1).is_none());
    REQUIRE(opt1.and_(opt2).is_none());
    REQUIRE(opt2.and_(opt1).is_none());
    REQUIRE(opt2.and_(opt2).is_some());
    REQUIRE_EQ(opt2.and_(opt2).unwrap(), 42);
}

[[=test]]
void and_then() {
    auto opt1 = Option<int>();
    auto opt2 = Option<int>(69);

    auto sub_2 = [] (int val) -> Option<int> {
        return val - 2;
    };

    auto res1 = opt1.and_then(sub_2);
    auto res2 = opt2.and_then(sub_2);

    REQUIRE(res1.is_none());
    REQUIRE(res2.is_some());
    REQUIRE_EQ(res2.unwrap(), 67);
}

[[=test]]
void filter() {
    std::vector<Option<int>> vals = { None, 1, 3, None, 4, None, None, 9, 8, None };
    std::vector<Option<int>> results;
    for (Option<int> v : vals) {
        results.push_back(v.filter([] (int val) -> bool {
            return val > 4;
        }));
    }
    const std::vector<Option<int>> match = { None, None, None, None, None, None, None, 9, 8, None };
    REQUIRE_EQ(results, match);
}

[[=test]]
void or_() {
    auto opt1 = Option<int>();
    auto opt2 = Option<int>(42);

    REQUIRE(opt1.or_(opt1).is_none());
    REQUIRE(opt1.or_(opt2).is_some());
    REQUIRE_EQ(opt1.or_(opt2).unwrap(), 42);
    REQUIRE_EQ(opt1.or_(Some(42)).unwrap(), 42);
    REQUIRE(opt2.or_(opt1).is_some());
    REQUIRE_EQ(opt2.or_(opt1).unwrap(), 42);
    REQUIRE_EQ(opt2.or_(Some(7)).unwrap(), 42);
}

[[=test]]
void or_else() {
    auto opt1 = Option<int>();
    auto opt2 = Option<int>(42);

    auto fallback = [] () -> Option<int> {
        return Option<int>(99);
    };

    auto none_fallback = [] () {
        return None;
    };

    auto some_fallback = [] () {
        return Some(77);
    };

    REQUIRE(opt1.or_else(fallback).is_some());
    REQUIRE_EQ(opt1.or_else(fallback).unwrap(), 99);
    REQUIRE(opt1.or_else(none_fallback).is_none());
    REQUIRE(opt1.or_else(some_fallback).is_some());
    REQUIRE_EQ(opt1.or_else(some_fallback).unwrap(), 77);
    REQUIRE(opt2.or_else(fallback).is_some());
    REQUIRE_EQ(opt2.or_else(fallback).unwrap(), 42);
    REQUIRE(opt2.or_else(none_fallback).is_some());
    REQUIRE_EQ(opt2.or_else(none_fallback).unwrap(), 42);
    REQUIRE(opt2.or_else(some_fallback).is_some());
    REQUIRE_EQ(opt2.or_else(some_fallback).unwrap(), 42);
}

[[=test]]
void xor_() {
    auto none = Option<int>();
    auto some1 = Option<int>(1);
    auto some2 = Option<int>(2);

    REQUIRE(none.xor_(none).is_none());
    REQUIRE(none.xor_(some1).is_some());
    REQUIRE_EQ(none.xor_(some2).unwrap(), 2);
    REQUIRE(some1.xor_(none).is_some());
    REQUIRE_EQ(some1.xor_(none).unwrap(), 1);
    REQUIRE(some1.xor_(some2).is_none());
    REQUIRE(some2.xor_(some1).is_none());
    REQUIRE(some1.xor_(Some(5)).is_none());
}

}

TEST_REGISTER_FILE();

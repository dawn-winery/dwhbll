# Dwhbll enum

Rust inspired sugary enums

## Defining an enum

Defining an enum is fairly straightforwards

```c++
ENUM foo {
    BEGIN_VARIANT(foo, u32);
    
    DEFINE_VARIANT(thing1, int) = 0;
    DEFINE_VARIANT(thing2, std::string, std::string) = 1;
    DEFINE_VARIANT(thing3, std::string) = 2;
    DEFINE_VARIANT(thing4) = 42;
    
    VARIANTS(foo, VARIANT(thing1), VARIANT(thing2), VARIANT(thing3));
}
```

First, you call the `BEGIN_VARIANT` macro to begin a variant
declaration.

Then you can make as many calls to `DEFINE_VARIANT` as necessary
to define all the members, it accepts the name of the variant
as well as an optional list of stored members.

Finally, call the `VARIANTS` macro, first parameter is the
containing type's name, then a list of `VARIANT()` calls
passing in the types of each variant declared.

`ENUM` is just a macro for `struct`, you may define any
additional members inside. Check the macros to find out what
internal members are accessible and how.

## Using a variant

### Storing and creating variants

variant names are callable to produce helpers, they must be stored into an object
of the parent type (nonexplicit conversion operator).

```c++
foo thing = foo::thing2("hitchikers", "guide");
foo second_thing = foo::thing4();
```

### Match

Match statements semi mirror what is available in rust

Case statements DO NOT fall though, and cannot be chained.

example:

```c++
MATCH(thing) {
    CASE(foo::thing1) {
        // do something
    }
    CASE(foo::thing2, x, y) {
        // values can be extracted as a binding statement too.
    }
    CASE_MUT(foo::thing3, s) {
        s = "foobar";
        // CASE_MUT can be used to get mutable references to the inner object.
    }
    CASE(foo::thing4) // braced statements are not necessary
    DEFAULT() // default case, existence is not enforced
    // can be used in place of DEFAULT() to show that not all
    // cases are expected to be covered.
    NONEXHAUSTIVE();
}
```

The program will enforce that the cases cover all possible conditions, UNLESS
`NONEXHAUSTIVE();` is called as the last statement in the MATCH.

### Checking if is variant

The IS_VARIANT macro can be used to check if the type is of the expected
variant, for example: `IS_VARIANT(second_thing, foo::thing4)` will return
true.

Alternatively, `MAYBE_IS` and `MAYBE_IS_MUT` can be used, for example:
`MAYBE_IS(thing, foo::thing2, x, y)`, the statement following will only
run if thing is of type foo::thing2, and it will unpack the members into
x, and y.

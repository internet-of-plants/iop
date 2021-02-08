# Linter Quirks

Contains information about linter conformance and common issues. Linters in c++ tend to have a lot of false positives. And since we use macros written in codebases that aren't as worried about the linters as ours wildly used external macros can cause a lot of warnings.

In that case we generally disable the warning, even when they are useful. And write a note in `platformio.ini` explaining why.

The rest may be disabled on demand. Some common warnings we get are documented here, on when they aren't useful. The best way to disable it. Or even how to rewrite things to completely avoid the warning. Their value (as in why they are enabled) may also be documented.

At worse grep for the lint name to find where it was disabled (if it was). That may also help understanding the best approach. Feel free to add more lints here if you notice some recurrent lint annoying you.

## performance-unnecessary-value-param

This is a constant issue. Because we abstract pointers in types like `StringView` and `StaticView`. And those types depend on implicit conversion to be ergonomic.

But because they are trivial and hold pointers we pass them by value. That triggers this lint. Which is a good lint in general. Except when dealing with this type.

The first instinct is to disable it in the method. The best way is to make sure the variable is mutable and call `std::move` on it somewhere in the method. You don't even need to call a move constructor, if you were using as `str.get()` replace for `std::move(str).get()`. And this only needs to be done once per method.

That will trick the lint to think we are actually using the value. The move is trivial so nothing will happen.

But otherwise things pass by reference or actually move the thing around.

## cppcoreguidelines-pro-bounds-constant-array-index

You should at least use `std::array`, but probably `Storage<T>`, `FixedString<SIZE>`, or make your own type with `TYPED_STORAGE(name, size)`.

In very specific cases you will have to use c arrays, so this lint may be disabled there.

Even then you probably could properly rewrite it to avoid the lint. Or at least make the access in a line and store to a reference tha is reused, that avoids cluttering the code with `NOLINT`s.

## cert-oop54-cpp

This lint has false positives for some reason. It's useful to detect bugs in move constructors. So it's disabled in a few specific places. Maybe you will have to disable it too.

## hicpp-explicit-conversions

As a general rule you should make single argument constructors explicit. The only exceptions we make are for ergonomy. They happen in `Option<T>`, `Result<T, E>`, `StaticString` and `StringView`. The first two because they can get fairly verbose with all the templating. And the second two is because they are exactly for implicit construction, they serve as ways to abstract less type-safe strings. So must remain implicit.

## cert-oop11-cpp

Generally move constructor... move out. But in c++ move constructors are non-destructive. This is a fundamental property of types like `Option<T>` or `Result<T, E>`. `Option<T>` is emptied out, but gets a otherwise valid value. `Result<T, E>` otherwise can't be properly emptied out, it gets a invalid state that will cause a panic if something other than the destructor is called on it. The best way to use those types is to extract a reference to the value and copy from it.

`std::{unique,shared}_ptr<T>` works like `Option<T>`, that can cause problems, from nullptr dereference to logic bugs. So when we can, for example in `Storage<SIZE>` we hijack move constructors to copy data (simple `std::shared_ptr<T>` copy), so nothings gets invalidated. Disabling the lint. This works well enough for us here.

# bugprone-macro-parentheses

This is a good lint to follow, but can't 
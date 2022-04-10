# Core

A huge part of this codebase is composed of the interwinded core code. That provides the higher level safe and ergonomic APIs. They all are integrated and depend on each-other, from string types, to sum-types, smart-pointers, panicking and a tracer that prints scope changes.

Using this safe and ergonomic core we were able to create high level code that satiesfies our needs for maintenance and reliability. Rarely messing with raw pointers, worrying about buffer overrun and having the type-safety to separate different types that have different purposes but are generally wongly abstracted as `String`.

## Strings

Strings are heavily used throught the code. And we try our best to avoid using raw pointers. Because of that a few types of strings were created to abstract all our usages in a type-safe and UB-free way.

Strings are implied to be all printable. There are methods to check it like `iop::isAllPrintable`. But you should check when deserializing data using `Unsafe` functions. Doing this has exposed bugs in our code before. Not everywhere is ensured to check, but we have a lot of tracing logs to detect non-printable characters in strings. Also `String` is `Arduino` core, we can't force it to be only printable always, but we do check when we get it from things like the network (or storage memory).

There are many string types:

`std::string`, `std::string_view`, `iop::StaticString`, `iop::CowString`, `String`, `std::array<char, SIZE>`

They are all supposed to be converted to a `std::string_view` before being handled as strings (except `StaticString`), if a constructor is not available, use `iop::to_view`. But `std::string_view` shouldn't be stored around, it's just a reference. So store the actual string storage and use the view to have access to a unified string API.

`iop::StaticString` can't be converted to `std::string_view` because its data is stored in PROGMEM, so needs 32 bits aligned reads. Some functions like `strlen` will cause a hardware exception if IOP_ROM data is passed to it. It needs functions ended in `_P` like `strlen_P` or to be converted to a `std::string`. This is still good because stack space is way smaller than heap space, so we copy the data from storage to the heap keeping the stack small. And immediately after release the memory. It can cause heap fragmentation if you keep the string around, but any "long-living" allocation can.

## Sum types

There are abstractions like `std::optional<T>`, `std::variant<T, E>` that are there to provide safe ways to use sum-types while still being ergonomic.

You should not use the default methods to extract the inner value, but use `iop::unwrap{_ok,_err}{_ref,_mut}`. They panic if the invalid variant is tried to be accessed. That means you must check before unwrapping. Since we don't support exceptions and don't want UB, please use these methods to integrate with our panic system.

`ìop::unwrap` moves out by default. This can be useful, but can cause problems by reusing an emptied out sum-type. Since c++ only has non-destructive moves. Moving out of an `std::optional<T>` is not a huge deal since it empties it, but may still cause logical bugs. Moving out of an `std::variant<T, E>` would mean the only valid operation you can do with it is destroying it, so we don't allow that by default, don't do it. At most move out of the inner variant type after getting a mutable reference to it.

That means you will probably want to use methods that extract a reference to the inner value (if available). Like `iop::unwrap{_OK,_ERR}_(ref_mut)`, that extract a reference to the inner value. So the sum type keeps intact and you can use the data.

By default panics only log core. But the application uses a panic function that keeps checking for updates and logs the critical error. Panics are the last barrier between the code and UB. They are there to make sure those abstractions listed above are safe.

Read each type documentation for more information.
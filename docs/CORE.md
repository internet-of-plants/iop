# Core

A huge part of this codebase is composed of the interwinded core code. That provides the higher level safe and ergonomic APIs. They all are integrated and depend on each-other, from string types, to sum-types, smart-pointers, panicking and a tracer that prints scope changes.

Using this safe and ergonomic core we were able to create high level code that satiesfies our needs for maintenance and reliability. Rarely messing with raw pointers, worrying about buffer overrun and having the type-safety to separate different types that have different purposes but are generally abstracted as `String`. Making them sized as they should be.

Read the quoted types documentation for more details

## Strings

Strings are heavily used throught the code. And we try our best to avoid using raw pointers. Because of that a few types of strings were created to abstract all our usages in a type-safe and UB-free way.

Strings are implied to be all printable. There are methods to check it like `StringView::isAllPrintable`. But you should check when deserializing data using `Unsafe` functions. Doing this has exposed bugs in our code before. Not everywhere is ensured to check, but we have a lot of tracing logs to detect non-printable characters in strings. Also `String` is `Arduino` core, we can't force it to be only printable always, but we do check when we get it from things like the network (or flash memory).

There are many string types:

`StringView`, `StaticString`, `FixedString<SIZE>`, `CowString`, `UnsafeRawString`, `String`, `Storage<SIZE>` or custom types created with `TYPED_STORAGE(name, size)`

They are all supposed to be converted to a `StringView` before being handled as strings. But `StringView` can't be stored, it's basically a reference. So store the actual string storage.

`Storage<SIZE>` and types created with `TYPED_STORAGE(name, size)` may not contain strings, like a wifi SSID, that is pretty much a binary blob.

`StaticString` can't be converted to `StringView` because its data is stored in PROGMEM, so needs 32 bits aligned reads. Some functions like `strlen` will cause a hardware exception if PROGMEM data is passed to it. It needs functions ended in `_P` like `strlen_P` or to be converted to a `String`. This is still good because stack space is way smaller than heap space, so we copy the data from flash to the heap keeping the stack small. It can cause heap fragmentation tho, but we monitor that.

## Sum types

There are abstractions like `Option<T>`, `Result<T, E>` that are there to provide safe ways to use sum-types while still being ergonomic.

They panic if the invalid variant is tried to be accessed. That means you must check before unwrapping.

They move out by default. This can be useful, but can cause problems by reusing an emptied out sum-type. Since c++ only has non-destructive moves. Moving out of an `Option<T>` is not a big deal since it empties it, but may still cause logical bugs. Moving out of an `Result<T, E>` tho will mean the only valid operation you can do with it is destroying it, any other **will** cause a `panic_`.

That means you will probably want to use methods that extract a reference to the inner value (if available). Like `Option<T>::asRef()`, `Option<T>::asMut()`, or the macros `UNWRAP(_OK,_ERR)_{REF,MUT}`, that extract a reference to the inner value. So the sum type keeps intact and you can use the data.



We have a logging core.

We have a panic function that keeps checking for updates and logs the critical error. Panics are the last barrier between the code and UB. They are there to make sure those abstractions listed above are safe.

Read each type documentation for more.
# Internet of Plants - Embedded Software

Datasheet: https://www.electrodragon.com/w/ESP-12F_ESP8266_Wifi_Board

# Dependencies

PlatformIO (nodemcu + arduino framework)

Needs OpenSSL in PATH, clang-format (install LLVM to get it) also should be in PATH if you want to be nice with our codebase :).

- https://wiki.openssl.org/index.php/Binaries
- https://releases.llvm.org/download.html

On linux you have to handle configs and permissions to deploy to serial port

```
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo service udev restart
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
```

# Build

After cloning you must set the configurations. To do that run this command:

`cp include/configuration.h.example include/configuration.h`

And fill all the constants with the appropriate value

Then you can deploy the code to the nodemcu using platform-io.

# TODO

Grep for "TODO"'s to find the known missing pieces

# Decisions

There have been a few inneficient decisions we took through this code. This was thinking about safety, to prevent programmer screw-ups from causing big problems. Performance isn't our bottleneck, since we don't do much in the embedded system, so safety becomes the prime issue. We want a stable system that can run without intervention for long times.

Ideally we would use zero-runtime-cost abstractions, but cpp doesn't help us make them UB proof, so we have to add runtime overhead to deal with it in a way that allows us to sleep at night.

Most decisions are listed here. If you find some other questionable decision please file an issue so we can explain them. Or even make a PR documenting them, and why they were made.

- Wrapping most things in pointers

    Since there is a background WiFi task running that we don't control it's hard to debug stack-overflows. Since they can happen outside of our code. So we tend to allocate most big things. Always using smart-pointers (std::unique_ptr, std::shared_ptr, FixedString<SIZE>, Storage<SIZE>...)

- Hijack all moves to make copies

    Since cpp doesn't have destructive moves, it can leave our code in invalid state. Either with a nulled `std::{unique_ptr,shared_ptr}`. Or with a empty `Result<T, E>`. And since those abstractions are heavily used throughout the code we don't want a human mistake to cause UB, panic or raise exceptions. To prevent that we try to hijack moves when we can, and force them to operate like copies. Either by doing it in a smart-pointer wrapper (`Storage<SIZE>`, `FixedString<SIZE>`...). Or by calling `.asRef()` or `.asMut()` (on `Option<T>` or `Result<T, E>`), since we don't need to move out of the object, we can move a reference out and keep the original intact.

    For `Optional<T>` `UNWRAP_{REF, MUT}` macros are the best way to ergonomically access a reference to the inner value, although `UNWRAP` also is available. And for `Result<T, E>` you should not use `Result` methods, but only the macros defined in `result.hpp`, like `UNWRAP_{OK, ERR}(_REF, _MUT)`, `IS_{OK, ERR}`, `{OK, ERR}`, etc. All methods have a macro alternative, that checks for an emptied Result and properly reports the invalid access location.

- No exceptions, but we halt using the `panic_` macro

    Most errors should be propagated with `Result<T, E>` or even an `Option<T>`. Exceptions should not happen. And critical errors, that can't be recovered, working as the last stand between us and UB should panic with the `panic_(F("Explanation of what went wrong..."))` macro.

    We don't like exceptions. It may be a naive decision, but we do want a way to fail hard. So we create a function to panic on our own way. It doesn't use any compiler/esp8266 panic internals, so maybe panic is a bad name. But it basically logs the critical error, reports it through the network (to the server, but we could report to nearby devices that we own too, so they can help) if we can and halt.

    We have future plans to improve this, but we should never panic. Panics should be a way to avoid UB when everything went wrong. Ideally we should be able to recover from it, either with a timer, calling the internal ESP8266 panic functions (and reseting). Or through the network, being able to receive updates, from the server or other devices running this code. Anyhow, we should be able to recover when we can, but avoid getting stuck into a broken infinite loop that's crashing and restarting really fast.

- Repeated runtime checks

    There are a few situations where we check for the same runtime problem multiple times. That happens to improve logging. So we can know exactly where the problem happened.
    
    It happens mostly in `Result<T, E>` and `Option<T>`, because we have to check their state (with `.isSome()`, `.isOk()`, ...) and then unwrap them to extract the inner value (which also checks, panicking if something went wrong). That's because cpp forces our hand by not having sum types with propper pattern matching. The mainstream solutions just return null and force the check on the user (at the cost of UB if the user makes a mistake), check like we do and throw an exception or just plainly cause UB if you try to get a value that's not there. Since UB is considered by us unnaccetable we check by default, panicking in case of a problem. Theoretically we could make zero-runtime-overhead `unsafe_` prefixed alternatives to signal the programmer took extra care, but you would have to convince the community that it will cause significant performance improvements. And that this improvement is important. Since performance is hardly this project's bottleneck we wish you good luck (or fork).

    Ideally a stack-trace would solve this for most cases, but we haven't managed to make our `panic_` print a stacktrace. It's possible as it happens in ESP8266 default Exceptions, but we haven't integrated it yet. It mostly happens at `result.hpp`.

    But sometimes a stacktrace wouldn't help much, we use those checks to enable detailed logging at runtime when something goes wrong, so we have to make the same checks in a few sequential places.
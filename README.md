# Internet of Plants - Embedded Software

Firmware for Internet of Plants embedded system. Made for ESP8266.

It connects to an already existent wifi network, and through it connects to a hardcoded [host](https://github.com/internet-of-plants/server), authenticating itself. It provides air humidity, air temperature, soil temperature and soil humidity measurements to the server every few minutes.

Eventually the goal is to use this data, to generate reports and properly analyze what is going on and allow one day for a good automation system. But we don't believe a naive system without much data to be better than simple timers, that's why we don't consider automation before providing a system to build a significant dataset. This is a plant monitor for now. If you want to automate your best bet for now is to buy a power plug with a timer.

There also are plans to use this to track entire grow rooms instead of a single plant. But they are not a current priority.

If there is no wifi credential or iop credentials stored, a captive portal will be open. The device will open its own wifi network, the password is hardcoded (this is a security vuln, we should generate it at compile time and save it externally, like a sticker, but we lack the infra to do it right now).

After connecting to the captive portal you must provide your wifi and iop credentials. If the wifi credential is correct it will login to the wifi network and then authenticate with the iop server.

No server credential is stored in the device, it will use the credentials to generate an access token for that device that is then stored (and can be revoked).

When the device is online and authenticated to the server a new plant is automatically created on your account if it doesn't already exist. Now you are able to track your plant with the iop server. There you also will be able to provide updates to each specific device (or some of them, or all of them). And monitor panics + logs for those devices.

The updates are automatic and happen over the air. Eventually the updates will demand signed binaries.

If some irrecoverable error happens the device will halt and constantly try to update it's binary. All errors are reported to the network (if available). So you must react to it and provide an update to fix it. Since our system is very small and event driven an irrecoverable error won't be solved by restarting, since the error will just happen again almost immediately. Ideally panics shouldn't happen, so easy recovery doesn't seem like a priority, but as the project evolves we can study a panic that restarts the system instead of halting + trying to update (or in addition to that).

If there is no network available it will just halt forever and must be restarted/updated physically.

The project has been designed and tested for the ESP8266-12F board (nodemcu), but it should be easily portable (and eventually part of our goal, but it's far away, right now we only commit to supporting ESP8266).

Datasheet: https://www.electrodragon.com/w/ESP-12F_ESP8266_Wifi_Board

# Dependencies

PlatformIO (nodemcu2 + arduino framework)

Needs OpenSSL in PATH, clang-format and clang-tidy (install LLVM to get it). Be nice to our codebase :).

- https://wiki.openssl.org/index.php/Binaries
- https://releases.llvm.org/download.html

On linux you have to change permissions to be able to deploy to a serial port

```
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo service udev restart
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
```

# Build

You can just deploy the code to the nodemcu using PlatformIO. Choose `env:release` to deploy to production. After the first deploy you can use the update server.

# TODO

Grep for "TODO"'s to find the known missing pieces

# Decisions

There have been a few possibly questionable decisions we took through this code. This was thinking about safety, to prevent programmer screw-ups from causing big problems. Performance isn't our bottleneck, since we don't do much in the embedded system, so safety becomes the prime issue. We want a stable system that can run without intervention for long times.

Ideally we would use zero-runtime-cost abstractions, but cpp doesn't help us make them UB proof, so we have to add runtime overhead to deal with it in a way that allows us to sleep at night.

Most decisions are listed here. If you find some other questionable decision please file an issue so we can explain them. Or even make a PR documenting them, and why they were made.

- Avoid static variables, heap allocations, big stack usage: put your buffer in unused4KbSysStack

    Since ESP8266 doesn't have much RAM, and it shares this RAM between IRAM, a few stacks, and regular heap. And that SSL connections uses something like 20kb of regular heap. And the default stack is 4kb We have found problems related to heap fragmentation and stack overflow in rare edge cases. Both of those are hard to debug, recover and fix for good.

    So to avoid that we increase the stack size. But the regular stack uses a neat trick, there is 4kb of sys stack that is unused, so the stack goes there by default. When we increase the stack size, it moves elsewhere and those 4kb get unused. So to avoid wasting space, and help tracking the upper memory bound, we created unused4KbSysStack, so all buffers and critical data-structures tend to go there. If we overflow the size, a static_assert will catch it.

    This allowed us to reuse buffers and track better the RAM usage, we endedup cleaning the RAM usage by half with this recycling. And avoid heap fragmentation as everything is pre-allocated. And now the system can run trivially for a long time without problems. There are a lot of Strings and ESP8266 libs buffers that can still cause heap fragmentation and eventual issues, but those are much rarer. And we are improving our resources monitoring in the long run, so this can be detected. And a casual reboot from time to time is not a big deal.

- Avoiding moves

    Since cpp doesn't have destructive moves, it can leave our code in an invalid state. Specially because the CPP reference only says that the value must be valid, but indeterminate, so it's technically valid that some methods cause UB when the structure is reused without being refilled (for example to set a nullptr to a moved out String and when trying to print assuming it's empty it crashes, this was fixed upstream but happened before). The problem is the standard allowing that, so we can never depend on moved out objects being sane. And even when it makes sense, a nulled `std::{unique,shared}_ptr`, for example, may cause logic problems because it generally only is checked for nullness at creation. So it can be propagated around while empty and only crash when used, far away from where it was wrongly emptied. And since those abstractions are heavily used throughout the code we don't want a human mistake to cause UB, panic or raise exceptions. We prefer to create redundancies. Even a wrongly moved-out `std::optional<T>` can cause logical errors.

    To avoid that we try not to move out, getting references when we can. For example using `iop::unwrap_{ok,err}_(ref,mut)` for `std::optional<T>` and `std::variant<T, E>`. But you have to be careful to make sure that the reference doesn't outlive its storage (as always). If a move is helpful, move the inner type out, keeping its variant intact.

- No exceptions, but we halt using the `iop_panic` macro

    Most errors should be propagated with `std::variant<T, E>` or even an `std::optional<T>`. Exceptions should not happen. And critical errors, that can't be recovered from, should panic as the last stand between us and UB. With the `iop_panic(F("Explanation of what went wrong..."))` macro.

    We don't want to pay the overhead for exceptions. Nor have alternatives code paths. It either works and reports the error with the return type. Or `iop_panic(F("Message...))` and halt, with no turning back. `iop_panic` logs the critical error, reports it through the network to the main server if it can, and keeps asking the server for updates. If network is available and working, when an update is provided it will download it and reboot.

    We have future plans to improve this, but we should never panic. Panics should be a way to avoid UB when everything went wrong, and quickly fix when detected.

    The embedded code is fairly small and a panic is probably going to be recurrent if no updates happen, so for now halting and allowing external updates to fix the problem seems the way to go. All errors are reported to the network, if available. While debugging manually press physical the restart button, and when done update all the devices.

    Panics before having network access + authentication with the central server have a very small code surface to cause. They should be very rare, hopefully impossible. But they are considered critical bugs. In this case the device will halt until manually restarted. They will only be fixable manually. Those panics are theoretically possible because we have no way to statically garantee their branches are unreachable, but they should be.

    Some software and hardware exceptions may still happen, we don't handle them, but it's a TODO to handle what we can. We should also report restart reasons. Panics also still don't support stack-dumps, but it's planned. PRs are welcome.

- Redundant runtime checks

    There are a few situations where we check for the same runtime problem multiple times. In some places that happens to improve logging. So we can know better the lifecycle of the problem.
    
    It also happens a lot with `std::variant<T, E>` and `std::optional<T>`, because we have to check their state (with `iop::is_err`, `iop::is_ok`, `std::optional<T>::has_value`) and then unwrap them to move out the inner value (with `iop::unwrap_{ok,err}_(ref,mut)` which checks again, panicking if the wanted value is not there).
    
    That's because cpp forces our hand by not having proper sum types with proper pattern matching. The mainstream solutions is `std::visit` which increases code boilerplate by a lot to support every single `(T, E)` pair (the possible cost outweights the benefits). Or just borrowing a pointer, that is null if not available. And force the check onto the user (at the cost of UB if the user makes a mistake). Or they check before accessing and throw an exception, or just plainly cause UB if you try to get a value that's not there. Since UB is considered unnacceptable by us we check by default on all accesses, panicking in case of a problem. Theoretically we could make zero-runtime-overhead `unchecked_` prefixed alternatives to signal the programmer took extra care, but you would have to convince the community that it will cause significant performance improvements. And that this improvement is important. Since performance is hardly this project's bottleneck: we wish you good luck (or fork). Use `unwrap`s.
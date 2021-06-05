## Description

This is a C++ library for doing basic operations on audio devices (e.g. mute/unmute, changing active device) on MacOS and Windows.

It is used by:
- [StreamDeck-AudioMute](https://github.com/fredemmott/StreamDeck-AudioMute)
- [StreamDeck-AudioSwitcher](https://github.com/fredemmott/StreamDeck-AudioSwitcher)

# Adding this to a CMake project

1. Download `AudioDeviceLib.cmake` from
[the latest release](https://github.com/fredemmott/AudioDeviceLib/releases/latest)
2. `include()` it in your project
3. A `AudioDeviceLib` static library is now available in your project; depending on it
   will also make nlohmann/json, websocketpp, and asio available in your project,
   including header files.

For example:

```cmake
include("AudioDeviceLib.cmake")
add_executable(
  myapp
  main.cpp
)
target_link_libraries(myapp AudioDeviceLib)
```

# License

This code is distributed under the terms of the
[MIT License](LICENSE).  

I am providing code in the repository to you under an open source license.
Because this is my personal repository, the license you recieve to my code
is from me and not my employer (Facebook).

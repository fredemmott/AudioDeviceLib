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

# Getting Help

I make this for my own use, and I share this in the hope others find it useful; I'm not able to commit to support, bug fixes, or feature development.

If you have found a bug or have a feature request, please check [GitHub issues](https://github.com/fredemmott/AudioDeviceLib/issues) to see if it has already been reported, and [create a new issue](https://github.com/fredemmott/AudioDeviceLib/issues/new) if not.

Support may be available from the community via [Discord](https://discord.gg/CWrvKfuff3).

I am not able to respond to 1:1 requests for help via any means, including GitHub, Discord, Twitter, Reddit, or email.

# License

This code is distributed under the terms of the
[MIT License](LICENSE).  

/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#pragma once

#include <functional>
#include <map>
#include <stdexcept>
#include <string>

namespace FredEmmott::Audio {

class device_error : public std::runtime_error {
 protected:
  device_error(const char* what) : std::runtime_error(what) {
  }
};

class operation_not_supported_error : public device_error {
 public:
  operation_not_supported_error() : device_error("Operation not supported") {
  }
};

class device_not_available_error : public device_error {
 public:
  device_not_available_error() : device_error("Device not available") {
  }
};

enum class AudioDeviceRole {
  DEFAULT,
  COMMUNICATION,
};

enum class AudioDeviceDirection {
  OUTPUT,
  INPUT,
};

enum class AudioDeviceState {
  CONNECTED,
  DEVICE_NOT_PRESENT,// USB device unplugged
  DEVICE_DISABLED,
  DEVICE_PRESENT_NO_CONNECTION,// device present, but nothing's plugged into it,
                               // e.g. headphone jack with nothing plugged in
};

struct AudioDeviceInfo {
  std::string id;
  std::string interfaceName;// e.g. "Generic USB Audio Device"
  std::string endpointName;// e.g. "Speakers"
  std::string displayName;// e.g. "Generic USB Audio Device (Speakers)"
  AudioDeviceDirection direction;
  AudioDeviceState state;

  auto operator<=>(const AudioDeviceInfo&) const = default;
};

std::map<std::string, AudioDeviceInfo> GetAudioDeviceList(AudioDeviceDirection);
AudioDeviceState GetAudioDeviceState(const std::string& id);

std::string GetDefaultAudioDeviceID(AudioDeviceDirection, AudioDeviceRole);
void SetDefaultAudioDeviceID(
  AudioDeviceDirection,
  AudioDeviceRole,
  const std::string& deviceID);

bool IsAudioDeviceMuted(const std::string& deviceID);
void MuteAudioDevice(const std::string& deviceID);
void UnmuteAudioDevice(const std::string& deviceID);

class MuteCallbackHandle final {
 public:
  class Impl;
  MuteCallbackHandle() = default;
  MuteCallbackHandle(const std::shared_ptr<Impl>& p);
  ~MuteCallbackHandle();

 private:
  std::shared_ptr<Impl> p;
};

MuteCallbackHandle AddAudioDeviceMuteUnmuteCallback(
  const std::string& deviceID,
  std::function<void(bool isMuted)>);

class DefaultChangeCallbackHandle final {
 public:
  class Impl;
  DefaultChangeCallbackHandle() = default;
  DefaultChangeCallbackHandle(const std::shared_ptr<Impl>& p);
  ~DefaultChangeCallbackHandle();

 private:
  std::shared_ptr<Impl> p;
};

DefaultChangeCallbackHandle AddDefaultAudioDeviceChangeCallback(
  std::function<
    void(AudioDeviceDirection, AudioDeviceRole, const std::string&)>);

class AudioDevicePlugEventCallbackHandle final {
 public:
  class Impl;
  AudioDevicePlugEventCallbackHandle() = default;
  AudioDevicePlugEventCallbackHandle(const std::shared_ptr<Impl>& p);
  ~AudioDevicePlugEventCallbackHandle();

 private:
  std::shared_ptr<Impl> p;
};

enum class AudioDevicePlugEvent { ADDED, REMOVED };

AudioDevicePlugEventCallbackHandle AddAudioDevicePlugEventCallback(
  std::function<void(AudioDevicePlugEvent, const std::string&)>);

}// namespace FredEmmott::Audio

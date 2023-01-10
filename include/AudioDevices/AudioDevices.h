/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

// TODO: use std::expected instead in C++23
#include "expected.h"

namespace FredEmmott::Audio {

enum class Error {
  UNKNOWN,
  DEVICE_NOT_AVAILABLE,
  OPERATION_UNSUPPORTED,
  OUT_OF_RANGE,
};

template <>
class bad_expected_access<Error> : public bad_expected_access<void> {
 public:
  bad_expected_access(Error e) : bad_expected_access<void>(), mError(e) {
  }
  bad_expected_access() = delete;

  virtual const char* what() const noexcept override {
    switch (mError) {
      case Error::DEVICE_NOT_AVAILABLE:
        return "Bad expected access - device not available";
      case Error::OPERATION_UNSUPPORTED:
        return "Bad expected access - operation not supported";
      case Error::UNKNOWN:
        return "Bad expected access - unknown OS error";
    }
    return "Bad expected access - INVALID ERROR VALUE";
  };

 private:
  Error mError;
};

template <class T>
using result = expected<T, Error>;

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

result<bool> IsAudioDeviceMuted(const std::string& deviceID);
result<void> MuteAudioDevice(const std::string& deviceID);
result<void> UnmuteAudioDevice(const std::string& deviceID);

struct VolumeRange {
  float minDecibels {};
  float maxDecibels {};
  float incrementDecibels {};
  uint32_t volumeSteps;
};

struct Volume {
  bool isMuted {};
  float volumeScalar {};
  std::optional<float> volumeDecibels {};
  std::optional<uint32_t> volumeStep {};
};

result<VolumeRange> GetDeviceVolumeRange(const std::string& deviceID);
result<Volume> GetDeviceVolume(const std::string& deviceID);
result<void> SetDeviceVolumeScalar(const std::string& deviceID, float);
result<void> SetDeviceVolumeDecibels(const std::string& deviceID, float);
result<void> IncreaseDeviceVolume(const std::string& deviceID);
result<void> DecreaseDeviceVolume(const std::string& deviceID);

class MuteCallbackHandle final {
 public:
  class Impl;
  MuteCallbackHandle() = default;
  MuteCallbackHandle(const std::shared_ptr<Impl>& p);
  ~MuteCallbackHandle();

 private:
  std::shared_ptr<Impl> p;
};

result<MuteCallbackHandle> AddAudioDeviceMuteUnmuteCallback(
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

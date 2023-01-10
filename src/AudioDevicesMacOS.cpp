/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include <AudioDevices/AudioDevices.h>
#include <CoreAudio/CoreAudio.h>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace FredEmmott::Audio {

namespace {

std::string Utf8StringFromCFString(CFStringRef ref, size_t buf_size = 1024) {
  // Re-use the existing buffer if possible...
  auto ptr = CFStringGetCStringPtr(ref, kCFStringEncodingUTF8);
  if (ptr) {
    return ptr;
  }
  // ... but sometimes it isn't. Copy.
  char buf[buf_size];
  CFStringGetCString(ref, buf, buf_size, kCFStringEncodingUTF8);
  return buf;
}

static Error ErrorFromOSStatus(OSStatus s) {
  switch (s) {
    case kAudioHardwareBadDeviceError:
    case kAudioHardwareBadObjectError:
      return Error::DEVICE_NOT_AVAILABLE;
    case kAudioHardwareUnsupportedOperationError:
    case kAudioHardwareUnknownPropertyError:
      return Error::OPERATION_UNSUPPORTED;
    default:
      return Error::UNKNOWN;
  }
}

template <class T>
result<T> GetAudioObjectProperty(
  AudioObjectID id,
  const AudioObjectPropertyAddress& prop) {
  T value;
  UInt32 size = sizeof(value);
  const auto result
    = AudioObjectGetPropertyData(id, &prop, 0, nullptr, &size, &value);
  if (result != kAudioHardwareNoError) {
    return {unexpect, ErrorFromOSStatus(result)};
  }
  return value;
}

template <>
result<bool> GetAudioObjectProperty<bool>(
  UInt32 id,
  const AudioObjectPropertyAddress& prop) {
  return result<bool> {GetAudioObjectProperty<UInt32>(id, prop)};
}

template <>
result<std::string> GetAudioObjectProperty<std::string>(
  UInt32 id,
  const AudioObjectPropertyAddress& prop) {
  CFStringRef value = nullptr;
  UInt32 size = sizeof(value);
  const auto result
    = AudioObjectGetPropertyData(id, &prop, 0, nullptr, &size, &value);
  if (result != kAudioHardwareNoError) {
    return {unexpect, ErrorFromOSStatus(result)};
  }

  if (!value) {
    return {unexpect, Error::OPERATION_UNSUPPORTED};
  }
  auto ret = Utf8StringFromCFString(value);
  CFRelease(value);
  return ret;
}

result<std::string> MakeDeviceID(UInt32 id, AudioDeviceDirection dir) {
  const auto uid = GetAudioObjectProperty<std::string>(
    id,
    {kAudioDevicePropertyDeviceUID,
     kAudioObjectPropertyScopeGlobal,
     kAudioObjectPropertyElementMain});
  if (!uid) {
    return {unexpect, Error::DEVICE_NOT_AVAILABLE};
  }

  char buf[1024];
  snprintf(
    buf,
    sizeof(buf),
    "%s/%s",
    dir == AudioDeviceDirection::INPUT ? "input" : "output",
    uid->c_str());
  return std::string(buf);
}

result<std::tuple<UInt32, AudioDeviceDirection>> ParseDeviceID(
  const std::string& id) {
  auto idx = id.find_first_of('/');
  auto direction = id.substr(0, idx) == "input" ? AudioDeviceDirection::INPUT
                                                : AudioDeviceDirection::OUTPUT;
  CFStringRef uid = CFStringCreateWithCString(
    kCFAllocatorDefault, id.substr(idx + 1).c_str(), kCFStringEncodingUTF8);
  UInt32 device_id;
  AudioValueTranslation value {
    &uid, sizeof(CFStringRef), &device_id, sizeof(device_id)};
  AudioObjectPropertyAddress prop {
    kAudioHardwarePropertyDeviceForUID,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain};
  UInt32 size = sizeof(value);

  const auto result = AudioObjectGetPropertyData(
    kAudioObjectSystemObject, &prop, 0, nullptr, &size, &value);
  CFRelease(uid);

  if (result != kAudioHardwareNoError) {
    return {unexpect, ErrorFromOSStatus(result)};
  }

  if (!device_id) {
    return {unexpect, Error::DEVICE_NOT_AVAILABLE};
  }

  return std::make_tuple(device_id, direction);
}

result<void> SetAudioDeviceIsMuted(const std::string& id, bool muted) {
  const UInt32 value = muted;
  const auto parsed = ParseDeviceID(id);
  if (!parsed) {
    return {unexpect, parsed.error()};
  }
  const auto [native_id, direction] = *parsed;
  AudioObjectPropertyAddress prop {
    kAudioDevicePropertyMute,
    direction == AudioDeviceDirection::INPUT ? kAudioDevicePropertyScopeInput
                                             : kAudioDevicePropertyScopeOutput,
    0};
  const auto result = AudioObjectSetPropertyData(
    native_id, &prop, 0, NULL, sizeof(value), &value);
  if (result != kAudioHardwareNoError) {
    return {unexpect, ErrorFromOSStatus(result)};
  }

  return {};
}

}// namespace

std::string GetDefaultAudioDeviceID(
  AudioDeviceDirection direction,
  AudioDeviceRole role) {
  if (role != AudioDeviceRole::DEFAULT) {
    return std::string();
  }
  AudioDeviceID native_id = 0;
  UInt32 native_id_size = sizeof(native_id);
  AudioObjectPropertyAddress prop = {
    direction == AudioDeviceDirection::INPUT
      ? kAudioHardwarePropertyDefaultInputDevice
      : kAudioHardwarePropertyDefaultOutputDevice,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain};

  AudioObjectGetPropertyData(
    kAudioObjectSystemObject, &prop, 0, NULL, &native_id_size, &native_id);
  return MakeDeviceID(native_id, direction).value();
}

void SetDefaultAudioDeviceID(
  AudioDeviceDirection direction,
  AudioDeviceRole role,
  const std::string& deviceID) {
  if (role != AudioDeviceRole::DEFAULT) {
    return;
  }

  const auto parsed = ParseDeviceID(deviceID);
  if (!parsed) {
    return;
  }

  const auto [native_id, id_dir] = *parsed;
  if (id_dir != direction) {
    return;
  }

  AudioObjectPropertyAddress prop = {
    direction == AudioDeviceDirection::INPUT
      ? kAudioHardwarePropertyDefaultInputDevice
      : kAudioHardwarePropertyDefaultOutputDevice,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain};
  AudioObjectSetPropertyData(
    kAudioObjectSystemObject, &prop, 0, NULL, sizeof(native_id), &native_id);
}

result<bool> IsAudioDeviceMuted(const std::string& id) {
  const auto parsed = ParseDeviceID(id);
  if (!parsed) {
    return {unexpect, parsed.error()};
  }
  const auto [native_id, direction] = *parsed;
  return GetAudioObjectProperty<bool>(
    native_id,
    {kAudioDevicePropertyMute,
     direction == AudioDeviceDirection::INPUT ? kAudioObjectPropertyScopeInput
                                              : kAudioObjectPropertyScopeOutput,
     kAudioObjectPropertyElementMain});
};

result<void> MuteAudioDevice(const std::string& id) {
  return SetAudioDeviceIsMuted(id, true);
}

result<void> UnmuteAudioDevice(const std::string& id) {
  return SetAudioDeviceIsMuted(id, false);
}

namespace {
result<std::string> GetDataSourceName(
  AudioDeviceID device_id,
  AudioObjectPropertyScope scope) {
  const auto data_source_result = GetAudioObjectProperty<AudioObjectID>(
    device_id,
    {
      kAudioDevicePropertyDataSource,
      scope,
      kAudioObjectPropertyElementMain,
    });
  if (!data_source_result) {
    return {unexpect, data_source_result.error()};
  }

  auto data_source = data_source_result.value();

  CFStringRef value = nullptr;
  AudioValueTranslation translate {
    &data_source, sizeof(data_source), &value, sizeof(value)};
  UInt32 size = sizeof(translate);
  const AudioObjectPropertyAddress prop {
    kAudioDevicePropertyDataSourceNameForIDCFString,
    scope,
    kAudioObjectPropertyElementMain};
  const auto status = AudioObjectGetPropertyData(
    device_id, &prop, 0, nullptr, &size, &translate);
  if (!status) {
    return {unexpect, ErrorFromOSStatus(status)};
  }
  if (!value) {
    return {unexpect, Error::OPERATION_UNSUPPORTED};
  }
  auto ret = Utf8StringFromCFString(value);
  CFRelease(value);
  return ret;
}

constexpr AudioObjectPropertyAddress gDeviceListProp {
  kAudioHardwarePropertyDevices,
  kAudioObjectPropertyScopeGlobal,
  kAudioObjectPropertyElementMain,
};

std::vector<AudioDeviceID> GetAudioDeviceIDs() {
  UInt32 size = 0;
  AudioObjectGetPropertyDataSize(
    kAudioObjectSystemObject, &gDeviceListProp, 0, nullptr, &size);
  const auto count = size / sizeof(AudioDeviceID);
  std::vector<AudioDeviceID> ids(count, {});
  AudioObjectGetPropertyData(
    kAudioObjectSystemObject, &gDeviceListProp, 0, nullptr, &size, ids.data());
  return ids;
}

bool AudioDeviceSupportsScope(
  AudioDeviceID id,
  AudioObjectPropertyScope scope) {
  // Check how many channels there are. No channels for a given direction? Not a
  // valid device for that direction.
  UInt32 size = 0;
  const AudioObjectPropertyAddress prop {
    kAudioDevicePropertyStreams,
    scope,
    kAudioObjectPropertyScopeGlobal,
  };
  AudioObjectGetPropertyDataSize(id, &prop, 0, nullptr, &size);
  return size > 0;
}

}// namespace

std::map<std::string, AudioDeviceInfo> GetAudioDeviceList(
  AudioDeviceDirection direction) {
  const auto ids = GetAudioDeviceIDs();
  std::map<std::string, AudioDeviceInfo> out;

  // The array of devices will always contain both input and output, even if
  // we set the scope above; instead filter inside the loop
  const auto scope = direction == AudioDeviceDirection::INPUT
    ? kAudioObjectPropertyScopeInput
    : kAudioObjectPropertyScopeOutput;
  for (const auto id: ids) {
    // ... so filter here
    if (!AudioDeviceSupportsScope(id, scope)) {
      continue;
    }

    const auto deviceID = MakeDeviceID(id, direction);
    if (!deviceID) {
      continue;
    }

    const auto manufacturer = GetAudioObjectProperty<std::string>(
      id,
      {
        kAudioObjectPropertyManufacturer,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
      });
    const auto model = GetAudioObjectProperty<std::string>(
      id,
      {
        kAudioDevicePropertyModelUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
      });

    if (!(manufacturer && model)) {
      continue;
    }

    AudioDeviceInfo info {
      .id = *deviceID,
      .interfaceName = *manufacturer + "/" + *model,
      .direction = direction,
    };
    info.state = GetAudioDeviceState(info.id);

    const auto data_source_name = GetDataSourceName(id, scope);
    if (data_source_name && !data_source_name->empty()) {
      info.displayName = *data_source_name;
      info.endpointName = *data_source_name;
    } else {
      const auto name = GetAudioObjectProperty<std::string>(
        id,
        {
          kAudioObjectPropertyName,
          kAudioObjectPropertyScopeGlobal,
          kAudioObjectPropertyElementMain,
        });
      if (!name) {
        continue;
      }
      info.displayName = *name;
    }

    out.emplace(info.id, info);
  }
  return out;
}

AudioDeviceState GetAudioDeviceState(const std::string& id) {
  const auto parsed = ParseDeviceID(id);
  if (!parsed) {
    return AudioDeviceState::DEVICE_NOT_PRESENT;
  }
  const auto [native_id, direction] = *parsed;
  if (!native_id) {
    return AudioDeviceState::DEVICE_NOT_PRESENT;
  }

  const auto scope = (direction == AudioDeviceDirection::INPUT)
    ? kAudioDevicePropertyScopeInput
    : kAudioDevicePropertyScopeOutput;

  const auto transport = GetAudioObjectProperty<UInt32>(
    native_id,
    {kAudioDevicePropertyTransportType,
     scope,
     kAudioObjectPropertyElementMain});

  if (!transport) {
    return AudioDeviceState::DEVICE_NOT_PRESENT;
  }

  // no jack: 'Internal Speakers'
  // jack: 'Headphones'
  //
  // Showing plugged/unplugged for these is just noise
  if (transport == kAudioDeviceTransportTypeBuiltIn) {
    return AudioDeviceState::CONNECTED;
  }

  const AudioObjectPropertyAddress prop = {
    kAudioDevicePropertyJackIsConnected,
    scope,
    kAudioObjectPropertyElementMain};
  const auto supports_jack = AudioObjectHasProperty(native_id, &prop);
  if (!supports_jack) {
    return AudioDeviceState::CONNECTED;
  }

  const auto is_plugged = GetAudioObjectProperty<bool>(native_id, prop);
  return (is_plugged && *is_plugged)
    ? AudioDeviceState::CONNECTED
    : AudioDeviceState::DEVICE_PRESENT_NO_CONNECTION;
}

namespace {

template <class TProp>
struct BaseCallbackHandleImpl {
  typedef std::function<void(TProp value)> UserCallback;
  BaseCallbackHandleImpl(
    UserCallback callback,
    AudioDeviceID device,
    AudioObjectPropertyAddress prop)
    : mProp(prop), mDevice(device), mCallback(callback) {
    AudioObjectAddPropertyListener(mDevice, &mProp, &OSCallback, this);
  }

  virtual ~BaseCallbackHandleImpl() {
    AudioObjectRemovePropertyListener(mDevice, &mProp, &OSCallback, this);
  }

 private:
  const AudioObjectPropertyAddress mProp;
  AudioDeviceID mDevice;
  UserCallback mCallback;

  static OSStatus OSCallback(
    AudioDeviceID id,
    UInt32 _prop_count,
    const AudioObjectPropertyAddress* _props,
    void* data) {
    auto self = reinterpret_cast<BaseCallbackHandleImpl<TProp>*>(data);
    const auto value = GetAudioObjectProperty<TProp>(id, self->mProp);
    self->mCallback(value);
    return 0;
  }
};

}// namespace

struct MuteCallbackHandle::Impl : BaseCallbackHandleImpl<bool> {
  Impl(UserCallback cb, AudioDeviceID device, AudioDeviceDirection direction)
    : BaseCallbackHandleImpl(
      cb,
      device,
      {kAudioDevicePropertyMute,
       direction == AudioDeviceDirection::INPUT
         ? kAudioObjectPropertyScopeInput
         : kAudioObjectPropertyScopeOutput,
       kAudioObjectPropertyElementMain}) {
  }
};

MuteCallbackHandle::MuteCallbackHandle(const std::shared_ptr<Impl>& p) : p(p) {
}

MuteCallbackHandle::~MuteCallbackHandle() = default;

result<MuteCallbackHandle> AddAudioDeviceMuteUnmuteCallback(
  const std::string& deviceID,
  std::function<void(bool isMuted)> cb) {
  const auto parsed = ParseDeviceID(deviceID);
  if (!parsed) {
    return {unexpect, Error::DEVICE_NOT_AVAILABLE};
  }
  const auto [id, direction] = *parsed;
  return {{std::make_shared<MuteCallbackHandle::Impl>(cb, id, direction)}};
}

struct DefaultChangeCallbackHandle::Impl {
  Impl(std::function<
       void(AudioDeviceDirection, AudioDeviceRole, const std::string&)> cb)
    : mInputImpl(
      [=](AudioDeviceID native_id) {
        const auto device
          = MakeDeviceID(native_id, AudioDeviceDirection::INPUT);
        if (!device) {
          return;
        }
        cb(AudioDeviceDirection::INPUT, AudioDeviceRole::DEFAULT, *device);
      },
      kAudioObjectSystemObject,
      {kAudioHardwarePropertyDefaultInputDevice,
       kAudioObjectPropertyScopeGlobal,
       kAudioObjectPropertyElementMain}),

      mOutputImpl(
        [=](AudioDeviceID native_id) {
          const auto device
            = MakeDeviceID(native_id, AudioDeviceDirection::OUTPUT);
          if (!device) {
            return;
          }
          cb(AudioDeviceDirection::OUTPUT, AudioDeviceRole::DEFAULT, *device);
        },
        kAudioObjectSystemObject,
        {kAudioHardwarePropertyDefaultOutputDevice,
         kAudioObjectPropertyScopeGlobal,
         kAudioObjectPropertyElementMain}) {
  }

 private:
  BaseCallbackHandleImpl<AudioDeviceID> mInputImpl;
  BaseCallbackHandleImpl<AudioDeviceID> mOutputImpl;
};

DefaultChangeCallbackHandle::DefaultChangeCallbackHandle(
  const std::shared_ptr<Impl>& p)
  : p(p) {
}

DefaultChangeCallbackHandle::~DefaultChangeCallbackHandle() = default;

DefaultChangeCallbackHandle AddDefaultAudioDeviceChangeCallback(
  std::function<void(AudioDeviceDirection, AudioDeviceRole, const std::string&)>
    cb) {
  return std::make_shared<DefaultChangeCallbackHandle::Impl>(cb);
}

struct AudioDevicePlugEventCallbackHandle::Impl final {
  using UserCallback
    = std::function<void(AudioDevicePlugEvent, const std::string&)>;

  Impl(const UserCallback& cb) : mCallback(cb) {
    UpdateDevices();
    AudioObjectAddPropertyListener(
      kAudioObjectSystemObject, &gDeviceListProp, &OSCallback, this);
  }

  ~Impl() {
    AudioObjectRemovePropertyListener(
      kAudioObjectSystemObject, &gDeviceListProp, &OSCallback, this);
  }

 private:
  UserCallback mCallback;
  std::vector<AudioDeviceID> mDevices;
  std::unordered_map<AudioDeviceID, std::vector<std::string>> mDeviceIDStrings;

  void UpdateDevices() {
    mDevices = GetAudioDeviceIDs();
    std::sort(mDevices.begin(), mDevices.end());

    for (const auto id: mDevices) {
      auto& id_strings = mDeviceIDStrings[id];
      id_strings.clear();
      if (AudioDeviceSupportsScope(id, kAudioDevicePropertyScopeInput)) {
        auto devID = MakeDeviceID(id, AudioDeviceDirection::INPUT);
        if (devID) {
          id_strings.push_back(*devID);
        }
      }
      if (AudioDeviceSupportsScope(id, kAudioDevicePropertyScopeOutput)) {
        auto devID = MakeDeviceID(id, AudioDeviceDirection::OUTPUT);
        id_strings.push_back(*devID);
      }
    }
  }

  void OnDevicesChanged() {
    const auto oldDevices = std::move(mDevices);
    UpdateDevices();
    const auto& newDevices = mDevices;

    std::vector<AudioDeviceID> added, removed;
    std::set_difference(
      oldDevices.begin(),
      oldDevices.end(),
      newDevices.begin(),
      newDevices.end(),
      std::back_inserter(removed));
    std::set_difference(
      newDevices.begin(),
      newDevices.end(),
      oldDevices.begin(),
      oldDevices.end(),
      std::back_inserter(added));

    for (const auto id: added) {
      if (AudioDeviceSupportsScope(id, kAudioDevicePropertyScopeInput)) {
        auto devID = MakeDeviceID(id, AudioDeviceDirection::INPUT);
        if (devID) {
          mCallback(AudioDevicePlugEvent::ADDED, *devID);
        }
      }
      if (AudioDeviceSupportsScope(id, kAudioDevicePropertyScopeOutput)) {
        auto devID = MakeDeviceID(id, AudioDeviceDirection::OUTPUT);
        if (devID) {
          mCallback(AudioDevicePlugEvent::ADDED, *devID);
        }
      }
    }
    for (const auto id: removed) {
      auto it = mDeviceIDStrings.find(id);
      if (it == mDeviceIDStrings.end()) {
        continue;
      }
      for (const auto& id_string: it->second) {
        mCallback(AudioDevicePlugEvent::REMOVED, id_string);
      }
      mDeviceIDStrings.erase(it);
    }
  }

  void InvokeCallback(
    AudioDevicePlugEvent event,
    const std::vector<AudioDeviceID>& ids) {
    for (const auto id: ids) {
    }
  }

  static OSStatus OSCallback(
    AudioDeviceID _id,
    UInt32 _prop_count,
    const AudioObjectPropertyAddress* _props,
    void* data) {
    reinterpret_cast<Impl*>(data)->OnDevicesChanged();
    return 0;
  }
};

AudioDevicePlugEventCallbackHandle::AudioDevicePlugEventCallbackHandle(
  const std::shared_ptr<Impl>& p)
  : p(p) {
}

AudioDevicePlugEventCallbackHandle::~AudioDevicePlugEventCallbackHandle()
  = default;

AudioDevicePlugEventCallbackHandle AddAudioDevicePlugEventCallback(
  std::function<void(AudioDevicePlugEvent, const std::string&)> userCallback) {
  return std::make_shared<AudioDevicePlugEventCallbackHandle::Impl>(
    userCallback);
}

result<VolumeRange> GetDeviceVolumeRange(const std::string&) {
  return {unexpect, Error::OPERATION_UNSUPPORTED};
}

result<Volume> GetDeviceVolume(const std::string& deviceID) {
  return {unexpect, Error::OPERATION_UNSUPPORTED};
}

result<void> SetDeviceVolumeScalar(const std::string& deviceID, float) {
  return {unexpect, Error::OPERATION_UNSUPPORTED};
}

result<void> SetDeviceVolumeDecibels(const std::string& deviceID, float) {
  return {unexpect, Error::OPERATION_UNSUPPORTED};
}

result<void> IncreaseDeviceVolume(const std::string& deviceID) {
  return {unexpect, Error::OPERATION_UNSUPPORTED};
}

result<void> DecreaseDeviceVolume(const std::string& deviceID) {
  return {unexpect, Error::OPERATION_UNSUPPORTED};
}

}// namespace FredEmmott::Audio

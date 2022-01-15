/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include "AudioDevices.h"

// Include order matters for these; don't let the autoformatter break things
// clang-format off
#include <Windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <Unknwn.h>
// clang-format on

#include <winrt/base.h>

#include "Functiondiscoverykeys_devpkey.h"
#include "PolicyConfig.h"

#pragma comment(lib,"WindowsApp.lib")

namespace FredEmmott::Audio {

namespace {

std::string Utf16ToUtf8(const std::wstring& utf16) {
  if (utf16.empty()) {
    return std::string();
  }
  return winrt::to_string(utf16);
}

std::wstring Utf8ToUtf16(const std::string& utf8) {
  if (utf8.empty()) {
    return std::wstring();
  }
  return std::wstring(winrt::to_hstring(utf8));
}

EDataFlow AudioDeviceDirectionToEDataFlow(const AudioDeviceDirection dir) {
  switch (dir) {
    case AudioDeviceDirection::INPUT:
      return eCapture;
    case AudioDeviceDirection::OUTPUT:
      return eRender;
  }
  __assume(0);
}

ERole AudioDeviceRoleToERole(const AudioDeviceRole role) {
  switch (role) {
    case AudioDeviceRole::COMMUNICATION:
      return eCommunications;
    case AudioDeviceRole::DEFAULT:
      return eConsole;
  }
  __assume(0);
}

winrt::com_ptr<IMMDevice> DeviceIDToDevice(const std::string& device_id) {
  static std::map<std::string, winrt::com_ptr<IMMDevice>> cache;
  const auto cached = cache.find(device_id);
  if (cached != cache.end()) {
    return cached->second;
  }

  auto de
    = winrt::create_instance<IMMDeviceEnumerator>(__uuidof(MMDeviceEnumerator));
  auto utf16 = Utf8ToUtf16(device_id);
  winrt::com_ptr<IMMDevice> device;
  de->GetDevice(utf16.c_str(), device.put());
  if (!device) {
    throw device_not_available_error();
  }
  cache.emplace(device_id, device);
  return device;
}

winrt::com_ptr<IAudioEndpointVolume> DeviceIDToAudioEndpointVolume(
  const std::string& device_id) {
  static std::map<std::string, winrt::com_ptr<IAudioEndpointVolume>> cache;
  const auto cached = cache.find(device_id);
  if (cached != cache.end()) {
    return cached->second;
  }
  auto device = DeviceIDToDevice(device_id);
  winrt::com_ptr<IAudioEndpointVolume> volume;
  device->Activate(
    __uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&volume);
  if (!volume) {
    throw operation_not_supported_error();
  }
  cache[device_id] = volume;
  return volume;
}

AudioDeviceState GetAudioDeviceState(winrt::com_ptr<IMMDevice> device) {
  DWORD nativeState;
  device->GetState(&nativeState);

  switch (nativeState) {
    case DEVICE_STATE_ACTIVE:
      return AudioDeviceState::CONNECTED;
    case DEVICE_STATE_DISABLED:
      return AudioDeviceState::DEVICE_DISABLED;
    case DEVICE_STATE_NOTPRESENT:
      return AudioDeviceState::DEVICE_NOT_PRESENT;
    case DEVICE_STATE_UNPLUGGED:
      return AudioDeviceState::DEVICE_PRESENT_NO_CONNECTION;
  }

  __assume(0);
}

}// namespace

AudioDeviceState GetAudioDeviceState(const std::string& id) {
  auto device = DeviceIDToDevice(id);
  if (device == nullptr) {
    return AudioDeviceState::DEVICE_NOT_PRESENT;
  }
  return GetAudioDeviceState(device);
}

std::map<std::string, AudioDeviceInfo> GetAudioDeviceList(
  AudioDeviceDirection direction) {
  auto de
    = winrt::create_instance<IMMDeviceEnumerator>(__uuidof(MMDeviceEnumerator));

  winrt::com_ptr<IMMDeviceCollection> devices;
  de->EnumAudioEndpoints(
    AudioDeviceDirectionToEDataFlow(direction),
    DEVICE_STATEMASK_ALL,
    devices.put());

  UINT deviceCount;
  devices->GetCount(&deviceCount);
  std::map<std::string, AudioDeviceInfo> out;

  for (UINT i = 0; i < deviceCount; ++i) {
    winrt::com_ptr<IMMDevice> device;
    devices->Item(i, device.put());
    LPWSTR nativeID;
    device->GetId(&nativeID);
    const auto id = Utf16ToUtf8(nativeID);
    winrt::com_ptr<IPropertyStore> properties;
    const auto propStoreResult
      = device->OpenPropertyStore(STGM_READ, properties.put());
    if (!properties) {
      continue;
    }
    PROPVARIANT nativeCombinedName;
    properties->GetValue(PKEY_Device_FriendlyName, &nativeCombinedName);
    PROPVARIANT nativeInterfaceName;
    properties->GetValue(
      PKEY_DeviceInterface_FriendlyName, &nativeInterfaceName);
    PROPVARIANT nativeEndpointName;
    properties->GetValue(PKEY_Device_DeviceDesc, &nativeEndpointName);

    if (!nativeCombinedName.pwszVal) {
      continue;
    }

    out[id] = AudioDeviceInfo {
      .id = id,
      .interfaceName = Utf16ToUtf8(nativeInterfaceName.pwszVal),
      .endpointName = Utf16ToUtf8(nativeEndpointName.pwszVal),
      .displayName = Utf16ToUtf8(nativeCombinedName.pwszVal),
      .direction = direction,
      .state = GetAudioDeviceState(device)};
  }
  return out;
}

std::string GetDefaultAudioDeviceID(
  AudioDeviceDirection direction,
  AudioDeviceRole role) {
  auto de
    = winrt::create_instance<IMMDeviceEnumerator>(__uuidof(MMDeviceEnumerator));
  if (!de) {
    return std::string();
  }
  winrt::com_ptr<IMMDevice> device;
  de->GetDefaultAudioEndpoint(
    AudioDeviceDirectionToEDataFlow(direction),
    AudioDeviceRoleToERole(role),
    device.put());
  if (!device) {
    return std::string();
  }
  LPWSTR deviceID;
  device->GetId(&deviceID);
  if (!deviceID) {
    return std::string();
  }
  return Utf16ToUtf8(deviceID);
}

void SetDefaultAudioDeviceID(
  AudioDeviceDirection direction,
  AudioDeviceRole role,
  const std::string& desiredID) {
  if (desiredID == GetDefaultAudioDeviceID(direction, role)) {
    return;
  }

  auto policyConfig = winrt::create_instance<IPolicyConfigVista>(
    __uuidof(CPolicyConfigVistaClient));
  const auto utf16 = Utf8ToUtf16(desiredID);
  policyConfig->SetDefaultEndpoint(utf16.c_str(), AudioDeviceRoleToERole(role));
}

bool IsAudioDeviceMuted(const std::string& deviceID) {
  auto volume = DeviceIDToAudioEndpointVolume(deviceID);
  if (!volume) {
    return false;
  }
  BOOL ret;
  volume->GetMute(&ret);
  return ret;
}

void MuteAudioDevice(const std::string& deviceID) {
  auto volume = DeviceIDToAudioEndpointVolume(deviceID);
  if (!volume) {
    return;
  }
  volume->SetMute(true, nullptr);
}

void UnmuteAudioDevice(const std::string& deviceID) {
  auto volume = DeviceIDToAudioEndpointVolume(deviceID);
  if (!volume) {
    return;
  }
  volume->SetMute(false, nullptr);
}

namespace {
class VolumeCOMCallback
  : public winrt::implements<VolumeCOMCallback, IAudioEndpointVolumeCallback> {
 public:
  VolumeCOMCallback(std::function<void(bool isMuted)> cb) : mCB(cb) {
  }

  ~VolumeCOMCallback() {
  }

  virtual HRESULT OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override {
    mCB(pNotify->bMuted);
    return S_OK;
  }

 private:
  std::function<void(bool)> mCB;
};

}// namespace

class MuteCallbackHandle::Impl {
 public:
  winrt::com_ptr<IAudioEndpointVolumeCallback> impl;
  winrt::com_ptr<IAudioEndpointVolume> dev;
  ~Impl() {
    dev->UnregisterControlChangeNotify(impl.get());
  }
};

MuteCallbackHandle::MuteCallbackHandle(const std::shared_ptr<Impl>& p) : p(p) {
}

MuteCallbackHandle::~MuteCallbackHandle() = default;

MuteCallbackHandle AddAudioDeviceMuteUnmuteCallback(
  const std::string& deviceID,
  std::function<void(bool isMuted)> cb) {
  auto dev = DeviceIDToAudioEndpointVolume(deviceID);
  auto impl = winrt::make<VolumeCOMCallback>(cb);
  auto ret = dev->RegisterControlChangeNotify(impl.get());
  if (ret != S_OK) {
    throw operation_not_supported_error();
  }
  return std::make_shared<MuteCallbackHandle::Impl>(impl, dev);
}

namespace {
typedef std::function<
  void(AudioDeviceDirection, AudioDeviceRole, const std::string&)>
  DefaultChangeCallbackFun;
class DefaultChangeCOMCallback
  : public winrt::implements<DefaultChangeCOMCallback, IMMNotificationClient> {
 public:
  DefaultChangeCOMCallback(DefaultChangeCallbackFun cb) : mCB(cb) {
  }

  virtual HRESULT OnDefaultDeviceChanged(
    EDataFlow flow,
    ERole winAudioDeviceRole,
    LPCWSTR defaultDeviceID) override {
    AudioDeviceRole role;
    switch (winAudioDeviceRole) {
      case ERole::eMultimedia:
        return S_OK;
      case ERole::eCommunications:
        role = AudioDeviceRole::COMMUNICATION;
        break;
      case ERole::eConsole:
        role = AudioDeviceRole::DEFAULT;
        break;
    }
    const AudioDeviceDirection direction = (flow == EDataFlow::eCapture)
      ? AudioDeviceDirection::INPUT
      : AudioDeviceDirection::OUTPUT;
    mCB(direction, role, Utf16ToUtf8(defaultDeviceID));

    return S_OK;
  };

  virtual HRESULT OnDeviceAdded(LPCWSTR pwstrDeviceId) override {
    return S_OK;
  };

  virtual HRESULT OnDeviceRemoved(LPCWSTR pwstrDeviceId) override {
    return S_OK;
  };

  virtual HRESULT OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
    override {
    return S_OK;
  };

  virtual HRESULT OnPropertyValueChanged(
    LPCWSTR pwstrDeviceId,
    const PROPERTYKEY key) override {
    return S_OK;
  };

 private:
  DefaultChangeCallbackFun mCB;
};

}// namespace

struct DefaultChangeCallbackHandle::Impl {
  winrt::com_ptr<IMMNotificationClient> impl;
  winrt::com_ptr<IMMDeviceEnumerator> enumerator;
  ~Impl() {
    enumerator->UnregisterEndpointNotificationCallback(impl.get());
  }
};

DefaultChangeCallbackHandle::DefaultChangeCallbackHandle(
  const std::shared_ptr<Impl>& p)
  : p(p) {
}

DefaultChangeCallbackHandle::~DefaultChangeCallbackHandle() = default;

DefaultChangeCallbackHandle AddDefaultAudioDeviceChangeCallback(
  DefaultChangeCallbackFun cb) {
  auto de
    = winrt::create_instance<IMMDeviceEnumerator>(__uuidof(MMDeviceEnumerator));
  if (!de) {
    return {};
  }
  auto impl = winrt::make<DefaultChangeCOMCallback>(cb);
  if (de->RegisterEndpointNotificationCallback(impl.get()) != S_OK) {
    return {};
  }

  return std::make_shared<DefaultChangeCallbackHandle::Impl>(impl, de);
}

}// namespace FredEmmott::Audio

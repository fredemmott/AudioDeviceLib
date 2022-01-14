/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include "../src/AudioDevices.h"

#include <iostream>

#ifdef _WIN32
#include <winrt/base.h>
#endif

using namespace FredEmmott::Audio;
using namespace std;

void dump_devices(AudioDeviceDirection dir) {
  const auto devices = GetAudioDeviceList(dir);
  for (const auto& [id, device]: devices) {
    cout << "\"" << device.displayName << "\"" << endl;
    cout << "\tID:        \"" << id << "\"" << endl;
    cout << "\tInterface: \"" << device.interfaceName << "\"" << endl;
    cout << "\tEndpoint:  \"" << device.endpointName << "\"" << endl;
    cout << "\tState:     ";
    switch (device.state) {
      case AudioDeviceState::CONNECTED:
        cout << "CONNECTED" << endl;
        break;
      case AudioDeviceState::DEVICE_NOT_PRESENT:
        cout << "DEVICE_NOT_PRESENT" << endl;
        break;
      case AudioDeviceState::DEVICE_DISABLED:
        cout << "DEVICE_DISABLED" << endl;
        break;
      case AudioDeviceState::DEVICE_PRESENT_NO_CONNECTION:
        cout << "DEVICE_PRESENT_NO_CONNECTION" << endl;
        break;
    }
  }
}

int main(int argc, char** argv) {
#ifdef _WIN32
  winrt::init_apartment();
#endif
  cout << "----- INPUT DEVICES -----" << endl;
  dump_devices(AudioDeviceDirection::INPUT);
  cout << "----- OUTPUT DEVICES -----" << endl;
  dump_devices(AudioDeviceDirection::OUTPUT);
  return 0;
}

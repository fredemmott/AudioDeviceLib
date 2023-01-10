/* Copyright (c) 2019-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include <AudioDevices/AudioDevices.h>

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
    cout << "\tID:\n\t\t" << id << "\"" << endl;
    cout << "\tInterface:\n\t\t\"" << device.interfaceName << "\"" << endl;
    cout << "\tEndpoint:\n\t\t\"" << device.endpointName << "\"" << endl;
    cout << "\tState:\n\t\t";
    switch (device.state) {
      case AudioDeviceState::CONNECTED:
        cout << "CONNECTED" << endl;
        break;
      case AudioDeviceState::DEVICE_PRESENT_NO_CONNECTION:
        cout << "DEVICE_PRESENT_NO_CONNECTION" << endl;
        break;
      case AudioDeviceState::DEVICE_NOT_PRESENT:
        cout << "DEVICE_NOT_PRESENT" << endl;
        continue;
      case AudioDeviceState::DEVICE_DISABLED:
        cout << "DEVICE_DISABLED" << endl;
        continue;
    }

    const auto volumeRange = GetDeviceVolumeRange(id);
    if (volumeRange) {
      cout << "\tVolume steps:\n\t\t" << volumeRange->volumeSteps << endl;
      cout << "\tVolume range:\n\t\t" << volumeRange->minDecibels << "dB to "
           << volumeRange->maxDecibels << "dB in "
           << volumeRange->incrementDecibels << "dB increments." << endl;
    }
    const auto volume = GetDeviceVolume(id);
    if (volume) {
      cout << "\tVolume:\n"
           << "\t\t" << std::lround(volume->volumeScalar * 100) << "%" << endl;
      if (volume->volumeStep) {
        cout << "\t\tStep " << *volume->volumeStep << endl;
      }
      if (volume->volumeDecibels) {
        cout << "\t\t" << *volume->volumeDecibels << "dB" << endl;
      }
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

/*
 * Volume Control
 */

#include "pamixer/pulseaudio.hh"
#include "pamixer/device.hh"

#include <cmath>
#include <iostream>

#include "rpi-radio.h"

using namespace std;

#include <pulse/pulseaudio.h>

Pulseaudio VolumeControl::pulse("rpi-radio");

int VolumeControl::increaseVolume()
{
  try
  {
    Device device = pulse.get_default_sink();
    int percent = device.volume_percent;
    percent += 5;
    if (percent > 100) percent = 100;
                  
    pa_volume_t newVolume = round( (double)percent * (double)PA_VOLUME_NORM / 100.0);

    pulse.set_volume(device, newVolume);
    return 0;
  }
  catch (const char* message)
  {
    cerr << message << endl;
    return -1;
  }
}

int VolumeControl::decreaseVolume()
{
  try
  {
    Device device = pulse.get_default_sink();
    int percent = device.volume_percent;
    percent -= 5;
    if (percent < 0) percent = 0;
                  
    pa_volume_t newVolume = round( (double)percent * (double)PA_VOLUME_NORM / 100.0);

    pulse.set_volume(device, newVolume);
    return 0;
  }
  catch (const char* message)
  {
    cerr << message << endl;
    return -1;
  }
}

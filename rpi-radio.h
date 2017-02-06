/* 
  RPI Header
*/

class Pulseaudio;

class VolumeControl {
public:
  static int increaseVolume();
  static int decreaseVolume();
private:
  VolumeControl() {};
  static Pulseaudio pulse;
};

// Simple c++ function to get mac address (linux)

#include <fstream>
#include <string>
using namespace std;

string myMacEthernetAddress () {
  string mac = "00:00:00:00:00:00";
  ifstream addressFile("/sys/class/net/eth0/address");

  if (addressFile.is_open())
  {
    getline(addressFile, mac);
    addressFile.close();
  }

  return mac;
}

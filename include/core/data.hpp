#ifndef IOP_CORE_DATA_HPP
#define IOP_CORE_DATA_HPP

#include "core/network.hpp"
#include "core/panic.hpp"

namespace iop {
struct Data {
  driver::HTTPClient http;
  driver::Wifi wifi;
  std::array<char, 32> ssid;
  std::array<char, 32> md5;
  std::array<char, 17> mac;
  Data() noexcept : ssid{0}, md5{0}, mac{0} {
    // This is a very bad hack, it allows core to access the data wherever it's stored
    // Assumes Data is never destroyed, and is only initialized once
    //iop_assert(rawData == nullptr, F("iop::Data was initialized twice"));
    //rawData = this;
  }
};

extern Data data;
}
#endif
#ifdef IOP_DESKTOP
#include "driver/cert_store.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/cert_store.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/cert_store.hpp"
#elif defined(IOP_ESP32)
#include "driver/cert_store.hpp"
#else
#error "Target not valid"
#endif

namespace driver {
auto CertList::count() const noexcept -> uint16_t {
  return this->numberOfCertificates;
}

auto CertList::cert(uint16_t index) const noexcept -> Cert {
  // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-a  rithmetic
  return Cert(this->certs[index], this->indexes[index], this->sizes[index]);
}
}
#include "driver/cert_store.hpp"

namespace driver {
CertStore::CertStore(CertList list) noexcept { (void) list; }
CertStore::~CertStore() noexcept {}
}
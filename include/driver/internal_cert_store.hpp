#ifndef IOP_DRIVER_INTERNAL_CERTSTORE_HPP
#define IOP_DRIVER_INTERNAL_CERTSTORE_HPP

// This module leaks implementation details, only import it from `cpp` files

#include "driver/cert_store.hpp"

#ifndef IOP_DESKTOP
#include "CertStoreBearSSL.h"
#undef LED_BUILTIN
#undef OUTPUT

namespace driver {
struct InternalCertStore : public BearSSL::CertStoreBase {
  BearSSL::X509List * x509;
  CertList certList;

  InternalCertStore(CertList list) noexcept: x509(nullptr), certList(list) {}
  ~InternalCertStore() noexcept {
    delete this->x509;
  }

  /// Called by libraries like `iop::Network`. Call `iop::Network::setCertList` before making a TLS connection.
  ///
  /// Panics if CertList is not set with `iop::Network::setCertList`.
  void installCertStore(br_x509_minimal_context *ctx) final;

  // These need to be static as they are called from from BearSSL C code
  // Panics if maybeCertList is None. Used to find appropriate cert for the connection

  static auto findHashedTA(void *ctx, void *hashed_dn, size_t len) -> const br_x509_trust_anchor *;
  static void freeHashedTA(void *ctx, const br_x509_trust_anchor *ta);
};
}
#endif

#endif
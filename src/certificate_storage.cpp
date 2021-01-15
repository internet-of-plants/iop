#include "certificate_storage.hpp"
#include "generated/certificates.h"
#include <memory>

// TODO: fix this mess, we shouldn't override it, but properly use it

namespace BearSSL {

void CertStore::installCertStore(br_x509_minimal_context *ctx) {
  br_x509_minimal_set_dynamic(ctx, reinterpret_cast<void *>(this), findHashedTA,
                              freeHashedTA);
}

auto CertStore::findHashedTA(void *ctx, void *hashed_dn, size_t len)
    -> const br_x509_trust_anchor * {
  auto *cs = static_cast<CertStore *>(ctx);

  // NOLINTNEXTLINE *-avoid-magic-numbers
  if ((cs == nullptr) || len != 32) {
    return nullptr;
  }

  for (int i = 0; i < numberOfCertificates; i++) {
    // NOLINTNEXTLINE *-avoid-magic-numbers
    if (memcmp_P(hashed_dn, indices[i], 32) == 0) {
      uint16_t certSize[1];
      memcpy_P(certSize, certSizes + i, 2); // NOLINT

      uint8_t *der = static_cast<uint8_t *>(malloc(certSize[0])); // NOLINT
      memcpy_P(der, certificates[i], certSize[0]);                // NOLINT
      cs->_x509 = new X509List(der, certSize[0]);                 // NOLINT
      free(der);                                                  // NOLINT

      if (cs->_x509 == nullptr) {
        return nullptr;
      }

      // NOLINTNEXTLINE
      br_x509_trust_anchor *ta = const_cast<br_x509_trust_anchor *>(
          cs->_x509->getTrustAnchors());     // NOLINT
      memcpy_P(ta->dn.data, indices[i], 32); // NOLINT
      ta->dn.len = 32;                       // NOLINT

      return ta;
    }
  }
  return nullptr;
}

void CertStore::freeHashedTA(void *ctx, const br_x509_trust_anchor *ta) {
  auto *cs = static_cast<CertStore *>(ctx);
  (void)ta;         // Unused
  delete cs->_x509; // NOLINT
  cs->_x509 = nullptr;
}

} // namespace BearSSL

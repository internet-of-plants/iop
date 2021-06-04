#ifndef IOP_DRIVER_CERTSTORE_HPP
#define IOP_DRIVER_CERTSTORE_HPP

#ifdef IOP_DESKTOP
#include <cstdint>
#include <stddef.h>
#define memcpy_P memcpy
#define memcmp_P memcmp
class br_x509_minimal_context;
class br_x509_trust_anchor {
public:
  struct { struct { uint8_t *data; size_t len; } dn; };
};
static br_x509_trust_anchor dummyAnchor;

namespace BearSSL {
class CertStoreBase {
  public:
    virtual ~CertStoreBase() {}

    // Installs the cert store into the X509 decoder (normally via static function callbacks)
    virtual void installCertStore(br_x509_minimal_context *ctx) = 0;
};
        
class X509List {
  uint8_t dummy;
public:
  X509List(const uint8_t * const cert, const uint16_t size) {
    (void)cert;
    (void)size;
  }

  const br_x509_trust_anchor* getTrustAnchors() const {
    return &dummyAnchor;
  }
};
} // namespace BearSSL

static void br_x509_minimal_set_dynamic(br_x509_minimal_context *ctx, void *dynamic_ctx,
	const br_x509_trust_anchor* (*dynamic)(void *ctx, void *hashed_dn, size_t hashed_dn_len),
        void (*dynamic_free)(void *ctx, const br_x509_trust_anchor *ta)) {
  (void)ctx;
  (void)dynamic_ctx;
  (void)dynamic;
  (void)dynamic_free;
}
#else
#include "CertStoreBearSSL.h"
#endif

#endif
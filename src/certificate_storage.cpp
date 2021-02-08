#include "certificate_storage.hpp"

#include "generated/certificates.h"
#include "tracer.hpp"
#include "utils.hpp"

constexpr const uint8_t hashSize = 32;

auto IopCertStore::findHashedTA(void *ctx, void *hashed_dn, size_t len)
    -> const br_x509_trust_anchor * {
  IOP_TRACE();

  // TODO: are this errors common of a symptom of a bug in our codebase, should
  // we log it? Check how upstream handles it
  if (ctx == nullptr || hashed_dn == nullptr || len != hashSize)
    return nullptr;
  auto *cs = static_cast<IopCertStore *>(ctx);

  for (int i = 0; i < numberOfCertificates; i++) {
    const auto size = certSizes[i]; // NOLINT *-pro-bounds-constant-array-index
    const auto *index = indices[i]; // NOLINT *-pro-bounds-constant-array-index
    const auto *cert = certificates[i]; // NOLINT *-bounds-constant-array-index

    if (memcmp_P(hashed_dn, index, hashSize) == 0) {
      auto der = try_make_unique<uint8_t[]>(size);
      if (!der) // TODO: Should we log this?
        return nullptr;

      memcpy_P(der.get(), cert, size);

      cs->x509 = try_make_unique<BearSSL::X509List>(der.get(), size);
      if (!cs->x509) // TODO: Should we log this?
        return nullptr;
      der.reset(nullptr);

      const auto *taTmp = cs->x509->getTrustAnchors();

      // We can const cast because x509 is heap allocated and we own it so it's
      // mutable. This isn't a const function. The upstream API is just that way
      // NOLINTNEXTLINE cppcoreguidelines-pro-type-const-cast
      auto *ta = const_cast<br_x509_trust_anchor *>(taTmp);

      memcpy_P(ta->dn.data, index, hashSize);
      ta->dn.len = hashSize;

      return ta;
    }
  }

  return nullptr;
}

void IopCertStore::freeHashedTA(void *ctx, const br_x509_trust_anchor *ta) {
  IOP_TRACE();
  (void)ta; // Unused

  static_cast<IopCertStore *>(ctx)->x509.reset(nullptr);
}

void IopCertStore::installCertStore(br_x509_minimal_context *ctx) {
  IOP_TRACE();
  auto *ptr = static_cast<void *>(this);
  br_x509_minimal_set_dynamic(ctx, ptr, findHashedTA, freeHashedTA);
}
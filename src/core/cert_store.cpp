#include "core/cert_store.hpp"
#include "core/panic.hpp"
#include "core/utils.hpp"
#include "driver/cert_store.hpp"

namespace iop {

constexpr const uint8_t hashSize = 32;
auto CertStore::findHashedTA(void *ctx, void *hashed_dn, size_t len)
    -> const br_x509_trust_anchor * {
  IOP_TRACE();
  auto *cs = static_cast<CertStore *>(ctx);

  iop_assert(cs, F("ctx is nullptr, this is unreachable because if this method is accessible, the ctx is set"));
  iop_assert(hashed_dn, F("hashed_dn is nullptr, this is unreachable because it's a static array"));
  iop_assert(len == hashSize, F("Invalid hash len"));

  const auto &list = cs->certList;
  for (uint16_t i = 0; i < list.count(); i++) {
    const auto cert = list.cert(i);

    if (memcmp_P(hashed_dn, cert.index, hashSize) == 0) {
      const auto size = cert.size;
      auto der = std::make_unique<uint8_t[]>(size);
      iop_assert(der, F("Cert allocation failed"));

      memcpy_P(der.get(), cert.cert, size);
      cs->x509 = std::make_unique<BearSSL::X509List>(der.get(), size);
      der.reset();

      // We can const cast because it's heap allocated
      // It shouldn't be a const function. But the upstream API is just that way
      // NOLINTNEXTLINE cppcoreguidelines-pro-type-const-cast
      iop_assert(cs->x509, F("Unable to allocate X509List"));
      const auto *taTmp = cs->x509->getTrustAnchors();
      auto *ta = const_cast<br_x509_trust_anchor *>(taTmp);
      memcpy_P(ta->dn.data, cert.index, hashSize);
      ta->dn.len = hashSize;

      return ta;
    }
  }

  return nullptr;
}

void CertStore::freeHashedTA(void *ctx, const br_x509_trust_anchor *ta) {
  IOP_TRACE();
  (void)ta; // Unused

  static_cast<CertStore *>(ctx)->x509.reset();
}

void CertStore::installCertStore(br_x509_minimal_context *ctx) {
  IOP_TRACE();
  auto *ptr = static_cast<void *>(this);
  br_x509_minimal_set_dynamic(ctx, ptr, findHashedTA, freeHashedTA);
}

auto CertList::count() const noexcept -> uint16_t {
  return this->numberOfCertificates;
}

auto CertList::cert(uint16_t index) const noexcept -> Cert {
  // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-a  rithmetic
  return Cert(this->certs[index], this->indexes[index], this->sizes[index]);
}
} // namespace iop

#ifdef IOP_DESKTOP
void br_x509_minimal_set_dynamic(br_x509_minimal_context *ctx, void *dynamic_ctx,
	const br_x509_trust_anchor* (*dynamic)(void *ctx, void *hashed_dn, size_t hashed_dn_len),
        void (*dynamic_free)(void *ctx, const br_x509_trust_anchor *ta)) {
  (void)ctx;
  (void)dynamic_ctx;
  (void)dynamic;
  (void)dynamic_free;
}
#endif
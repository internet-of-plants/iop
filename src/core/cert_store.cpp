#include "core/cert_store.hpp"
#include "core/panic.hpp"
#include "core/utils.hpp"
#include "driver/cert_store.hpp"

namespace iop {

constexpr const uint8_t hashSize = 32;

CertStore::CertStore(CertList certList) noexcept
    : certList(std::move(certList)) {
  IOP_TRACE();
}

auto CertStore::findHashedTA(void *ctx, void *hashed_dn, size_t len)
    -> const br_x509_trust_anchor * {
  IOP_TRACE();

  auto *cs = static_cast<CertStore *>(ctx);

  if (cs == nullptr)
    iop_panic(F("ctx is nullptr, this is unreachable because if this method is "
                "accessible, the ctx is set"));

  if (hashed_dn == nullptr) {
    iop_panic(F("hashed_dn is nullptr, this is unreachable because it's a "
                "static array"));
  }
  if (len != hashSize)
    iop_panic(StaticString(F("Invalid hash len, this is critical: ")).toStdString()
                + std::to_string(len));

  const auto &list = cs->certList;
  for (uint16_t i = 0; i < list.count(); i++) {
    const auto cert = list.cert(i);

    if (memcmp_P(hashed_dn, cert.index, hashSize) == 0) {
      cs->x509.emplace(cert.cert, *cert.size);
      const auto *taTmp = iop::unwrap_ref(cs->x509, IOP_CTX()).getTrustAnchors();

      // We can const cast because x509 is heap allocated and we own it so it's
      // mutable. This isn't a const function. The upstream API is just that way
      // NOLINTNEXTLINE cppcoreguidelines-pro-type-const-cast
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

Cert::Cert(const uint8_t *cert, const uint8_t *index,
           const uint16_t *size) noexcept
    : size(size), index(index), cert(cert) {
  IOP_TRACE();
}
Cert::Cert(Cert &&other) noexcept
    : size(other.size), index(other.index), cert(other.cert) {
  IOP_TRACE();
}

CertList::CertList(const uint8_t *const *certs, const uint8_t *const *indexes,
                   const uint16_t *sizes,
                   const uint16_t numberOfCertificates) noexcept
    : sizes(sizes), indexes(indexes), certs(certs),
      numberOfCertificates(numberOfCertificates) {
  IOP_TRACE();
}

auto CertList::count() const noexcept -> uint16_t {
  IOP_TRACE();
  return this->numberOfCertificates;
}

auto CertList::cert(uint16_t index) const noexcept -> Cert {
  IOP_TRACE();
  // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
  return {this->certs[index], this->indexes[index], &this->sizes[index]};
}

CertList::CertList(CertList &&other) noexcept
    : sizes(other.sizes), indexes(other.indexes), certs(other.certs),
      numberOfCertificates(other.numberOfCertificates) {
  IOP_TRACE();
}

} // namespace iop
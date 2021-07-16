#ifndef IOP_CORE_CERTSTORE_HPP
#define IOP_CORE_CERTSTORE_HPP

#include <optional>
#include "driver/cert_store.hpp"

namespace iop {

/// Individual certificate structure. Contains certificate array, its size. And
/// its hash.
///
/// Points to PROGMEM data
struct Cert {
  const uint16_t *size;
  const uint8_t *index;
  const uint8_t *cert;

  constexpr Cert(const uint8_t *cert, const uint8_t *index, const uint16_t *size) noexcept:
    size(size), index(index), cert(cert) {}
};

/// Abstracts hardcoded certificates. You must run the pre build script
/// that downloads certificates and generates a header file with them.
///
/// This will store 3 arrays (and the number of certificates). Certificates,
/// Certificates Sizes and Hashes. With that the certificate storage can be
/// installed and provide certificates as needed
///
/// You won't have to instantiate your own CertList. This should be generated
/// too.
///
/// _Only_ pass pointers _from_ the PROGMEM to this class
///
/// No pointer ownership is acquired, those pointers should point to static data
///
/// Install it with `CertStore::setCertList(...)`
/// If not installed it will iop_panic
class CertList {
  const uint16_t *sizes;
  const uint8_t *const *indexes;
  const uint8_t *const *certs;
  uint16_t numberOfCertificates;

public:
  CertList(const uint8_t *const *certs, const uint8_t *const *indexes,
           const uint16_t *sizes, uint16_t numberOfCertificates) noexcept:
      sizes(sizes), indexes(indexes), certs(certs),
      numberOfCertificates(numberOfCertificates) {}

  auto cert(uint16_t index) const noexcept -> Cert;
  auto count() const noexcept -> uint16_t;
};

/// TLS certificate storage using hardcoded certs. You must construct it from the
/// generated certs. Check the (build/preBuildCertificates.py)
///
/// Should be constructed in static. It's not copyable, nor movable.
class CertStore : public BearSSL::CertStoreBase {
  std::optional<BearSSL::X509List> x509;
  CertList certList;

public:
  explicit CertStore(CertList list) noexcept: x509{}, certList(list) {}

  CertStore(CertStore const &other) noexcept = delete;
  CertStore(CertStore &&other) noexcept = delete;
  auto operator=(CertStore const &other) noexcept -> CertStore & = delete;
  auto operator=(CertStore &&other) noexcept -> CertStore & = delete;
  ~CertStore() noexcept final = default;

  /// Called by libraries like HttpClient. Call `setCertList` before passing
  /// iop::CertStore to whichever lib is going to use it.
  ///
  /// Panics if CertList is not set.
  void installCertStore(br_x509_minimal_context *ctx) final;

private:
  // These need to be static as they are called from from BearSSL C code
  // Panic if maybeCertList is None. Used to handle data for a specific cert

  static auto findHashedTA(void *ctx, void *hashed_dn, size_t len)
      -> const br_x509_trust_anchor *;
  static void freeHashedTA(void *ctx, const br_x509_trust_anchor *ta);
};
} // namespace iop

#endif

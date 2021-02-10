#ifndef IOP_CORE_CERTSTORE_HPP
#define IOP_CORE_CERTSTORE_HPP

#include "core/memory.hpp"
#include "core/option.hpp"

#include "CertStoreBearSSL.h"

namespace iop {

/// Individual certificate structure. Contains certificate array, its size. And
/// its hash.
///
/// Points to PROGMEM data
struct Cert {
  const uint16_t *size;
  const uint8_t *index;
  const uint8_t *cert;

  ~Cert() noexcept = default;
  Cert(const uint8_t *cert, const uint8_t *index,
       const uint16_t *size) noexcept;
  Cert(Cert const &other) noexcept = default;
  Cert(Cert &&other) noexcept;
  auto operator=(Cert const &other) noexcept -> Cert & = default;
  auto operator=(Cert &&other) noexcept -> Cert & = default;
};

/// Abstracts hardcoded certificates. You must run the pre build certificates
/// downloading python script and generate your header file with certificates.
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
  const uint16_t *numberOfCertificates;

public:
  CertList(const uint8_t *const *certs, const uint8_t *const *indexes,
           const uint16_t *sizes,
           const uint16_t *numberOfCertificates) noexcept;

  auto cert(uint16_t index) const noexcept -> Cert;
  auto certificates() const noexcept -> uint16_t;

  CertList(CertList const &other) noexcept = default;
  CertList(CertList &&other) noexcept;
  auto operator=(CertList const &other) noexcept -> CertList & = default;
  auto operator=(CertList &&other) noexcept -> CertList & = default;
  ~CertList() noexcept = default;
};

/// TLS certificate storage using hardcoded certs. You must set your hardcoded
/// certs, use `setCertList`. The certList should be generated together with the
/// certificates.
///
/// Should be used as a static variable. It's not copyable, nor movable.
class CertStore : public BearSSL::CertStoreBase {
  std::unique_ptr<BearSSL::X509List> x509;
  iop::Option<CertList> maybeCertList;

public:
  CertStore() noexcept = default;
  ~CertStore() noexcept override = default;

  // This is crucial. Set this at your setup. Use the generated `certList`
  void setCertList(CertList list) noexcept {
    this->maybeCertList.emplace(list);
  }

  // Not something to copy or move around. Set to a static var and forget it
  CertStore(CertStore const &other) noexcept = delete;
  CertStore(CertStore &&other) noexcept = delete;
  auto operator=(CertStore const &other) noexcept -> CertStore & = delete;
  auto operator=(CertStore &&other) noexcept -> CertStore & = delete;

  // Panics if maybeCertList is None
  void installCertStore(br_x509_minimal_context *ctx) final;

private:
  // These need to be static as they are called from from BearSSL C code

  // Panics if maybeCertList is None. Uses it to get cert data
  static auto findHashedTA(void *ctx, void *hashed_dn, size_t len)
      -> const br_x509_trust_anchor *;
  static void freeHashedTA(void *ctx, const br_x509_trust_anchor *ta);
};
} // namespace iop

#endif

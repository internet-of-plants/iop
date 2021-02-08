#ifndef IOP_CERTSTORE_HPP
#define IOP_CERTSTORE_HPP

#include "CertStoreBearSSL.h"
#include <memory>

/// TLS certificate storage using hardcoded certs
class IopCertStore : public BearSSL::CertStoreBase {
public:
  IopCertStore() noexcept = default;
  ~IopCertStore() noexcept override = default;

  // We create it as a static and that's it
  IopCertStore(IopCertStore const &other) noexcept = delete;
  IopCertStore(IopCertStore &&other) noexcept = delete;
  auto operator=(IopCertStore const &other) noexcept -> IopCertStore & = delete;
  auto operator=(IopCertStore &&other) noexcept -> IopCertStore & = delete;

  // Installs cert store into the X509 decoder, normally via static callbacks
  void installCertStore(br_x509_minimal_context *ctx) final;

private:
  std::unique_ptr<BearSSL::X509List> x509;

  // These need to be static as they are called from from BearSSL C code
  static auto findHashedTA(void *ctx, void *hashed_dn, size_t len)
      -> const br_x509_trust_anchor *;
  static void freeHashedTA(void *ctx, const br_x509_trust_anchor *ta);
};

#endif

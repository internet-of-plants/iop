#ifndef IOP_API_H_
#define IOP_API_H_

#include <models.hpp>
#include <utils.hpp>

class Api {
  private:
    String host_;
  public:
    Api(const String host): host_(host) {}
    bool registerEvent(const AuthToken token, const Event event) const;
    Option<bool> doWeOwnsThisPlant(const String token, const String plantId) const;
    Option<PlantId> registerPlant(const String token, const String macAddress) const;
    Option<AuthToken> authenticate(const String username, const String password) const;
    String host() const { return this->host_; }
};

#endif
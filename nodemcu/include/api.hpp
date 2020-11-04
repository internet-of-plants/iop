#ifndef IOP_API_H_
#define IOP_API_H_

#include <models.hpp>
#include <utils.hpp>

void apiSetup();
int sendEvent(const String token, const Event event);
bool doWeOwnsThisPlant(const String token, const String plantId);
Option<PlantId> getPlantId(const String token, const String macAddress);
Option<AuthToken> generateToken(const String username, const String password);

#endif
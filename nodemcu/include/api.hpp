#ifndef IOP_API_H_
#define IOP_API_H_

#include <Arduino.h>
#include <models.hpp>
#include <utils.hpp>

bool sendEvent(const String token, const Event event);
Option<String> generateToken();

#endif
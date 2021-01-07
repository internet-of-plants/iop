#include "utils.hpp"
#include "Esp.cpp"
#include "fixed_string.hpp"
#include "models.hpp"

MD5Hash hashSketch() noexcept {
  static MD5Hash result = MD5Hash::empty();
  if (result.asString().length()) {
    return result;
  }
  uint32_t lengthLeft = ESP.getSketchSize();
  constexpr const size_t bufSize = 512;
  FixedString<bufSize> buf = FixedString<bufSize>::empty();
  uint32_t offset = 0;
  MD5Builder md5;
  md5.begin();
  while (lengthLeft > 0) {
    size_t readBytes = (lengthLeft < bufSize) ? lengthLeft : bufSize;
    if (!ESP.flashRead(offset, reinterpret_cast<uint32_t *>(buf.asMut()),
                       (readBytes + 3) & ~3)) {
      return result;
    }
    md5.add(buf.asStorage().constPtr(), readBytes);
    lengthLeft -= readBytes;
    offset += readBytes;
  }
  md5.calculate();
  md5.getBytes(result.mutPtr());
  return result;
}

#include "StreamString.h"

/**
 * write Update to flash
 * @param in Stream&
 * @param size uint32_t
 * @param md5 String
 * @return true if Update ok
 */
bool runUpdate(Stream &in, uint32_t size, const String &md5, int command) {

  StreamString error;

  if (!Update.begin(size, command, LED_BUILTIN, HIGH)) {
    //_setLastError(Update.getError());
    Update.printError(error);
    error.trim(); // remove line ending
    // DEBUG_HTTP_UPDATE("[httpUpdate] Update.begin failed! (%s)\n",
    //                  error.c_str());
    return false;
  }

  if (md5.length()) {
    if (!Update.setMD5(md5.c_str())) {
      //_setLastError(HTTP_UE_SERVER_FAULTY_MD5);
      // DEBUG_HTTP_UPDATE("[httpUpdate] Update.setMD5 failed! (%s)\n",
      //                  md5.c_str());
      return false;
    }
  }

  if (Update.writeStream(in) != size) {
    //_setLastError(Update.getError());
    Update.printError(error);
    error.trim(); // remove line ending
    // DEBUG_HTTP_UPDATE("[httpUpdate] Update.writeStream failed! (%s)\n",
    //                  error.c_str());
    return false;
  }

  if (!Update.end()) {
    //_setLastError(Update.getError());
    Update.printError(error);
    error.trim(); // remove line ending
    // DEBUG_HTTP_UPDATE("[httpUpdate] Update.end failed! (%s)\n",
    // error.c_str());
    return false;
  }

  return true;
}

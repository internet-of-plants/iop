#include "utils.hpp"
#include "Esp.cpp"
#include "fixed_string.hpp"
#include "models.hpp"

auto hashSketch() noexcept -> MD5Hash {
  static MD5Hash result = MD5Hash::empty();
  if (result.asString().length() != 0U) {
    return result;
  }
  uint32_t lengthLeft = ESP.getSketchSize();
  constexpr const size_t bufSize = 512;
  FixedString<bufSize> buf = FixedString<bufSize>::empty();
  uint32_t offset = 0;
  MD5Builder md5 = {};
  md5.begin();
  while (lengthLeft > 0) {
    size_t readBytes = (lengthLeft < bufSize) ? lengthLeft : bufSize;
    if (!ESP.flashRead(offset, reinterpret_cast<uint32_t *>(buf.asMut()),
                       (readBytes + 3) & ~3)) { // NOLINT hicpp-signed-bitwise
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
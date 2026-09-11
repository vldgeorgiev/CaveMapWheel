#pragma once

#include "device.hpp"

namespace cmw::diagnostics {

void begin();
void emit(const DeviceSnapshot& snapshot, bool ble_connected);

}  // namespace cmw::diagnostics

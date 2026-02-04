#pragma once

#include <cstdint>

namespace naviga {

double distance_m_e7(int32_t lat1_e7, int32_t lon1_e7, int32_t lat2_e7, int32_t lon2_e7);
double bearing_deg_e7(int32_t lat1_e7, int32_t lon1_e7, int32_t lat2_e7, int32_t lon2_e7);

} // namespace naviga

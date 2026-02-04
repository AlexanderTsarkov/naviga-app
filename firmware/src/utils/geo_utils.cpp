#include "utils/geo_utils.h"

#include <cmath>

namespace naviga {

namespace {

constexpr double kEarthRadiusM = 6371000.0;

double to_rad_e7(int32_t deg_e7) {
  return (deg_e7 / 1e7) * M_PI / 180.0;
}

} // namespace

// Equirectangular approximation for local distances.
double distance_m_e7(int32_t lat1_e7, int32_t lon1_e7, int32_t lat2_e7, int32_t lon2_e7) {
  const double lat1 = to_rad_e7(lat1_e7);
  const double lat2 = to_rad_e7(lat2_e7);
  const double lon1 = to_rad_e7(lon1_e7);
  const double lon2 = to_rad_e7(lon2_e7);

  const double x = (lon2 - lon1) * std::cos((lat1 + lat2) * 0.5);
  const double y = (lat2 - lat1);
  return std::sqrt(x * x + y * y) * kEarthRadiusM;
}

double bearing_deg_e7(int32_t lat1_e7, int32_t lon1_e7, int32_t lat2_e7, int32_t lon2_e7) {
  const double lat1 = to_rad_e7(lat1_e7);
  const double lat2 = to_rad_e7(lat2_e7);
  const double lon1 = to_rad_e7(lon1_e7);
  const double lon2 = to_rad_e7(lon2_e7);

  const double x = (lon2 - lon1) * std::cos((lat1 + lat2) * 0.5);
  const double y = (lat2 - lat1);
  const double bearing_rad = std::atan2(x, y);
  double bearing_deg = bearing_rad * 180.0 / M_PI;
  if (bearing_deg < 0.0) {
    bearing_deg += 360.0;
  }
  return bearing_deg;
}

} // namespace naviga

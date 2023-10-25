#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

namespace lemlib {
namespace PathFileSystem {

struct Waypoint {
        int16_t x; // Signed Integer, 0.5mm/bit, range: -16384mm ~ +16383.5mm
        int16_t y; // Signed Integer, 0.5mm/bit, range: -16384mm ~ +16383.5mm
        int16_t speed; // Signed Integer, mm/s/bit, range: -32768mm/s ~ +32767mm/s
        uint16_t heading; // Unsigned Integer , 0.0001rad/bit, range: 0rad ~ 6.2832rad
        int16_t lookahead; // Signed Integer, 0.5mm/bit, range: -16384mm ~ +16383.5mm
        bool isHeadingAvailable : 1;
        bool isLookaheadAvailable : 1;
};

class Path {
    public:
        std::string name;
        std::vector<Waypoint> waypoints;

        Path() = default;
};

class PathFile {
    public:
        std::vector<Path> paths;

        PathFile() = default;
};

bool decode(const uint8_t* fileBuffer, const size_t fileSize, PathFile& output);

} // namespace PathFileSystem
} // namespace lemlib

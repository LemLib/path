
#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <cstring>
#include "pathFileSystem.hpp"

class ArrayStreamBuffer : public std::streambuf {
    private:
        uint8_t* array;
        std::size_t size;
    public:
        ArrayStreamBuffer(uint8_t* array, std::size_t size) : array(array), size(size) {
            char* start = reinterpret_cast<char*>(array);
            char* end = reinterpret_cast<char*>(array + size);
            setg(start, start, end);
            setp(start, end);
        }

        virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
            std::size_t remaining = size - (pptr() - pbase());
            std::streamsize to_write = std::min(n, static_cast<std::streamsize>(remaining));
            std::copy(s, s + to_write, pptr());
            pbump(static_cast<int>(to_write));
            return to_write;
        }
};

template <class T> T readBuf(T& item, std::istream& in) {
    in.read(reinterpret_cast<char*>(&item), sizeof(T));
    return item;
}

static void readBuf(uint8_t* buf, const size_t size, std::istream& in) { in.read(reinterpret_cast<char*>(buf), size); }

static std::string readNTBS(size_t maxSize, std::istream& in) {
    std::string rtn = "";
    char now;
    for (size_t i = 0; i < maxSize; i++) {
        readBuf(now, in);
        if (now == (char)0x00) break;
        rtn += now;
    }
    return rtn;
}

namespace lemlib {
namespace PathFileSystem {

using namespace lemlib;

bool decode(const uint8_t* fileBuffer, const size_t fileSize, PathFile& output) {
    try {
        uint8_t metadata[256];
        uint8_t metadataSize;
        uint16_t pathCount;
        uint32_t waypointCount;
        uint8_t flag;
        uint16_t ignored;

        ArrayStreamBuffer sbuf((uint8_t*)fileBuffer, fileSize);
        std::istream in(&sbuf);

        // Start reading metadata
        readBuf(metadataSize, in);
        readBuf(metadata, metadataSize, in);

        // End reading metadata

        readBuf(pathCount, in);

        for (int i = 0; i < pathCount; i++) {
            Path p;

            p.name = readNTBS(1024, in);

            // Start reading metadata
            readBuf(metadataSize, in);
            readBuf(metadata, metadataSize, in);

            // End reading metadata

            readBuf(waypointCount, in);

            for (int j = 0; j < waypointCount; j++) {
                Waypoint w;

                readBuf(flag, in);
                readBuf(w.x, in);
                readBuf(w.y, in);
                readBuf(w.speed, in);

                if ((w.isHeadingAvailable = (flag & 0x01) != 0)) readBuf(w.heading, in);
                if ((w.isLookaheadAvailable = (flag & 0x02) != 0)) readBuf(w.lookahead, in);
                if ((flag & 0x04) != 0) readBuf(ignored, in);
                if ((flag & 0x08) != 0) readBuf(ignored, in);
                if ((flag & 0x10) != 0) readBuf(ignored, in);
                if ((flag & 0x20) != 0) readBuf(ignored, in);
                if ((flag & 0x40) != 0) readBuf(ignored, in);
                if ((flag & 0x80) != 0) readBuf(ignored, in);

                p.waypoints.push_back(w);
            }

            output.paths.push_back(p);
        }

        return true;
    } catch (std::exception& e) { return false; }
}

} // namespace PathFileSystem
} // namespace lemlib
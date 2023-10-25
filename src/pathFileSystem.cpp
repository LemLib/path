
#include "pathFileSystem.hpp"
#include "byteBuffer.hpp"

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

template <class T> void writeBuf(const T& item, std::ostream& out) {
    out.write(reinterpret_cast<const char*>(&item), sizeof(T));
}

static void readBuf(uint8_t* buf, const size_t size, std::istream& in) { in.read(reinterpret_cast<char*>(buf), size); }

static void writeBuf(const uint8_t* buf, const size_t size, std::ostream& out) {
    out.write(reinterpret_cast<const char*>(buf), size);
}

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

static void writeNTBS(const std::string& str, std::ostream& out) {
    const char zero = (char)0x00;
    for (size_t i = 0; i < str.size(); i++) writeBuf(str[i], out);
    writeBuf(zero, out);
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

        ByteBuffer buf = ByteBuffer::wrap(fileSize, (char*)fileBuffer);

        // Start reading metadata
        metadataSize = buf.get<uint8_t>();
        buf.get((char*)metadata, metadataSize);

        // End reading metadata

        pathCount = buf.get<uint16_t>();

        for (int i = 0; i < pathCount; i++) {
            Path p;

            p.name = buf.getNTBS();

            // Start reading metadata
            metadataSize = buf.get<uint8_t>();
            buf.get((char*)metadata, metadataSize);

            // End reading metadata

            waypointCount = buf.get<uint32_t>();

            for (int j = 0; j < waypointCount; j++) {
                Waypoint w;

                flag = buf.get<uint8_t>();
                w.x = buf.get<int16_t>();
                w.y = buf.get<int16_t>();
                w.speed = buf.get<int16_t>();

                if (w.isHeadingAvailable = (flag & 0x01) != 0) w.heading = buf.get<uint16_t>();
                if (w.isLookaheadAvailable = (flag & 0x02) != 0) w.lookahead = buf.get<int16_t>();
                if ((flag & 0x04) != 0) buf.get<uint16_t>();
                if ((flag & 0x08) != 0) buf.get<uint16_t>();
                if ((flag & 0x10) != 0) buf.get<uint16_t>();
                if ((flag & 0x20) != 0) buf.get<uint16_t>();
                if ((flag & 0x40) != 0) buf.get<uint16_t>();
                if ((flag & 0x80) != 0) buf.get<uint16_t>();

                p.waypoints.push_back(w);
            }

            output.paths.push_back(p);
        }

        return true;
    } catch (std::exception& e) { return false; }
}

bool decode2(const uint8_t* fileBuffer, const size_t fileSize, PathFile& output) {
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

bool encode(const PathFile& input, uint8_t* fileBuffer, size_t& fileSize) {
    try {
        uint8_t metadata[256];
        uint8_t metadataSize;
        uint16_t pathCount;
        uint32_t waypointCount;
        uint8_t flag;

        ByteBuffer buf = ByteBuffer::wrap(fileSize, (char*)fileBuffer);

        // Start writing metadata
        metadataSize = 0;
        buf.put(metadataSize);
        buf.put((char*)metadata, metadataSize);

        // End writing metadata

        pathCount = input.paths.size();
        buf.put(pathCount);

        for (int i = 0; i < pathCount; i++) {
            const Path& p = input.paths[i];

            buf.putNTBS(p.name);

            // Start writing metadata
            metadataSize = 0;
            buf.put(metadataSize);
            buf.put((char*)metadata, metadataSize);

            // End writing metadata

            waypointCount = p.waypoints.size();
            buf.put(waypointCount);

            for (int j = 0; j < waypointCount; j++) {
                const Waypoint& w = p.waypoints[j];

                flag = 0;
                if (w.isHeadingAvailable) flag |= 0x01;
                if (w.isLookaheadAvailable) flag |= 0x02;
                buf.put(flag);
                buf.put(w.x);
                buf.put(w.y);
                buf.put(w.speed);
                if (w.isHeadingAvailable) buf.put(w.heading);
                if (w.isLookaheadAvailable) buf.put(w.lookahead);
                if ((flag & 0x04) != 0) buf.put<uint16_t>(0);
                if ((flag & 0x08) != 0) buf.put<uint16_t>(0);
                if ((flag & 0x10) != 0) buf.put<uint16_t>(0);
                if ((flag & 0x20) != 0) buf.put<uint16_t>(0);
                if ((flag & 0x40) != 0) buf.put<uint16_t>(0);
                if ((flag & 0x80) != 0) buf.put<uint16_t>(0);
            }
        }

        fileSize = buf.position();

        return true;
    } catch (std::exception& e) { return false; }
}

bool encode2(const PathFile& input, uint8_t* fileBuffer, size_t& fileSize) {
    try {
        uint16_t zero16 = 0;
        uint8_t metadata[256];
        uint8_t metadataSize;
        uint16_t pathCount;
        uint32_t waypointCount;
        uint8_t flag;

        ArrayStreamBuffer sbuf(fileBuffer, fileSize);
        std::ostream out(&sbuf);

        // Start writing metadata
        metadataSize = 0;
        writeBuf(metadataSize, out);
        writeBuf(metadata, metadataSize, out);

        // End writing metadata

        pathCount = input.paths.size();
        writeBuf(pathCount, out);

        for (int i = 0; i < pathCount; i++) {
            const Path& p = input.paths[i];

            writeNTBS(p.name, out);

            // Start writing metadata
            metadataSize = 0;
            writeBuf(metadataSize, out);
            writeBuf(metadata, metadataSize, out);

            // End writing metadata

            waypointCount = p.waypoints.size();
            writeBuf(waypointCount, out);

            for (int j = 0; j < waypointCount; j++) {
                const Waypoint& w = p.waypoints[j];

                flag = 0;
                if (w.isHeadingAvailable) flag |= 0x01;
                if (w.isLookaheadAvailable) flag |= 0x02;
                writeBuf(flag, out);
                writeBuf(w.x, out);
                writeBuf(w.y, out);
                writeBuf(w.speed, out);
                if (w.isHeadingAvailable) writeBuf(w.heading, out);
                if (w.isLookaheadAvailable) writeBuf(w.lookahead, out);
                if ((flag & 0x04) != 0) writeBuf<uint16_t>(zero16, out);
                if ((flag & 0x08) != 0) writeBuf<uint16_t>(zero16, out);
                if ((flag & 0x10) != 0) writeBuf<uint16_t>(zero16, out);
                if ((flag & 0x20) != 0) writeBuf<uint16_t>(zero16, out);
                if ((flag & 0x40) != 0) writeBuf<uint16_t>(zero16, out);
                if ((flag & 0x80) != 0) writeBuf<uint16_t>(zero16, out);
            }
        }

        out.flush();
        return true;
    } catch (std::exception& e) { return false; }
}

} // namespace PathFileSystem
} // namespace lemlib
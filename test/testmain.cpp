#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "byteBuffer.hpp"
#include "pathFileSystem.hpp"

using namespace lemlib;
using namespace lemlib::PathFileSystem;

using namespace std;

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

template <class T> void writeBuf(const T& item, std::ostream& out) {
    out.write(reinterpret_cast<const char*>(&item), sizeof(T));
}

static void writeBuf(const uint8_t* buf, const size_t size, std::ostream& out) {
    out.write(reinterpret_cast<const char*>(buf), size);
}

static void writeNTBS(const std::string& str, std::ostream& out) {
    const char zero = (char)0x00;
    for (size_t i = 0; i < str.size(); i++) writeBuf(str[i], out);
    writeBuf(zero, out);
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

bool encodeUsingStream(const PathFile& input, uint8_t* fileBuffer, size_t& fileSize) {
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

TEST_CASE("test encode & decode") {
    PathFile pf;

    // make 100 random paths, with 100 to 1000 random waypoints
    for (int i = 0; i < 100; i++) {
        Path p;
        p.name = "Path " + to_string(i);

        for (int j = 0; j < rand() % 900 + 100; j++) {
            Waypoint w;
            w.x = rand() % 32768 - 16384;
            w.y = rand() % 32768 - 16384;
            w.speed = rand() % 65536 - 32768;
            w.heading = rand() % 65536;
            w.lookahead = rand() % 32768 - 16384;
            w.isHeadingAvailable = rand() % 2;
            w.isLookaheadAvailable = rand() % 2;
            p.waypoints.push_back(w);
        }

        pf.paths.push_back(p);
    }

    uint8_t* buf = new uint8_t[1024 * 1024 * 10];

    size_t size = 1024 * 1024 * 10;
    REQUIRE(encode(pf, buf, size));

    std::cout << "size: " << size << std::endl;

    PathFile pf2;
    REQUIRE(decode(buf, size, pf2));

    // Check if the two PathFile objects are equal

    REQUIRE(pf.paths.size() == pf2.paths.size());

    for (int i = 0; i < pf.paths.size(); i++) {
        REQUIRE(pf.paths[i].name == pf2.paths[i].name);
        REQUIRE(pf.paths[i].waypoints.size() == pf2.paths[i].waypoints.size());

        for (int j = 0; j < pf.paths[i].waypoints.size(); j++) {
            REQUIRE(pf.paths[i].waypoints[j].x == pf2.paths[i].waypoints[j].x);
            REQUIRE(pf.paths[i].waypoints[j].y == pf2.paths[i].waypoints[j].y);
            REQUIRE(pf.paths[i].waypoints[j].speed == pf2.paths[i].waypoints[j].speed);
            REQUIRE(pf.paths[i].waypoints[j].isHeadingAvailable == pf2.paths[i].waypoints[j].isHeadingAvailable);
            if (pf.paths[i].waypoints[j].isHeadingAvailable)
                REQUIRE(pf.paths[i].waypoints[j].heading == pf2.paths[i].waypoints[j].heading);
            REQUIRE(pf.paths[i].waypoints[j].isLookaheadAvailable == pf2.paths[i].waypoints[j].isLookaheadAvailable);
            if (pf.paths[i].waypoints[j].isLookaheadAvailable)
                REQUIRE(pf.paths[i].waypoints[j].lookahead == pf2.paths[i].waypoints[j].lookahead);
        }
    }
}

TEST_CASE("benchmark encode & decode") {
    // SKIP("benchmark");
    PathFile pf;

    // make 100 random paths, with 100 to 1000 random waypoints
    for (int i = 0; i < 100; i++) {
        Path p;
        p.name = "Path " + to_string(i);

        for (int j = 0; j < 1000; j++) {
            Waypoint w;
            w.x = rand() % 32768 - 16384;
            w.y = rand() % 32768 - 16384;
            w.speed = rand() % 65536 - 32768;
            w.heading = rand() % 65536;
            w.lookahead = rand() % 32768 - 16384;
            w.isHeadingAvailable = rand() % 2;
            w.isLookaheadAvailable = rand() % 2;
            p.waypoints.push_back(w);
        }

        pf.paths.push_back(p);
    }

    uint8_t* buf = new uint8_t[1024 * 1024 * 10];

    size_t size = 1024 * 1024 * 10;
    BENCHMARK("encode") { encode(pf, buf, size); };

    PathFile pf2;
    BENCHMARK("decode") { decode(buf, size, pf2); };
}

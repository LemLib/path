#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "byteBuffer.hpp"
#include "pathFileSystem.hpp"

using namespace lemlib;
using namespace lemlib::PathFileSystem;

using namespace std;

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
    REQUIRE(encode2(pf, buf, size));

    std::cout << "size: " << size << std::endl;

    PathFile pf2;
    // REQUIRE(decode(buf, size, pf2));
    REQUIRE(decode2(buf, size, pf2));

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

    BENCHMARK("encode2") { encode2(pf, buf, size); };

    PathFile pf2;
    BENCHMARK("decode") { decode(buf, size, pf2); };

    PathFile pf3;
    BENCHMARK("decode2") { decode2(buf, size, pf3); };
}

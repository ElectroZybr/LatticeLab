// Hot paths of the boundaries library: run `xmake run Boundaries.bench [name]`.
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include <BmRunner/Benchmarks.hpp>
#include <Boundaries/Geometry.hpp>
#include <Boundaries/Space.hpp>
#include <Boundaries/Walls.hpp>

using namespace Boundaries;

namespace {

const std::vector<size_t> Sizes = {1'000, 10'000, 100'000, 1'000'000};

/// Points spread over a box larger than the space: wraps and walls both happen.
struct Points : Lattice::Fixture {
    explicit Points(size_t n) : x(n), y(n), z(n), out(n) {
        std::mt19937 random(1);
        std::uniform_real_distribution<float> coordinate(-15, 25);
        for (size_t i = 0; i < n; ++i)
            sx.push_back(coordinate(random)), sy.push_back(coordinate(random)), sz.push_back(coordinate(random));
    }
    void prepare() override { x = sx, y = sy, z = sz; }

    std::vector<float> x, y, z, out;
    std::vector<float> sx, sy, sz;
};

/// A test tube with a mixer: capsule region minus a blended paddle, rounded.
Shape tube() {
    auto vessel = Shape::capsule({5, 1, 5}, {5, 9, 5}, 4);
    auto shaft = Shape::capsule({5, 3, 5}, {5, 10, 5}, 0.3f);
    auto paddle = std::move(Shape::box({2, 0.3f, 0.2f})).translated({5, 3, 5});
    return std::move(vessel) - std::move(Shape::smoothUnion(std::move(shaft), std::move(paddle), 0.2f)).rounded(0.05f);
}

}

BENCH_GROUP(Space) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Space, WrapColumns, Points, "wrap SoA columns, three periodic axes") {
    const Space space{{0, 0, 0}, {10, 10, 10}, {true, true, true}};
    bench.measure<Points>([&](Points& points) { space.wrap(points.x, points.y, points.z); });
}

BENCH_GROUPED(Space, MinimumImage, Points, "minimum image of consecutive separations") {
    const Space space{{0, 0, 0}, {10, 10, 10}, {true, true, true}};
    bench.measure<Points>([&](Points& points) {
        float sum = 0;
        for (size_t i = 1; i < points.x.size(); ++i)
            sum += space.minimumImage({points.x[i] - points.x[i - 1], points.y[i] - points.y[i - 1],
                points.z[i] - points.z[i - 1]}).x;
        return sum;
    });
}

BENCH_GROUP(Geometry) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Geometry, EvaluateBlocks, Points, "tube with a mixer, SoA in blocks of 64") {
    const Geometry geometry{tube()};
    bench.measure<Points>([&](Points& points) { geometry.evaluate(points.x, points.y, points.z, points.out); });
}

BENCH_GROUPED(Geometry, SamplePoints, Points, "same program, one point at a time") {
    const Geometry geometry{tube()};
    bench.measure<Points>([&](Points& points) {
        float sum = 0;
        for (size_t i = 0; i < points.x.size(); ++i)
            sum += geometry.distance({points.x[i], points.y[i], points.z[i]});
        return sum;
    });
}

BENCH_GROUPED(Geometry, Normals, Points, "normal per point (four evaluations)") {
    const Geometry geometry{tube()};
    bench.measure<Points>([&](Points& points) {
        float sum = 0;
        for (size_t i = 0; i < points.x.size(); ++i)
            sum += geometry.normal({points.x[i], points.y[i], points.z[i]}).x;
        return sum;
    });
}

namespace {

/// Particles inside and around the tube: most in the region, some in the walls.
struct TubeParticles : Lattice::Fixture {
    explicit TubeParticles(size_t n) : x(n), y(n), z(n), vx(n), vy(n), vz(n), fx(n), fy(n), fz(n) {
        std::mt19937 random(2);
        std::uniform_real_distribution<float> radial(0, 4.4f), angle(0, 6.2831853f), height(0, 10), speed(-1, 1);
        for (size_t i = 0; i < n; ++i) {
            const float r = radial(random), a = angle(random);
            sx.push_back(5 + r * std::cos(a)), sy.push_back(height(random)), sz.push_back(5 + r * std::sin(a));
            svx.push_back(speed(random)), svy.push_back(speed(random)), svz.push_back(speed(random));
        }
    }
    void prepare() override {
        x = sx, y = sy, z = sz, vx = svx, vy = svy, vz = svz;
        std::fill(fx.begin(), fx.end(), 0.0f), std::fill(fy.begin(), fy.end(), 0.0f), std::fill(fz.begin(), fz.end(), 0.0f);
    }
    Boundaries::ParticleColumns columns() { return {x, y, z, vx, vy, vz, fx, fy, fz}; }

    std::vector<float> x, y, z, vx, vy, vz, fx, fy, fz;
    std::vector<float> sx, sy, sz, svx, svy, svz;
};

}

BENCH_GROUP(Walls) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Walls, Constrain, TubeParticles, "hard tube walls: project and reflect the ~10% in walls") {
    const Geometry geometry{tube()};
    const Walls walls{geometry};
    bench.measure<TubeParticles>([&](TubeParticles& particles) { walls.constrain(particles.columns()); });
}

BENCH_GROUPED(Walls, SoftForces, TubeParticles, "soft tube walls, layer 0.5 thick") {
    const Geometry geometry{tube()};
    const Walls walls{geometry, {Laws::Soft::make(10, 0.5f, 6)}};
    bench.measure<TubeParticles>([&](TubeParticles& particles) { walls.addForces(particles.columns()); });
}

int main(int argc, char** argv) {
    if (argc > 1)
        Lattice::Benchmarks::run(argv[1]);
    else
        Lattice::Benchmarks::runAll();
}

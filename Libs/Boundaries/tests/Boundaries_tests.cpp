// Space and geometry on the CPU: no runtime, no GPU.
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <Boundaries/Checks.hpp>
#include <Boundaries/Geometry.hpp>
#include <Boundaries/Walls.hpp>
#include <Boundaries/Space.hpp>
#include <Lattice/Tools/Exception.hpp>

using namespace Boundaries;

namespace {

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::abs(a - b) <= tolerance; }
bool near(glm::vec3 a, glm::vec3 b, float tolerance = 1e-4f) { return glm::length(a - b) <= tolerance; }

template<class F>
bool throws(F&& f) {
    try { f(); } catch (const Lattice::ExceptionBase&) { return true; }
    return false;
}

void spaceWrapsAndTakesMinimumImage() {
    const Space space{{0, 0, 0}, {10, 10, 10}, {true, false, true}};
    check(near(space.wrap({23.5f, 0, 0}).x, 3.5f) && near(space.wrap({-0.5f, 0, 0}).x, 9.5f),
          "wrap from several periods away");
    check(space.wrap({10.0f, 0, 0}).x == 0.0f, "the upper bound wraps to the lower one");
    check(space.wrap({0, 23.5f, 0}).y == 23.5f, "a non-periodic axis is untouched");
    check(near(space.minimumImage({9, 9, -6}), {-1, 9, 4}), "minimum image on periodic axes only");
    check(space.contains({0, 0, 0}) && !space.contains({10, 5, 5}), "half-open box");

    std::vector<float> x{23.5f, -0.5f, 3}, y{1, 2, 3}, z{-7, 15, 0};
    space.wrap(x, y, z);
    check(near(x[0], 3.5f) && near(x[1], 9.5f) && near(z[0], 3) && near(z[1], 5) && y[1] == 2,
          "columns wrap like points");
    check(throws([] { Space{{0, 0, 0}, {1, 0, 1}}; }), "zero size is rejected");
}

void primitivesGiveExactDistances() {
    const Geometry sphere{Shape::sphere(1)};
    check(near(sphere.distance({0, 0, 0}), 1) && near(sphere.distance({2, 0, 0}), -1),
          "sphere: positive inside the region, negative in the wall");

    const Geometry box{Shape::box({1, 2, 3})};
    check(near(box.distance({0, 0, 0}), 1) && near(box.distance({2, 0, 0}), -1), "box faces");
    check(near(box.distance({2, 3, 0}), -std::sqrt(2.0f)), "box edge");

    const Geometry capsule{Shape::capsule({0, -1, 0}, {0, 1, 0}, 0.5f)};
    check(near(capsule.distance({0, 0, 0}), 0.5f) && near(capsule.distance({0, 2, 0}), -0.5f), "capsule");

    const Geometry cylinder{Shape::cylinder(1, 1)};
    check(near(cylinder.distance({0, 1.5f, 0}), -0.5f) && near(cylinder.distance({2, 0, 0}), -1), "cylinder");

    const Geometry floor{Shape::halfSpace({0, 2, 0}, 0)};
    check(near(floor.distance({0, -2, 0}), 2) && near(floor.distance({0, 3, 0}), -3), "half-space");

    const Geometry pipe{Shape::infiniteCylinder(1)};
    check(near(pipe.distance({0, 100, 0}), 1), "infinite cylinder along y");

    check(throws([] { Shape::sphere(0); }) && throws([] { Shape::box({1, -1, 1}); }), "invalid sizes");
}

void booleansAndBlends() {
    const auto two = [] { return Shape::sphere(1) | std::move(Shape::sphere(1)).translated({3, 0, 0}); };
    check(near(Geometry{two()}.distance({3, 0, 0}), 1), "union keeps both spheres");

    // Box minus a sphere: the hole is wall; next to it the hole's surface is the nearest.
    const Geometry holed{Shape::box({1, 1, 1}) - Shape::sphere(0.5f)};
    check(near(holed.distance({0, 0, 0}), -0.5f) && near(holed.distance({0.75f, 0, 0}), 0.25f), "subtraction");

    const Geometry lens{Shape::sphere(1) & std::move(Shape::sphere(1)).translated({1, 0, 0})};
    check(near(lens.distance({0.5f, 0, 0}), 0.5f) && lens.distance({-0.5f, 0, 0}) < 0, "intersection");

    // A blend only adds material between the shapes.
    const Geometry blended{Shape::smoothUnion(Shape::sphere(1), std::move(Shape::sphere(1)).translated({2.5f, 0, 0}), 1)};
    check(blended.distance({1.25f, 0, 0}) > Geometry{Shape::sphere(1) |
          std::move(Shape::sphere(1)).translated({2.5f, 0, 0})}.distance({1.25f, 0, 0}), "smooth union fills the gap");

    check(near(Geometry{std::move(Shape::box({1, 1, 1})).rounded(0.1f)}.distance({0, 0, 0}), 1.1f), "rounding");
    const Geometry layer{std::move(Shape::sphere(1)).shell(0.1f)};
    check(near(layer.distance({1, 0, 0}), 0.1f) && near(layer.distance({0, 0, 0}), -0.9f), "shell");
}

void transformsKeepDistancesExact() {
    const Geometry moved{std::move(Shape::sphere(1)).scaled(2).translated({5, 0, 0})};
    check(near(moved.distance({5, 0, 0}), 2) && near(moved.distance({8, 0, 0}), -1), "scale then move");

    const auto quarter = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
    const Geometry turned{std::move(Shape::box({1, 0.5f, 0.5f})).rotated(quarter)};
    check(near(turned.distance({0, 0.9f, 0}), 0.1f) && turned.distance({0.9f, 0, 0}) < 0, "rotation");

    // Distance parameters of operations scale with the shape.
    const Geometry big{std::move(std::move(Shape::box({1, 1, 1})).rounded(0.1f)).scaled(2)};
    check(near(big.distance({0, 0, 0}), 2.2f), "rounding radius scales");
}

void materialOfTheNearestSurface() {
    const Geometry holed{std::move(Shape::box({1, 1, 1})).material(1) - std::move(Shape::sphere(0.5f)).material(2)};
    check(holed.sample({0.6f, 0, 0}).material == 2 && holed.sample({0.95f, 0, 0}).material == 1,
          "material follows the surface that decides the distance");
    const Geometry turning{std::move(Shape::sphere(1)).motion(3)};
    check(turning.sample({0, 0, 0}).motion == 3, "motion index is carried like material");
}

void normalsPointIntoTheRegion() {
    const Geometry sphere{Shape::sphere(1)};
    check(near(sphere.normal({0.5f, 0, 0}), {-1, 0, 0}, 1e-3f), "toward the centre, away from the wall");
    const Geometry floor{Shape::halfSpace({0, 1, 0}, 0)};
    check(near(floor.normal({3, -1, 7}), {0, -1, 0}, 1e-3f), "half-space normal");
}

void batchesMatchPointsAndStayLipschitz() {
    const Geometry geometry{(Shape::box({2, 2, 2}) - Shape::sphere(1)) |
        std::move(Shape::capsule({0, 0, 0}, {3, 0, 0}, 0.3f)).translated({0, 1, 0})};
    std::mt19937 random(7);
    std::uniform_real_distribution<float> coordinate(-3, 3);
    std::vector<float> x(200), y(200), z(200), d(200);
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = coordinate(random), y[i] = coordinate(random), z[i] = coordinate(random);
    geometry.evaluate(x, y, z, d);   // 200 points: spans several blocks
    bool same = true, lipschitz = true;
    for (size_t i = 0; i < x.size(); ++i) {
        same &= near(d[i], geometry.distance({x[i], y[i], z[i]}));
        if (i > 0) {
            const float step = glm::length(glm::vec3(x[i] - x[i - 1], y[i] - y[i - 1], z[i] - z[i - 1]));
            lipschitz &= std::abs(d[i] - d[i - 1]) <= step + 1e-4f;
        }
    }
    check(same, "block evaluation equals point evaluation");
    check(lipschitz, "exact operations never change faster than distance");
    check(geometry.program().depth == 2, "stack depth of (box − sphere) ∪ capsule");
}

struct Particles {
    std::vector<float> x, y, z, vx, vy, vz, fx, fy, fz;
    explicit Particles(size_t n) : x(n), y(n), z(n), vx(n), vy(n), vz(n), fx(n), fy(n), fz(n) {}
    ParticleColumns columns() { return {x, y, z, vx, vy, vz, fx, fy, fz}; }
    glm::vec3 position(size_t i) const { return {x[i], y[i], z[i]}; }
    glm::vec3 velocity(size_t i) const { return {vx[i], vy[i], vz[i]}; }
};

void hardWallsReflect() {
    const Geometry sphere{Shape::sphere(1)};
    const Walls walls{sphere};
    Particles p(2);
    p.x = {1.2f, 0.5f};
    p.vx = {1, 1};
    walls.constrain(p.columns());
    check(near(glm::length(p.position(0)), 1.0f, 1e-4f) && sphere.distance(p.position(0)) >= -1e-5f,
          "a particle in the wall is moved onto the surface");
    check(near(p.velocity(0), {-1, 0, 0}) , "normal velocity reflected, speed kept (elastic)");
    check(p.x[1] == 0.5f && p.vx[1] == 1, "particles inside the region are untouched");

    const Walls damped{sphere, {Laws::Hard::make(0.5f)}};
    Particles q(1);
    q.x = {1.1f};
    q.vx = {2};
    q.vy = {1};
    damped.constrain(q.columns());
    check(near(q.velocity(0), {-1, 1, 0}), "restitution scales only the normal velocity");
}

void softWallsPushBack() {
    // Region y <= 10: the wall is above. Layer 1 thick, harmonic.
    const Geometry ceiling{Shape::halfSpace({0, 1, 0}, 10)};
    const auto soft = Laws::Soft::make(2, 1, 2);
    const Walls walls{ceiling, {soft}};
    Particles p(3);
    p.y = {9.5f, 8.0f, 9.9f};
    walls.addForces(p.columns());
    check(near(p.fy[0], -1.0f) && p.fy[1] == 0.0f, "harmonic: F = k (range − d), zero outside the layer");

    // Force is minus the potential gradient (numerical check at y = 9.9).
    const float h = 1e-3f;
    const float dU = Laws::Soft::potential(ceiling.distance({0, 9.9f + h, 0}), soft)
                   - Laws::Soft::potential(ceiling.distance({0, 9.9f - h, 0}), soft);
    check(near(p.fy[2], -dU / (2 * h), 1e-2f), "F = −dU/dy");

    check(throws([&] { Walls{ceiling, {Laws::Soft::make(0, 1)}}; }), "invalid soft parameters");
    check(throws([&] { Walls{Geometry{std::move(Shape::sphere(1)).material(3)}}; }), "missing material");
}

void materialsChooseTheLaw() {
    // Box with a hole: box faces soft, the hole hard.
    const Geometry holed{std::move(Shape::box({2, 2, 2})).material(0) - std::move(Shape::sphere(0.5f)).material(1)};
    const Walls walls{holed, {Laws::Soft::make(1, 0.5f), Laws::Hard::make()}};
    Particles p(2);
    p.x = {1.8f, 0.4f};
    p.vx = {0, 1};
    walls.addForces(p.columns());
    walls.constrain(p.columns());
    check(p.fx[0] < 0 && p.vx[0] == 0, "near a box face: soft force, no reflection");
    check(p.fx[1] == 0 && near(p.x[1], 0.5f, 1e-4f) && p.vx[1] > 0, "in the hole: pushed out and reflected");
}

void wallsCoverEveryChunk() {
    const Geometry sphere{Shape::sphere(1)};
    const Walls walls{sphere};
    Particles p(1000);   // several chunks of 256
    for (size_t i = 0; i < p.x.size(); ++i)
        p.x[i] = 1.5f, p.vx[i] = 1;
    walls.constrain(p.columns());
    bool all = true;
    for (size_t i = 0; i < p.x.size(); ++i)
        all &= near(p.x[i], 1.0f, 1e-4f) && p.vx[i] < 0;
    check(all, "every chunk is processed");
}

void periodicSeamsAreChecked() {
    const Space space{{-5, -5, -5}, {10, 10, 10}, {true, false, false}};
    const auto alongX = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
    checkPeriodicSeams(space, Geometry{std::move(Shape::infiniteCylinder(2)).rotated(alongX)});
    check(throws([&] { checkPeriodicSeams(space, Geometry{std::move(Shape::sphere(2)).translated({3, 0, 0})}); }),
          "an off-centre sphere tears at the periodic seam");
}

}

int main() {
    spaceWrapsAndTakesMinimumImage();
    primitivesGiveExactDistances();
    booleansAndBlends();
    transformsKeepDistancesExact();
    materialOfTheNearestSurface();
    normalsPointIntoTheRegion();
    batchesMatchPointsAndStayLipschitz();
    hardWallsReflect();
    softWallsPushBack();
    materialsChooseTheLaw();
    wallsCoverEveryChunk();
    periodicSeamsAreChecked();
    std::cout << "Boundaries: space, geometry, walls and seam checks passed\n";
}

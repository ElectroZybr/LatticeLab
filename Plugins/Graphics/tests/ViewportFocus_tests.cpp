#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include "Viewport.hpp"

TEST(Viewport_ActiveControllerMovesLocalCamera, Lattice::RuntimeFixture) {
    auto& ctx = fixture.run_ctx;
    fixture.blueprints.add<SceneObject>();
    fixture.blueprints.add<Camera, SceneObject>();
    fixture.blueprints.add<TransformController>();
    fixture.blueprints.add<Viewport>();
    fixture.root.add<Viewport>("left");
    auto* left = fixture.root.find<Viewport>("left").node;
    REQUIRE(ctx.activeScope == left->getFocusScopeId());
    fixture.root.configureBranch();
    auto leftCamera = left->focus<SceneObject>("camera");
    const auto role = ctx.roles.find("right");
    ctx.bindings.invoke(ctx.resolveFocus(Lattice::InvalidFocusScopeId, role));
    REQUIRE(leftCamera->transform().position.x > 0.09f);

    fixture.root.add<Viewport>("right");
    auto* right = fixture.root.find<Viewport>("right").node;
    fixture.root.configureBranch();
    REQUIRE(ctx.activeScope == right->getFocusScopeId());
    auto rightCamera = right->focus<SceneObject>("camera");
    const auto oldLeft = leftCamera->transform().position.x;
    ctx.bindings.invoke(ctx.resolveFocus(Lattice::InvalidFocusScopeId, role));
    REQUIRE(rightCamera->transform().position.x > 0.09f);
    REQUIRE(leftCamera->transform().position.x == oldLeft);

    ctx.activateFocus(left->getFocusScopeId());
    ctx.bindings.invoke(ctx.resolveFocus(Lattice::InvalidFocusScopeId, role));
    REQUIRE(leftCamera->transform().position.x > oldLeft);
    REQUIRE(rightCamera->transform().position.x < 0.11f);
}

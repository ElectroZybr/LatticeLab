#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include "Render.hpp"
#include "Shell/include/WindowAPI.hpp"
#include <cstring>
#include <vector>

namespace RenderTests {
    
struct Counters {
    int surfaces = 0, shaders = 0, pipelines = 0, draws = 0, submits = 0, presents = 0, releases = 0;
    int liveSurfaces = 0, nativeDestroyed = 0;
    int passes = 0, ends = 0;
    std::vector<GPU::Rect> regions;
    std::vector<glm::mat4> matrices;
    bool available = true, failDraw = false, failPipeline = false, prematureWindowDestroy = false;
} stats;

struct Surface final : GPU::Surface {
    uint32_t width = 0, height = 0;
    Surface(Lattice::Node&, const Desc&) { ++stats.surfaces; ++stats.liveSurfaces; }
    ~Surface() override { --stats.liveSurfaces; }
    GPU::TextureFormat format() const override { return GPU::TextureFormat::BGRA8Unorm; }
    void resize(uint32_t w, uint32_t h) override { width = w; height = h; }
    bool acquire() override { return width && height && stats.available; }
    void present() override { ++stats.presents; }
    void releaseFrame() override { ++stats.releases; }
};

struct Shader final : GPU::Shader {
    Shader(Lattice::Node&, const Desc& desc) {
        REQUIRE(desc.source.find("@vertex") != std::string::npos);
        ++stats.shaders;
    }
};

struct Pipeline final : GPU::Pipeline {
    Pipeline(Lattice::Node&, const Desc& desc) {
        REQUIRE(desc.shader);
        if (stats.failPipeline) throw Lattice::Exception("test", "pipeline failure");
        ++stats.pipelines;
    }
};

struct Pass final : GPU::RenderPass {
    bool active = false, pipeline = false, uniform = false;
    GPU::Rect viewport{}, scissor{};
    void setViewport(GPU::Rect rect) override { REQUIRE(active); viewport = rect; }
    void setScissor(GPU::Rect rect) override { REQUIRE(active); scissor = rect; }
    void setPipeline(GPU::Pipeline&) override { REQUIRE(active); pipeline = true; }
    void setUniform(uint32_t group, uint32_t binding, std::span<const std::byte> data) override {
        REQUIRE(active && pipeline);
        REQUIRE(group == 0 && binding == 0 && data.size() == sizeof(glm::mat4));
        glm::mat4 matrix;
        std::memcpy(&matrix, data.data(), data.size());
        stats.matrices.push_back(matrix);
        uniform = true;
    }
    void draw(uint32_t vertices, uint32_t firstVertex) override {
        REQUIRE(active && pipeline && uniform);
        REQUIRE(vertices == 6 && firstVertex == 0);
        REQUIRE(viewport.x == scissor.x && viewport.y == scissor.y);
        REQUIRE(viewport.width == scissor.width && viewport.height == scissor.height);
        if (stats.failDraw) throw Lattice::Exception("test", "draw failure");
        stats.regions.push_back(viewport);
        ++stats.draws;
        uniform = false;
    }
    void end() override { REQUIRE(active); active = false; ++stats.ends; }
};

struct Commands final : GPU::CommandList {
    Pass pass;
    GPU::RenderPass& beginRenderPass(GPU::Surface&, GPU::Color clear) override {
        REQUIRE(!pass.active);
        REQUIRE(clear.r == 0.1f && clear.g == 0.2f && clear.b == 0.3f && clear.a == 1.0f);
        pass.active = true;
        ++stats.passes;
        return pass;
    }
    void submit() override { REQUIRE(!pass.active); ++stats.submits; }
};

struct Device final : GPU::Device {
    Device(Lattice::Node&, const Desc&) {}
    std::unique_ptr<GPU::CommandList> createCommandList() override { return std::make_unique<Commands>(); }
};

struct Window final : WindowAPI {
    glm::vec2 size{640, 480};
    bool closed = false;
    NativeWindow handle{NativeWindow::Kind::X11, nullptr, reinterpret_cast<void*>(1)};
    Window() {
        handle.owner = std::shared_ptr<void>(new int, [](void* value) {
            delete static_cast<int*>(value);
            ++stats.nativeDestroyed;
            // During replacement the old surface must be gone before its window.
            if (stats.liveSurfaces) stats.prematureWindowDestroy = true;
        });
    }
    bool shouldClose() const override { return closed; }
    void requestClose() override { closed = true; }
    void pollEvents() override {}
    const Ref<Input::Keyboard> keyboard() const override { return {}; }
    const Ref<Input::Mouse> mouse() const override { return {}; }
    glm::vec2 windowSize() const override { return size; }
    glm::vec2 framebufferSize() const override { return size; }
    float contentScale() const override { return 1; }
    bool fullscreen() const override { return false; }
    void setFullscreen(bool) override {}
    NativeWindow native() const override { return handle; }
    void show() override {}
    void setTitle(std::string_view) override {}
};

struct Fixture : Lattice::RuntimeFixture {
    Ref<Window> window;
    Ref<Render> renderer;
    Fixture() {
        stats = {};
        blueprints.add<GPU::Device>();
        blueprints.add<Device, GPU::Device>();
        blueprints.add<GPU::Surface>(); blueprints.add<Surface, GPU::Surface>();
        blueprints.add<GPU::Shader>(); blueprints.add<Shader, GPU::Shader>();
        blueprints.add<GPU::Pipeline>(); blueprints.add<Pipeline, GPU::Pipeline>();
        blueprints.add<WindowAPI>(); blueprints.add<Window, WindowAPI>(); blueprints.add<Render>();
        blueprints.add<SceneObject>(); blueprints.add<Camera, SceneObject>(); blueprints.add<Viewport>();
        root.add<Device>("GPU");
        auto slot = root.slot<WindowAPI>();
        root.use<WindowAPI, Window>();
        window = Ref<Window>(static_cast<Window*>(slot.get()));
        renderer = root.add<Render>();
        root.configureBranch();
    }
};

TEST(Render_ViewportsShareFrameResources, Fixture) {
    auto& branch = fixture.root.require("Render");
    auto main = branch.find<Viewport>("Main");
    main->setSize({320, 480});
    auto second = branch.add<Viewport>("Second");
    second->setPosition({320, 0});
    second->setSize({320, 240});
    branch.configureBranch();
    fixture.renderer->configure(branch);
    fixture.renderer->frame();
    REQUIRE(stats.draws == 2);
    REQUIRE(stats.passes == 1 && stats.ends == 1);
    REQUIRE(stats.regions[0].x == 0 && stats.regions[0].width == 320 && stats.regions[0].height == 480);
    REQUIRE(stats.regions[1].x == 320 && stats.regions[1].width == 320 && stats.regions[1].height == 240);
    REQUIRE(stats.matrices[0] != stats.matrices[1]);
    REQUIRE(stats.submits == 1 && stats.presents == 1);
    REQUIRE(stats.surfaces == 1 && stats.shaders == 1 && stats.pipelines == 1);

    branch.remove<Viewport>("Main");
    branch.remove<Viewport>("Second");
    fixture.renderer->configure(branch);
    fixture.renderer->frame();
    REQUIRE(stats.draws == 2);
    REQUIRE(stats.passes == 2 && stats.ends == 2 && stats.presents == 2);
}

TEST(Render_ClipsAndSkipsViewportRegions, Fixture) {
    auto& branch = fixture.root.require("Render");
    auto viewport = branch.find<Viewport>("Main");
    viewport->setPosition({600, 400});
    viewport->setSize({100, 100});
    fixture.renderer->frame();
    REQUIRE(stats.regions.back().width == 40 && stats.regions.back().height == 80);
    viewport->setPosition({640, 0});
    fixture.renderer->frame();
    viewport->setPosition({0, 0});
    viewport->setSize({0, 480});
    fixture.renderer->frame();
    REQUIRE(stats.draws == 1 && stats.presents == 3);
    viewport->fitSurface();
    fixture.renderer->frame();
    REQUIRE(stats.regions.back().width == 640 && stats.regions.back().height == 480);
}

TEST(Render_CachesResourcesAndResizes, Fixture) {
    REQUIRE(stats.surfaces == 1 && stats.shaders == 1 && stats.pipelines == 1);
    REQUIRE(stats.draws == 0);
    fixture.renderer->configure(fixture.root.require("Render"));
    REQUIRE(stats.surfaces == 1 && stats.shaders == 1 && stats.pipelines == 1);
    fixture.renderer->frame();
    REQUIRE(stats.regions.back().width == 640 && stats.regions.back().height == 480);
    fixture.window->size = {800, 600};
    fixture.renderer->frame();
    REQUIRE(stats.regions.back().width == 800 && stats.regions.back().height == 600);
    REQUIRE(stats.draws == 2 && stats.submits == 2 && stats.presents == 2);
    REQUIRE(stats.surfaces == 1 && stats.shaders == 1 && stats.pipelines == 1);
    fixture.window->size = {0, 0};
    fixture.renderer->frame();
    REQUIRE(stats.draws == 2);
    fixture.window->size = {800, 600};
    fixture.renderer->frame();
    REQUIRE(stats.draws == 3);
    fixture.root.remove<Render>();
    REQUIRE(fixture.root.globalCollect<GPU::Surface>().empty());
    REQUIRE(fixture.root.globalCollect<GPU::Pipeline>().empty());
}

TEST(Render_UnavailableAndFailedFrames, Fixture) {
    stats.available = false;
    fixture.renderer->frame();
    REQUIRE(stats.submits == 0);
    stats.available = true;
    stats.failDraw = true;
    bool rejected = false;
    try { fixture.renderer->frame(); } catch (const Lattice::Exception&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(stats.releases == 1 && stats.presents == 0);
    stats.failDraw = false;
    fixture.renderer->frame();
    REQUIRE(stats.presents == 1);
    fixture.window->requestClose();
    fixture.renderer->frame();
    REQUIRE(stats.presents == 1);
}

TEST(Render_RecreatesWindowResources, Fixture) {
    fixture.renderer->frame();
    fixture.root.use<WindowAPI, Window>();
    REQUIRE(stats.nativeDestroyed == 1);
    REQUIRE(!stats.prematureWindowDestroy);
    REQUIRE(stats.surfaces == 2 && stats.pipelines == 2 && stats.shaders == 2);
    fixture.renderer->frame();
    REQUIRE(stats.surfaces == 2 && stats.pipelines == 2 && stats.shaders == 2);
    REQUIRE(stats.presents == 2);
}

TEST(Render_FailedConfigureReleasesResources, Fixture) {
    fixture.root.remove<Render>();
    stats.failPipeline = true;
    fixture.root.add<Render>();
    bool rejected = false;
    try { fixture.root.require("Render").configureBranch(); }
    catch (const Lattice::Exception&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(stats.liveSurfaces == 0);
    REQUIRE(fixture.root.globalCollect<GPU::Shader>().empty());
    REQUIRE(fixture.root.globalCollect<GPU::Pipeline>().empty());
}

TEST(Render_DeviceDestroyedBeforeRenderer, Fixture) {
    fixture.root.remove<Device>("GPU");
    REQUIRE(stats.liveSurfaces == 0);
    fixture.root.remove<Render>();
    REQUIRE(!stats.prematureWindowDestroy);
}

}

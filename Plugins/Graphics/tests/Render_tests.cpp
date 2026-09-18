#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include "Render.hpp"
#include "Shell/include/WindowAPI.hpp"

namespace RenderTests {
    
struct Counters {
    int surfaces = 0, shaders = 0, pipelines = 0, draws = 0, submits = 0, presents = 0, releases = 0;
    int liveSurfaces = 0, nativeDestroyed = 0;
    bool available = true, failDraw = false, failPipeline = false, prematureWindowDestroy = false;
} stats;

struct Surface final : Graphics::Surface {
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

struct Pipeline final : Graphics::RenderPipeline {
    Pipeline(Lattice::Node&, const Desc& desc) {
        REQUIRE(desc.shader);
        if (stats.failPipeline) throw Lattice::Exception("test", "pipeline failure");
        ++stats.pipelines;
    }
};

struct Commands final : Graphics::CommandList {
    void draw(Graphics::Surface&, Graphics::RenderPipeline&, Graphics::ClearColor, uint32_t vertices) override {
        REQUIRE(vertices == 6);
        if (stats.failDraw) throw Lattice::Exception("test", "draw failure");
        ++stats.draws;
    }
    void submit() override { ++stats.submits; }
};

struct Device final : Graphics::Device {
    std::unique_ptr<GPU::CommandList> createCommandList() override { return std::make_unique<Commands>(); }
    std::unique_ptr<Graphics::CommandList> createRenderCommandList() override { return std::make_unique<Commands>(); }
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
        blueprints.add<Graphics::Device, GPU::Device>();
        blueprints.add<Device, Graphics::Device>();
        blueprints.add<Graphics::Surface>(); blueprints.add<Surface, Graphics::Surface>();
        blueprints.add<GPU::Shader>(); blueprints.add<Shader, GPU::Shader>();
        blueprints.add<Graphics::RenderPipeline>(); blueprints.add<Pipeline, Graphics::RenderPipeline>();
        blueprints.add<WindowAPI>(); blueprints.add<Window, WindowAPI>(); blueprints.add<Render>();
        root.add<Device>("GPU");
        auto slot = root.slot<WindowAPI>();
        root.use<WindowAPI, Window>();
        window = Ref<Window>(static_cast<Window*>(slot.get()));
        renderer = root.add<Render>();
        root.configureBranch();
    }
};

TEST(Render_CachesResourcesAndResizes, Fixture) {
    REQUIRE(stats.surfaces == 1 && stats.shaders == 1 && stats.pipelines == 1);
    REQUIRE(stats.draws == 0);
    fixture.renderer->configure(fixture.root.require("Render"));
    REQUIRE(stats.surfaces == 1 && stats.shaders == 1 && stats.pipelines == 1);
    fixture.renderer->frame();
    fixture.window->size = {800, 600};
    fixture.renderer->frame();
    REQUIRE(stats.draws == 2 && stats.submits == 2 && stats.presents == 2);
    REQUIRE(stats.surfaces == 1 && stats.shaders == 1 && stats.pipelines == 1);
    fixture.window->size = {0, 0};
    fixture.renderer->frame();
    REQUIRE(stats.draws == 2);
    fixture.window->size = {800, 600};
    fixture.renderer->frame();
    REQUIRE(stats.draws == 3);
    fixture.root.remove<Render>();
    REQUIRE(fixture.root.globalCollect<Graphics::Surface>().empty());
    REQUIRE(fixture.root.globalCollect<Graphics::RenderPipeline>().empty());
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
    REQUIRE(fixture.root.globalCollect<Graphics::RenderPipeline>().empty());
}

TEST(Render_DeviceDestroyedBeforeRenderer, Fixture) {
    fixture.root.remove<Device>("GPU");
    REQUIRE(stats.liveSurfaces == 0);
    fixture.root.remove<Render>();
    REQUIRE(!stats.prematureWindowDestroy);
}

}

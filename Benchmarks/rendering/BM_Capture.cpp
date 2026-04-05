#include <benchmark/benchmark.h>

#include <filesystem>
#include <memory>

#include "App/capture/FrameRecorder.h"
#include "Rendering/2d/Renderer2D.h"
#include "Rendering/RendererCapture.h"
#include "fixtures/RendererFixture.h"

namespace {
std::filesystem::path makeCaptureBenchDir() {
    const std::filesystem::path dir = std::filesystem::current_path() / "Benchmarks" / "results" / "capture_bench_tmp";
    std::filesystem::create_directories(dir);
    return dir;
}
}

class CaptureFixture : public RendererFixture<Renderer2D> {
public:
    CaptureFixture() = default;
    ~CaptureFixture() noexcept override = default;

    void SetUp(benchmark::State& state) override {
        RendererFixture<Renderer2D>::SetUp(state);
        if (!renderTexture_ || !renderer_) {
            state.SkipWithError("Renderer fixture setup failed");
            return;
        }

        renderer_->drawShot(atomStorage_, box_);
        renderTexture_->display();
        capturedFrame_ = rendererCapture_.captureRGBA_PBO(*renderTexture_);
        renderer_->drawShot(atomStorage_, box_);
        renderTexture_->display();
        capturedFrame_ = rendererCapture_.captureRGBA_PBO(*renderTexture_);

        if (capturedFrame_.empty()) {
            state.SkipWithError("Failed to capture benchmark frame");
            return;
        }

        captureDir_ = makeCaptureBenchDir();
        frameRecorder_ = std::make_unique<FrameRecorder>();
        frameRecorder_->start(captureDir_, CaptureSettings{});
        if (!frameRecorder_->isRecording()) {
            state.SkipWithError("Failed to start frame recorder");
        }
    }

    void TearDown(benchmark::State& state) override {
        if (frameRecorder_) {
            frameRecorder_->stop();
            frameRecorder_.reset();
        }
        RendererFixture<Renderer2D>::TearDown(state);
        std::error_code ec;
        std::filesystem::remove_all(captureDir_, ec);
    }

protected:
    CapturedFrame capturedFrame_{};
    std::unique_ptr<FrameRecorder> frameRecorder_{};
    std::filesystem::path captureDir_{};
    RendererCapture rendererCapture_{};
};

// @bench_meta {"id":"CaptureFixture/CaptureReadback2DSync","ru":"Снятие кадра из RenderTexture (sync)","group":"Рендер/Захват"}
BENCHMARK_DEFINE_F(CaptureFixture, CaptureReadback2DSync)(benchmark::State& state) {
    for (auto _ : state) {
        renderer_->drawShot(atomStorage_, box_);
        renderTexture_->display();
        CapturedFrame frame = RendererCapture::captureRGBA(*renderTexture_);
        benchmark::DoNotOptimize(frame);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(CaptureFixture, CaptureReadback2DSync)
    ->RangeMultiplier(8)->Range(125, 8000);

// @bench_meta {"id":"CaptureFixture/CaptureReadback2DPBO","ru":"Снятие кадра из RenderTexture (PBO)","group":"Рендер/Захват"}
BENCHMARK_DEFINE_F(CaptureFixture, CaptureReadback2DPBO)(benchmark::State& state) {
    for (auto _ : state) {
        renderer_->drawShot(atomStorage_, box_);
        renderTexture_->display();
        CapturedFrame frame = rendererCapture_.captureRGBA_PBO(*renderTexture_);
        benchmark::DoNotOptimize(frame);
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(CaptureFixture, CaptureReadback2DPBO)
    ->RangeMultiplier(8)->Range(125, 8000);

// @bench_meta {"id":"CaptureFixture/CaptureEncodeFrame","ru":"Кодирование кадра в видео","group":"Рендер/Захват"}
BENCHMARK_DEFINE_F(CaptureFixture, CaptureEncodeFrame)(benchmark::State& state) {
    if (!frameRecorder_ || !frameRecorder_->isRecording()) {
        state.SkipWithError("Frame recorder is not available");
        return;
    }

    for (auto _ : state) {
        CapturedFrame frame = capturedFrame_;
        const bool ok = frameRecorder_->submit(std::move(frame));
        if (!ok) {
            state.SkipWithError("Frame recorder submit failed");
            break;
        }
        benchmark::ClobberMemory();
    }
    setCounters(state);
}

BENCHMARK_REGISTER_F(CaptureFixture, CaptureEncodeFrame)
    ->RangeMultiplier(8)->Range(125, 8000);

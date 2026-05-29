#include "Rendering/BaseRenderer.h"

RenderData& BaseRenderer::addRenderData() {
    return renderData_.emplace_back();
}

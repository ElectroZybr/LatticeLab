#pragma once
#include "NativeWindow.hpp"
#include <GPU/include/Texture.hpp>

namespace Graphics {
struct SurfaceDesc { NativeWindow window; };
class Surface : public Lattice::Component {
public:
    using Desc = SurfaceDesc;
    virtual ~Surface() = default;
    virtual GPU::TextureFormat format() const = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual bool acquire() = 0;
    virtual void present() = 0;
    virtual void releaseFrame() = 0;
};
}

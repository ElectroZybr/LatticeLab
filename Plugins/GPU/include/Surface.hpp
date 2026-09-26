#pragma once
#include "NativeWindow.hpp"
#include <GPU/include/Texture.hpp>

namespace GPU {

class Surface : public Lattice::Component {
public:
    virtual ~Surface() = default;
    virtual GPU::TextureFormat format() const = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void attach(const NativeWindow& window) = 0;
    virtual bool acquire() = 0;
    virtual void present() = 0;
    virtual void releaseFrame() = 0;
};

}

#pragma once

/// Something the window loop advances once per frame (renderers, overlays).
class FrameAPI {
public:
    virtual ~FrameAPI() = default;
    virtual void frame(float dt) = 0;
};

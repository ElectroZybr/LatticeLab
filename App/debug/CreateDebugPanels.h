#pragma once

class DebugPanel;
class DebugView;

struct DebugViews {
    DebugView* sim = nullptr;
    DebugView* atomSingle = nullptr;
    DebugView* atomBatch = nullptr;
    DebugView* neighbor = nullptr;
};

DebugViews createDebugViews(DebugPanel& panel);

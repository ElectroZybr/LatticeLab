#pragma once

class DebugPanel;
class DebugView;

struct DebugViews {
    DebugView* sim = nullptr;
    DebugView* atomSingle = nullptr;
    DebugView* atomBatch = nullptr;
    DebugView* neighbor = nullptr;
    DebugView* timers = nullptr;
};

DebugViews createDebugViews(DebugPanel& panel);

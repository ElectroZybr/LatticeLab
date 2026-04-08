#pragma once

#include <imgui.h>

#include "Engine/physics/AtomData.h"

struct UiState;

void drawIOPanelAtomTypeCombo(const char* id, AtomData::Type& atomType, float width, float uiScale);
void drawIOPanelCaptureStatus(const UiState& uiState);
void drawIOPanelScenePreviewFallback(const ImVec2& previewSize);

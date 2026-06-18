#include "pch.h"
#include "UI/DebugUI.h"
#include "Gameplay/PlayerCamera.h"
#include "Gameplay/Player.h"
#include "Gameplay/Weapon/PlayerWeapon.h"
#include "Render/Visuals/Grid.h"
#include "Render/Visuals/Skybox.h"
#include "Services/AudioManager.h"
#include "Services/BeatTracker.h"
#include "Gameplay/BulletPool.h"
#include "Gameplay/Boss.h"
#include "Render/Pipeline/Bloom.h"
#include "Render/Lighting/SceneLighting.h"
#include "Services/InputManager.h"
#include <imgui.h>

#include <cstdarg>
#include <cstdio>

namespace
{
    constexpr int DEBUG_STYLE_VAR_COUNT = 7;
    constexpr int DEBUG_STYLE_COLOR_COUNT = 24;

    constexpr ImU32 DEBUG_COLOR_ACCENT = IM_COL32(236, 158, 36, 255);
    constexpr ImU32 DEBUG_COLOR_TEXT = IM_COL32(192, 192, 192, 255);
    constexpr ImU32 DEBUG_COLOR_TEXT_DISABLED = IM_COL32(128, 128, 128, 255);
    constexpr ImU32 DEBUG_COLOR_BACKGROUND = IM_COL32(36, 36, 36, 245);
    constexpr ImU32 DEBUG_COLOR_BACKGROUND_DARK = IM_COL32(26, 26, 26, 255);
    constexpr ImU32 DEBUG_COLOR_TITLEBAR = IM_COL32(21, 21, 21, 255);
    constexpr ImU32 DEBUG_COLOR_PROPERTY_FIELD = IM_COL32(15, 15, 15, 255);
    constexpr ImU32 DEBUG_COLOR_GROUP_HEADER = IM_COL32(47, 47, 47, 255);
    constexpr ImU32 DEBUG_COLOR_GROUP_HEADER_HOVER = IM_COL32(58, 58, 58, 255);
    constexpr ImU32 DEBUG_COLOR_BORDER = IM_COL32(50, 50, 50, 255);
    constexpr ImU32 DEBUG_COLOR_ROW_ALT = IM_COL32(31, 31, 31, 255);
    constexpr ImU32 DEBUG_COLOR_ERROR = IM_COL32(230, 51, 51, 255);

    void PushDebugWindowStyle()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 7.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 2.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, DEBUG_COLOR_TEXT);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, DEBUG_COLOR_TEXT_DISABLED);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, DEBUG_COLOR_BACKGROUND);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, DEBUG_COLOR_TITLEBAR);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, DEBUG_COLOR_TITLEBAR);
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, DEBUG_COLOR_TITLEBAR);
        ImGui::PushStyleColor(ImGuiCol_Border, DEBUG_COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_Header, DEBUG_COLOR_GROUP_HEADER);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, DEBUG_COLOR_GROUP_HEADER_HOVER);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, DEBUG_COLOR_GROUP_HEADER);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, DEBUG_COLOR_PROPERTY_FIELD);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(31, 31, 31, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(40, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_Button, DEBUG_COLOR_GROUP_HEADER);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DEBUG_COLOR_GROUP_HEADER_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(38, 38, 38, 255));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, DEBUG_COLOR_ACCENT);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(130, 130, 130, 220));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, DEBUG_COLOR_ACCENT);
        ImGui::PushStyleColor(ImGuiCol_Separator, DEBUG_COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_TableRowBg, DEBUG_COLOR_BACKGROUND_DARK);
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, DEBUG_COLOR_ROW_ALT);
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, DEBUG_COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, DEBUG_COLOR_BORDER);
    }

    void PopDebugWindowStyle()
    {
        ImGui::PopStyleColor(DEBUG_STYLE_COLOR_COUNT);
        ImGui::PopStyleVar(DEBUG_STYLE_VAR_COUNT);
    }

    bool BeginPropertyGrid(const char* id)
    {
        const ImGuiTableFlags flags =
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_NoPadOuterX;

        if (!ImGui::BeginTable(id, 2, flags))
        {
            return false;
        }

        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    void BeginPropertyRow(const char* label)
    {
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", label);
        ImGui::TableNextColumn();
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(-1.0f);
    }

    void EndPropertyRow()
    {
        ImGui::PopID();
    }

    void DrawTextRow(const char* label, const char* value)
    {
        BeginPropertyRow(label);
        ImGui::TextUnformatted(value);
        EndPropertyRow();
    }

    void DrawFormattedTextRow(const char* label, const char* format, ...)
    {
        char value[128] = {};

        va_list args;
        va_start(args, format);
        std::vsnprintf(value, sizeof(value), format, args);
        va_end(args);

        DrawTextRow(label, value);
    }

    void DrawUnavailableRow(const char* label)
    {
        BeginPropertyRow(label);
        ImGui::TextDisabled("Unavailable");
        EndPropertyRow();
    }

    void DrawUnavailable(const char* label)
    {
        ImGui::TextDisabled("%s unavailable", label);
    }

    bool DrawSliderFloatRow(const char* label, float* value, float min, float max, const char* format = "%.2f")
    {
        BeginPropertyRow(label);
        const bool changed = ImGui::SliderFloat("##value", value, min, max, format);
        EndPropertyRow();
        return changed;
    }

    bool DrawSliderFloat3Row(const char* label, float* values, float min, float max, const char* format = "%.2f")
    {
        BeginPropertyRow(label);
        const bool changed = ImGui::SliderFloat3("##value", values, min, max, format);
        EndPropertyRow();
        return changed;
    }

    bool DrawColorEdit3Row(const char* label, float* color)
    {
        BeginPropertyRow(label);
        const bool changed = ImGui::ColorEdit3("##value", color);
        EndPropertyRow();
        return changed;
    }

    bool DrawColorEdit4Row(const char* label, float* color)
    {
        BeginPropertyRow(label);
        const bool changed = ImGui::ColorEdit4("##value", color);
        EndPropertyRow();
        return changed;
    }

    bool DrawCheckboxRow(const char* label, bool* value)
    {
        BeginPropertyRow(label);
        const bool changed = ImGui::Checkbox("##value", value);
        EndPropertyRow();
        return changed;
    }

    void DrawProgressRow(const char* label, float fraction, const char* overlay = nullptr)
    {
        BeginPropertyRow(label);
        ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);
        EndPropertyRow();
    }

    bool DrawPanelHeader(const char* label, bool defaultOpen = true)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_FramePadding;

        if (defaultOpen)
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        return ImGui::CollapsingHeader(label, flags);
    }

    void DrawSubheading(const char* label)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", label);
    }

    void DrawStatusCell(const char* label, const char* value)
    {
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", label);
        ImGui::TextUnformatted(value);
    }
}

bool DebugUI::beginDebugWindow()
{
    ImGui::SetNextWindowPos(ImVec2(24.0f, 24.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520.0f, 720.0f), ImGuiCond_FirstUseEver);

    return ImGui::Begin("Photonics Debug", nullptr, ImGuiWindowFlags_NoCollapse);
}

void DebugUI::endDebugWindow()
{
    ImGui::End();
}

void DebugUI::render()
{
    PushDebugWindowStyle();
    const bool visible = beginDebugWindow();

    if (visible)
    {
        drawHeader();
        drawCameraPanel();
        drawWeaponPanel();
        drawRenderingPanel();
        drawCombatPanel();
        drawAudioPanel();
    }

    endDebugWindow();
    PopDebugWindowStyle();

    drawCompositionOverlay();
}

void DebugUI::drawHeader()
{
    ImGuiIO& io = ImGui::GetIO();
    const float frameMs = io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f;

    char fpsText[32] = {};
    char frameText[32] = {};
    std::snprintf(fpsText, sizeof(fpsText), "%.1f", io.Framerate);
    std::snprintf(frameText, sizeof(frameText), "%.2f ms", frameMs);

#ifdef _DEBUG
    const char* buildText = "Debug";
#else
    const char* buildText = "Release";
#endif

    const char* cursorText = "Unavailable";
    if (m_input)
    {
        cursorText = m_input->isCursorVisible() ? "Visible / Absolute" : "Hidden / Relative";
    }

    const ImGuiTableFlags flags =
        ImGuiTableFlags_SizingStretchSame |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_NoSavedSettings;

    if (ImGui::BeginTable("##debug_status", 4, flags))
    {
        DrawStatusCell("FPS", fpsText);
        DrawStatusCell("Frame", frameText);
        DrawStatusCell("Build", buildText);
        DrawStatusCell("Cursor", cursorText);
        ImGui::EndTable();
    }

    if (m_input)
    {
        const char* systemCursor = m_input->isSystemCursorVisible() ? "visible" : "hidden";
        ImGui::TextDisabled("F3 closes debug. OS cursor: %s", systemCursor);
    }
    else
    {
        ImGui::TextDisabled("F3 closes debug.");
    }

    ImGui::Separator();
}

void DebugUI::drawCameraPanel()
{
    if (!DrawPanelHeader("Camera"))
    {
        return;
    }

    if (!m_camera)
    {
        DrawUnavailable("Camera");
        return;
    }

    if (BeginPropertyGrid("##camera_properties"))
    {
        DrawSliderFloatRow("HIP FOV", m_camera->hipFovDegreesPtr(), 30.0f, 90.0f, "%.1f");
        DrawSliderFloatRow("ADS FOV", m_camera->adsFovDegreesPtr(), 20.0f, 70.0f, "%.1f");
        DrawSliderFloatRow("FOV Blend Speed", m_camera->fovBlendSpeedPtr(), 0.0f, 30.0f, "%.2f");

        if (m_player)
        {
            DrawSliderFloatRow("Mouse Sens", m_player->mouseSensitivityPtr(), 0.005f, 0.200f, "%.3f");
        }
        else
        {
            DrawUnavailableRow("Mouse Sens");
        }

        DrawCheckboxRow("Rule of Thirds", &m_showThirds);
        ImGui::EndTable();
    }
}

void DebugUI::drawWeaponPanel()
{
    if (!DrawPanelHeader("Weapon"))
    {
        return;
    }

    if (!m_player)
    {
        DrawUnavailable("Weapon");
        return;
    }

    WeaponMotionTuning* t = m_player->weapon().getMotionTuning();
    if (!t)
    {
        DrawUnavailable("Weapon tuning");
        return;
    }

    DrawSubheading("Pose");
    if (BeginPropertyGrid("##weapon_pose"))
    {
        DrawSliderFloat3Row("Hip Position", &t->hipPosition.x, -1.0f, 1.0f, "%.3f");
        DrawSliderFloat3Row("ADS Position", &t->adsPosition.x, -1.0f, 1.0f, "%.3f");
        DrawSliderFloatRow("ADS Blend Speed", &t->adsBlendSpeed, 0.0f, 30.0f, "%.2f");
        ImGui::EndTable();
    }

    DrawSubheading("Sway");
    if (BeginPropertyGrid("##weapon_sway"))
    {
        DrawSliderFloatRow("Sway Max Delta", &t->swayMaxDeltaDeg, 0.0f, 60.0f, "%.2f");
        DrawSliderFloatRow("Position Gain", &t->swayPositionGain, 0.0f, 0.02f, "%.4f");
        DrawSliderFloatRow("Rotation Gain", &t->swayRotationGain, 0.0f, 1.0f, "%.3f");
        DrawSliderFloatRow("Return Speed", &t->swayReturnSpeed, 0.0f, 30.0f, "%.2f");
        ImGui::EndTable();
    }

    DrawSubheading("Movement Bob");
    if (BeginPropertyGrid("##weapon_bob"))
    {
        DrawSliderFloatRow("Frequency", &t->bobFrequency, 0.0f, 20.0f, "%.2f");
        DrawSliderFloatRow("Amplitude X", &t->bobAmplitudeX, 0.0f, 0.1f, "%.4f");
        DrawSliderFloatRow("Amplitude Y", &t->bobAmplitudeY, 0.0f, 0.1f, "%.4f");
        DrawSliderFloatRow("Falloff", &t->bobFalloff, 0.0f, 30.0f, "%.2f");
        ImGui::EndTable();
    }

    DrawSubheading("Recoil");
    if (BeginPropertyGrid("##weapon_recoil"))
    {
        DrawSliderFloatRow("Kickback", &t->recoilKickback, -30.0f, 0.0f, "%.2f");
        DrawSliderFloatRow("Pitch", &t->recoilPitchDeg, -30.0f, 0.0f, "%.2f");
        ImGui::EndTable();
    }
}

void DebugUI::drawRenderingPanel()
{
    if (!DrawPanelHeader("Rendering"))
    {
        return;
    }

    DrawSubheading("Camera");
    if (BeginPropertyGrid("##render_camera"))
    {
        if (m_exposure)
        {
            DrawSliderFloatRow("Exposure", m_exposure, 0.0f, 3.0f, "%.2f");
        }
        else
        {
            DrawUnavailableRow("Exposure");
        }
        ImGui::EndTable();
    }

    DrawSubheading("Directional Light");
    if (BeginPropertyGrid("##render_light"))
    {
        if (m_lighting)
        {
            DirectionalLight& keyLight = m_lighting->keyLight;
            if (DrawSliderFloat3Row("Direction To Light", &keyLight.directionToLight.x, -1.0f, 1.0f, "%.2f"))
            {
                if (keyLight.directionToLight.LengthSquared() > 0.0001f)
                {
                    keyLight.directionToLight.Normalize();
                }
            }
            DrawColorEdit3Row("Color", &keyLight.color.x);
            DrawSliderFloatRow("Intensity", &keyLight.intensity, 0.0f, 20.0f, "%.2f");
        }
        else
        {
            DrawUnavailableRow("Scene lighting");
        }
        ImGui::EndTable();
    }

    DrawSubheading("Grid");
    if (BeginPropertyGrid("##render_grid"))
    {
        if (m_grid)
        {
            DrawSliderFloatRow("Line Width X", m_grid->getLineWidthXPtr(), 0.001f, 0.1f, "%.4f");
            DrawSliderFloatRow("Line Width Y", m_grid->getLineWidthYPtr(), 0.001f, 0.1f, "%.4f");
            DrawSliderFloatRow("Scale", m_grid->getGridScalePtr(), 0.1f, 5.0f, "%.2f");
            DrawSliderFloatRow("Emissive", m_grid->getLineEmissiveIntensityPtr(), 0.0f, 8.0f, "%.2f");
            DrawColorEdit4Row("Line Color", m_grid->getLineColorPtr());
            DrawColorEdit4Row("Base Color", m_grid->getBaseColorPtr());
        }
        else
        {
            DrawUnavailableRow("Grid");
        }
        ImGui::EndTable();
    }

    DrawSubheading("Bloom");
    if (BeginPropertyGrid("##render_bloom"))
    {
        if (m_bloom)
        {
            DrawCheckboxRow("Enabled", m_bloom->getEnabledPtr());
            DrawSliderFloatRow("Threshold", m_bloom->getThresholdPtr(), 0.0f, 5.0f, "%.2f");
            DrawSliderFloatRow("Knee", m_bloom->getKneePtr(), 0.0f, 1.0f, "%.2f");
            DrawSliderFloatRow("Intensity", m_bloom->getIntensityPtr(), 0.0f, 5.0f, "%.2f");
            DrawSliderFloatRow("Upsample Scale", m_bloom->getUpsampleScalePtr(), 0.25f, 3.0f, "%.2f");
        }
        else
        {
            DrawUnavailableRow("Bloom");
        }
        ImGui::EndTable();
    }
}

void DebugUI::drawCombatPanel()
{
    if (!DrawPanelHeader("Combat"))
    {
        return;
    }

    DrawSubheading("Bullets");
    if (BeginPropertyGrid("##combat_bullets"))
    {
        if (m_bulletPool)
        {
            Bullet* bullets = m_bulletPool->getBullets();
            const int max = m_bulletPool->getMaxBullets();
            int activeCount = 0;

            for (int i = 0; i < max; i++)
            {
                if (bullets[i].isActive())
                {
                    activeCount++;
                }
            }

            DrawFormattedTextRow("Active", "%d / %d", activeCount, max);
            DrawProgressRow("Usage", max > 0 ? static_cast<float>(activeCount) / static_cast<float>(max) : 0.0f);

            if (activeCount >= max)
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Status");
                ImGui::TableNextColumn();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(DEBUG_COLOR_ERROR), "POOL FULL");
            }
        }
        else
        {
            DrawUnavailableRow("Bullet pool");
        }
        ImGui::EndTable();
    }

    if (m_boss)
    {
        DrawSubheading("Boss");
        if (BeginPropertyGrid("##combat_boss"))
        {
            const Vector3 pos = m_boss->getPosition();
            const float healthPercent = m_boss->getHealthPercent();

            char healthText[64] = {};
            std::snprintf(healthText, sizeof(healthText), "%.0f / %.0f",
                m_boss->getHealth(), m_boss->getMaxHealth());

            DrawTextRow("Activated", m_boss->isActivated() ? "YES" : "NO");
            DrawProgressRow("Health", healthPercent, healthText);
            DrawTextRow("Dead", m_boss->isDead() ? "YES" : "NO");
            DrawFormattedTextRow("Position", "%.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
            ImGui::EndTable();
        }
    }
}

void DebugUI::drawAudioPanel()
{
    if (!DrawPanelHeader("Audio", false))
    {
        return;
    }

    DrawSubheading("Mixer");
    if (BeginPropertyGrid("##audio_mixer"))
    {
        if (m_audioManager)
        {
            if (DrawSliderFloatRow("Master Volume", m_audioManager->getMasterVolumePtr(), 0.0f, 1.0f, "%.2f"))
            {
                m_audioManager->setMasterVolume(m_audioManager->getMasterVolume());
            }
        }
        else
        {
            DrawUnavailableRow("Audio");
        }
        ImGui::EndTable();
    }

    DrawSubheading("Beat Tracker");
    if (BeginPropertyGrid("##audio_beat"))
    {
        if (m_beatTracker)
        {
            DrawFormattedTextRow("Beat", "%d", m_beatTracker->getBeat());
            DrawProgressRow("Progress", m_beatTracker->getBeatProgress(), "Beat progress");
        }
        else
        {
            DrawUnavailableRow("Beat tracker");
        }
        ImGui::EndTable();
    }
}

void DebugUI::drawCompositionOverlay()
{
    if (!m_showThirds)
    {
        return;
    }

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    const ImVec2 screen = ImGui::GetIO().DisplaySize;

    const float x1 = screen.x / 3.0f;
    const float x2 = screen.x * 2.0f / 3.0f;
    const float y1 = screen.y / 3.0f;
    const float y2 = screen.y * 2.0f / 3.0f;

    constexpr ImU32 lineColor = IM_COL32(255, 255, 255, 80);
    constexpr ImU32 dotColor = IM_COL32(255, 255, 0, 150);
    constexpr float thickness = 1.0f;

    draw->AddLine(ImVec2(x1, 0), ImVec2(x1, screen.y), lineColor, thickness);
    draw->AddLine(ImVec2(x2, 0), ImVec2(x2, screen.y), lineColor, thickness);

    draw->AddLine(ImVec2(0, y1), ImVec2(screen.x, y1), lineColor, thickness);
    draw->AddLine(ImVec2(0, y2), ImVec2(screen.x, y2), lineColor, thickness);

    constexpr float dotRadius = 4.0f;
    draw->AddCircleFilled(ImVec2(x1, y1), dotRadius, dotColor);
    draw->AddCircleFilled(ImVec2(x2, y1), dotRadius, dotColor);
    draw->AddCircleFilled(ImVec2(x1, y2), dotRadius, dotColor);
    draw->AddCircleFilled(ImVec2(x2, y2), dotRadius, dotColor);
}

#pragma once

#include "imgui.h"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define PUSH_SCOPED_STYLE_COLOR(idx, col) ::AsphaltTas::GuiStyle::ScopedStyleColor CONCAT(_style_color_, __COUNTER__)(idx, col)

#define PUSH_SCOPED_STYLE_VAR(idx, val) ::AsphaltTas::GuiStyle::ScopedStyleVar CONCAT(_style_var_, __COUNTER__)(idx, val)

namespace AsphaltTas
{
    namespace GuiStyle
    {
        static constexpr const ImVec4 COLOR_RED          {0.8f, 0, 0, 1.0f};
        static constexpr const ImVec4 COLOR_GREEN        {0, 0.8f, 0, 1};
        static constexpr const ImVec4 COLOR_BLUE         {0, 0, 0.8f, 1};
        static constexpr const ImVec4 COLOR_ORANGE       {1.0f, 0.518f, 0.0f, 1.0f};
        static constexpr const ImVec4 COLOR_TRANSPARENT  {0, 0, 0, 0};
        static constexpr const ImVec4 COLOR_BLACK        {0, 0, 0, 1};

        struct ScopedStyleColor 
        {
            explicit ScopedStyleColor(ImGuiCol idx, const ImVec4 &col) noexcept 
            {
                ImGui::PushStyleColor(idx, col);
            }

            ~ScopedStyleColor() noexcept 
            {
                ImGui::PopStyleColor();
            }
        };

        struct ScopedStyleVar
        {
            explicit ScopedStyleVar(ImGuiStyleVar idx, const ImVec2 &val) noexcept 
            {
                ImGui::PushStyleVar(idx, val);
            }

            explicit ScopedStyleVar(ImGuiStyleVar idx, float val) noexcept 
            {
                ImGui::PushStyleVar(idx, val);
            }

            ~ScopedStyleVar() noexcept 
            {
                ImGui::PopStyleVar();
            }
        };
    }
}
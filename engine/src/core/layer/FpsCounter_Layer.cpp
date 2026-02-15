#include "core/layer/FpsCounter_Layer.h"

//ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/ImGuiFileDialog.h>

namespace CoreEngine
{
    void FpsCounter_Layer::OnImGuiRender()
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();

        ImGui::PushID(this);

        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x, vp->WorkPos.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        std::string unique_window_name = std::format("FpsCounter_Layer##{}", (void*)this);
        ImGui::Begin(unique_window_name.c_str(), nullptr,
            ImGuiWindowFlags_NoDecoration     |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove           |
            ImGuiWindowFlags_NoSavedSettings  |
            ImGuiWindowFlags_NoBackground     |
            ImGuiWindowFlags_NoFocusOnAppearing
        );
    
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.234375, 1.0, 0, 1));
        ImGui::SetWindowFontScale(1.4f);
        ImGui::Text("%d", static_cast<int>(ImGui::GetIO().Framerate));
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::PopID();
    }
}
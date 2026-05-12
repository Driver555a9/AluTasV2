#include "layer/GhostToolLayer.h"

#include "core/application/Application.h"
#include "core/utility/Assert.h"
#include "core/event/EventDispatcher.h"
#include "core/event/WindowEvents.h"
#include "core/model/PathModel.h"

#include "memory/MemoryRW.h"
#include "layer/GuiStyle.h"
#include "layer/GhostRenderLayer.h"
#include "common/Utility.h"

#include "imgui/ImGuiFileDialog.h"

namespace AsphaltTas
{
    GhostToolLayer::GhostToolLayer(CoreEngine::Window::Handle handle) noexcept : CoreEngine::Basic_Layer(handle)
    {
        ENGINE_ASSERT(! s_instance && "There ought only to be one GhostLayer object at one time.");
        s_instance = this;
    }

    GhostToolLayer::~GhostToolLayer() noexcept
    {
        s_instance = nullptr;
        GhostRenderLayer::DeleteInstance();
    }  

    void GhostToolLayer::OnEvent([[maybe_unused]] CoreEngine::Basic_Event& e) noexcept 
    {

    }

    void GhostToolLayer::OnUpdate([[maybe_unused]] CoreEngine::Units::MicroSecond dt) noexcept
    {
        
    }

    void GhostToolLayer::OnRender() noexcept
    {
        
    }

////////////////////////////////////
// @TODO Improve this: 
// DONE: 1. Improve GUI - Clear seperation between record & playback state
// DONE: 2. Prefer forcing automatic full run recording (tool should auto detect 0% to 100%)
// 3. Auto management system; Give name to hunting sessions -> automatically store the best (or all) ghosts in a custom format the tool can parse efficiently
// 4. Implement into GUI the ability to scroll and select a ghost with time + custom name to race against
////////////////////////////////////
    void GhostToolLayer::OnImGuiRender() noexcept
    {
        ImVec2 display = ImGui::GetIO().DisplaySize;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(display);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                               | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_WindowBg, GuiStyle::COLOR_BLACK);

        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowBorderSize, 0.0f);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowRounding, 0.0f);

        if (! ImGui::Begin("Camera Tool", nullptr, flags))
        {
            ImGui::End();
            return;
        }

        { // Scope to delete scoped styles
            const auto ChangeStateButton = [](bool is_on_now, void(*on_turn_off)(), void(*on_turn_on)(), const char* label) -> void 
            {
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, is_on_now ? GuiStyle::COLOR_RED : GuiStyle::COLOR_GREEN);
                if (ImGui::Button(std::format("{} {}", (is_on_now ? "Stop" : "Begin"), label).c_str()))
                {
                    is_on_now ? on_turn_off() : on_turn_on();
                }
            };

            if (ImGui::CollapsingHeader("Record", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {

            }
            
            if (ImGui::CollapsingHeader("Playback", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {
 

            }

            if (ImGui::CollapsingHeader("Customize Ghost", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ChangeStateButton(GhostRenderLayer::InstanceExists(), &GhostRenderLayer::DeleteInstance, &GhostRenderLayer::CreateInstance, "Displaying Ghost");

                ImGui::SameLine();
                if (ImGui::Button("Load Model##ghostmodelload"))
                {
                    ImGuiFileDialog::Instance()->OpenDialog("ghostmodelloadkey", "Load Model", ".*");
                }

                if (ImGuiFileDialog::Instance()->Display("ghostmodelloadkey"))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        //Dummy values
                        constexpr glm::vec3 pos = glm::vec3(0.0f);
                        constexpr glm::quat rot = glm::identity<glm::quat>();
                        constexpr glm::vec3 col = glm::vec3(1.0f);
                        if (GhostRenderLayer::CreateCustomGhostModel<CoreEngine::PathModel>(ImGuiFileDialog::Instance()->GetFilePathName(), pos, rot, col))
                        {
                            m_current_loaded_model = ImGuiFileDialog::Instance()->GetCurrentFileName();
                        }
                    }
                    ImGuiFileDialog::Instance()->Close();
                }

                ImGui::SameLine();
                if (m_current_loaded_model.empty())
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(std::format("Loaded Model: NONE").c_str());
                }
                else 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                    ImGui::TextUnformatted(std::format("Loaded Model: {}", m_current_loaded_model).c_str());
                }

                bool use_custom_color_shader = GhostRenderLayer::GetIsUsingCustomColorShader();
                if (ImGui::Checkbox("Use Single Color Shader", &use_custom_color_shader))
                {
                    GhostRenderLayer::SetUseCustomColorShader(use_custom_color_shader);
                }
                
                std::unique_ptr<CoreEngine::Basic_Model>& model = GhostRenderLayer::GetCurrentGhostModel();
                if (model)
                {
                    glm::vec3 model_scale = model->GetScale();
                    if (ImGui::DragFloat3("Model Scale", glm::value_ptr(model_scale), 0.01f, 0.001f, 100.0f))
                    {
                        model->SetScale(model_scale);
                    }
                }

                glm::vec4 ghost_color = GhostRenderLayer::GetGhostColor();
                if (ImGui::ColorPicker4("Ghost Color", glm::value_ptr(ghost_color)))
                {
                    GhostRenderLayer::SetGhostColor(ghost_color);
                }
            }
        }

        ImGui::End();
    }

    void GhostToolLayer::CreateInstance() noexcept
    {
        if (InstanceExists()) return;

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        constexpr CoreEngine::Window::WindowCreationConfig config 
        {
            .m_title                       = "Ghost Replay",
            .m_relative_size               = {0.4f, 0.8f},
            .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseButtonCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 0,
            .m_is_decorated                = true,
            .m_has_transparent_framebuffer = false,
            .m_is_clickthrough             = false
        };

        CoreEngine::Application::Get()->QueueCreateWindowAndPushLayer<GhostToolLayer>(config);
    }

    bool GhostToolLayer::InstanceExists() noexcept
    {
        return s_instance != nullptr;
    }

    void GhostToolLayer::DeleteInstance() noexcept
    {
        if (! s_instance) return;
        CoreEngine::Application::Get()->QueueDeleteWindowLayerStack(s_instance->m_handle);
    }

}
#include "tas/layer/GhostToolLayer.h"

#include "core/application/Application.h"
#include "core/utility/Assert.h"
#include "core/event/EventDispatcher.h"
#include "core/event/WindowEvents.h"
#include "core/model/PathModel.h"

#include "tas/servicethreads/ReadCurrentStateService.h"
#include "tas/servicethreads/ReplayRecorderService.h"
#include "tas/memory/MemoryRW.h"
#include "tas/layer/GuiStyle.h"
#include "tas/layer/GhostRenderLayer.h"
#include "tas/common/ReplaySerializer.h"
#include "tas/common/Utility.h"

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
                ChangeStateButton(ReplayRecorderService::GetThreadIsRunning(), &ReplayRecorderService::StopRecordThread, &ReplayRecorderService::LaunchRecordThread, "Recording");

                std::optional<Replay::Frame> frame = ReplayRecorderService::GetLastFrame();
                if (frame.has_value())
                {
                    const RaceProgressState& prog = frame->m_race_progress_state;

                    ImGui::SameLine();
                    ImGui::Text(std::format("Frames: {} Time: {}", ReplayRecorderService::GetAmountRecordedFrames(), 
                                            Utility::TimeToFormatedString(CoreEngine::Units::Convert<CoreEngine::Units::MilliSecond>(prog.m_lap_time))).c_str());

                    const float progress = prog.m_race_progress_percentage / 100.0f;
                    if (progress < 0.9995f)
                    {
                        const float t    = std::clamp(progress, 0.0f, 1.0f);
                        const float hue  = (1.0f - t) * 0.0f + t * (120.0f / 360.0f);
                        const ImVec4 col = ImColor::HSV(hue, 1.0f, 1.0f);
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_PlotHistogram, col);
                        ImGui::SameLine();
                        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), (std::to_string(static_cast<int>(progress * 100.0f))+"%").c_str());
                    }
                    else 
                    {
                        {
                            PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_ORANGE);
                            ImGui::SameLine();
                            if (ImGui::Button("Save##replaysave"))
                            {
                                ImGuiFileDialog::Instance()->OpenDialog("replaysavekey", "Save Replay", ".replay");
                            }
                        }
                        if (ImGuiFileDialog::Instance()->Display("replaysavekey"))
                        {
                            if (ImGuiFileDialog::Instance()->IsOk())
                            {
                                ReplaySerializer::SaveBinary(ReplayRecorderService::GetReplayCopy(), ImGuiFileDialog::Instance()->GetFilePathName());
                            }
                            ImGuiFileDialog::Instance()->Close();
                        }
                    }
                }
            }
            
            if (ImGui::CollapsingHeader("Playback", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {
                const std::optional<Replay>& cur_replay = GhostRenderLayer::GetCurrentReplayConstRef();

                {
                    const bool playback_enabled = GhostRenderLayer::GetReplayPlaybackIsOn();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, playback_enabled ? GuiStyle::COLOR_RED : GuiStyle::COLOR_GREEN);
                    if (ImGui::Button(std::format("{} {}", (playback_enabled ? "Stop" : "Begin"), "Playback").c_str()))
                    {
                        GhostRenderLayer::SetEnableReplayPlayback(!playback_enabled);
                    }
                }

                ImGui::SameLine();
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_ORANGE);
                    if (ImGui::Button("Load##replayload"))
                    {
                        ImGuiFileDialog::Instance()->OpenDialog("replayloadkey", "Load Replay", ".replay");
                    }
                }
                if (ImGuiFileDialog::Instance()->Display("replayloadkey"))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        GhostRenderLayer::SetCurrentReplay(ReplaySerializer::LoadBinary(ImGuiFileDialog::Instance()->GetFilePathName())); 
                    }
                    ImGuiFileDialog::Instance()->Close();
                }

                ImGui::SameLine();
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                    if (ImGui::Button("Clear"))
                    {
                        GhostRenderLayer::SetCurrentReplay(std::nullopt);
                    }
                }

                ImGui::SameLine();
                if (cur_replay.has_value())
                {
                    const std::optional<Replay::Frame>& cur_frame = cur_replay->GetCurrentFrame();
                    ImGui::TextUnformatted(std::format("Frame: {}/{}", cur_replay->GetCurrentIndex() + 1, cur_replay->GetAmountFrames()).c_str());
                    if (cur_frame.has_value())
                    {
                        ImGui::SameLine();
                        const RaceProgressState& prog = cur_frame->m_race_progress_state;
                        ImGui::TextUnformatted(std::format(" Time: {}", Utility::TimeToFormatedString(prog.m_lap_time.ConvertTo<CoreEngine::Units::MilliSecond>())).c_str());
                        ImGui::SameLine();

                        const float progress = prog.m_race_progress_percentage / 100.0f;
                        const float t    = std::clamp(progress, 0.0f, 1.0f);
                        const float hue  = (1.0f - t) * 0.0f + t * (120.0f / 360.0f);
                        const ImVec4 col = ImColor::HSV(hue, 1.0f, 1.0f);
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_PlotHistogram, col);
                        ImGui::SameLine();
                        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), (std::to_string(static_cast<int>(progress * 100.0f))+"%").c_str());
                    }
                }
                else 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted("No replay currently loaded.");
                }
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
#include "tas/common/FrontCar_CameraController.h"

#include "tas/memory/MemoryUtility.h"

namespace AsphaltTas
{
    FrontCar_CameraControler::FrontCar_CameraControler() noexcept : CoreEngine::Basic_CameraController(CoreEngine::Basic_CameraController::Type::Custom)
    {

    }

    void FrontCar_CameraControler::Update(CoreEngine::CameraReverseZ& camera, const CoreEngine::InputState& input_state, CoreEngine::Units::Second delta_time) noexcept
    {
        const glm::vec3 car_pos = m_racer_state.GetExtractedPosition();
        const glm::quat car_rot = m_racer_state.GetExtractedRotation();
        //const glm::vec3 velocity = m_racer_state.GetVelocity();

        const glm::vec3 target_pos = car_pos + (car_rot * m_offset);

        camera.SetPosition(target_pos);

        if (m_look_backwards)
            camera.SetRotation(car_rot);
        else
            camera.SetRotation(car_rot * glm::angleAxis(glm::radians(180.0f), glm::vec3(0,1,0)));
    }

    void FrontCar_CameraControler::SetRacerState(RacerState state) noexcept
    {
        m_racer_state = state;
    }

    float FrontCar_CameraControler::GetOffsetForward() noexcept
    {
        return m_offset.z;
    }

    void FrontCar_CameraControler::SetOffsetForward(float offset) noexcept
    {
        m_offset.z = offset;
    }

    float FrontCar_CameraControler::GetOffsetUp() noexcept
    {
        return m_offset.y;
    }

    void FrontCar_CameraControler::SetOffsetUp(float offset) noexcept
    {
        m_offset.y = offset;
    }

    float FrontCar_CameraControler::GetOffsetRight() noexcept
    {
        return m_offset.x;
    }

    void FrontCar_CameraControler::SetOffsetRight(float offset) noexcept
    {
        m_offset.x = -1.0f * offset;
    }   

    bool FrontCar_CameraControler::GetLookBackwards() noexcept
    {
        return m_look_backwards;
    }

    void FrontCar_CameraControler::SetLookBackwards(bool look_backwards) noexcept
    {
        m_look_backwards = look_backwards;
    }

}
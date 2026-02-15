#pragma once
//std
#include <optional>
#include <array>
#include <string>

//glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace CoreEngine
{
	class CameraReverseZ 
	{
		private:
			constexpr static inline glm::vec3 s_UP 	= glm::vec3(0.0f, 1.0f, 0.0f);

			glm::vec3 m_position;
			glm::quat m_rotation;

			float m_aspect_ratio;
			float m_near_plane;
			float m_far_plane;
			float m_fov_rad;

		public:

			explicit CameraReverseZ(const glm::vec3& position, float aspect_ratio, float fov_deg, float near_plane, glm::quat rot = glm::quat{});

			[[nodiscard]] glm::mat4 CalculateCameraMatrix(const std::optional<glm::vec3>& target_to_look_at = std::nullopt) const;

			[[nodiscard]] glm::mat4 CalculateViewMatrix(const std::optional<glm::vec3>& target_to_look_at = std::nullopt) const;
			[[nodiscard]] glm::mat4 CalculateProjectionMatrix() const;

			[[nodiscard]] std::array<glm::vec4, 5> GetViewProjPlanes(const std::optional<glm::vec3>& target_to_look_at) const noexcept;

			void SetPosition(const glm::vec3& position)    noexcept;
			void Move(const glm::vec3& movement)		   noexcept;
			void SetRotation(const glm::quat& rotation)    noexcept;
			void SetAspectRatio(float ratio)         	   noexcept;
			void SetNearPlane(float nearPlane)       	   noexcept;
			void SetFovRad(float fov)                	   noexcept;
			void SetFovDeg(float fov) 		       		   noexcept;

			[[nodiscard]] glm::vec3 GetForwardDirection()  const noexcept;
			[[nodiscard]] glm::vec3 GetRightDirection()    const noexcept;
			[[nodiscard]] glm::vec3 GetPosition() const noexcept;
			[[nodiscard]] glm::quat GetRotation() const noexcept;
			[[nodiscard]] float GetNearPlane()    const noexcept;
			[[nodiscard]] float GetFovRad()       const noexcept;
			[[nodiscard]] float GetFovDeg()       const noexcept;
			[[nodiscard]] float GetAspectRatio()  const noexcept;
			[[nodiscard]] std::string ToString()  const noexcept;

			[[nodiscard]] constexpr inline static glm::vec3 GetAbsoluteUp() { return s_UP; }
	};
}
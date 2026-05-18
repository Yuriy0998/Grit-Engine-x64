#include "CameraController.h"
#include "Camera.h"
#include "../core/Input.h"
#include "../core/Log.h"

#include <cmath>
#include <algorithm>

namespace grit {
    namespace render {

        static constexpr f32 kPi = 3.14159265358979323846f;
        static constexpr f32 kHalfPi = kPi * 0.5f;
        static constexpr f32 kPitchLimit = kHalfPi - 0.01f; // не доводим до полюса

        void CameraController::setPositionAndTarget(const vec3& pos, const vec3& target) {
            m_position = pos;
            vec3 dir{ target.x - pos.x, target.y - pos.y, target.z - pos.z };
            f32 lenXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);
            m_pitch = std::atan2(dir.y, lenXZ);

            // yaw: 0 — вдоль +Z, рост по часовой при взгляде сверху (правая система Y-up)
            // forward = (sin(yaw)*cos(pitch), sin(pitch), cos(yaw)*cos(pitch))
            m_yaw = std::atan2(dir.x, dir.z);

            m_speed = m_config.baseSpeed;
        }

        void CameraController::update(f32 dt) {
            if (!m_camera || !m_input) return;

            // --- Look-режим: захват мыши по ПКМ ---
            if (m_config.rightMouseToLook) {
                if (m_input->wasMousePressed(MouseButton::Right)) {
                    m_input->setMouseCaptured(true);
                    m_looking = true;
                }
                if (m_input->wasMouseReleased(MouseButton::Right)) {
                    m_input->setMouseCaptured(false);
                    m_looking = false;
                }
            }
            else {
                m_looking = m_input->isMouseCaptured();
            }

            // --- Mouse look ---
            if (m_looking) {
                f32 dx = (f32)m_input->mouseDeltaX();
                f32 dy = (f32)m_input->mouseDeltaY();
                if (dx != 0 || dy != 0) {
                    GRIT_INFO("mouse delta: %.0f %.0f", dx, dy);
                }

                // Сдвигаем углы. Знаки подобраны под нашу матрицу взгляда (Y-up, LH/RH — не важно
                // для управления; визуально: мышь вправо → камера поворачивается вправо).
                m_yaw += dx * m_config.mouseSensitivity;
                m_pitch += (m_config.invertY ? dy : -dy) * m_config.mouseSensitivity;

                // Wrap yaw в [-PI, PI]
                if (m_yaw > kPi) m_yaw -= 2.f * kPi;
                if (m_yaw < -kPi) m_yaw += 2.f * kPi;

                // Clamp pitch
                if (m_pitch > kPitchLimit) m_pitch = kPitchLimit;
                if (m_pitch < -kPitchLimit) m_pitch = -kPitchLimit;
            }

            // --- Регулировка базовой скорости колесом ---
            f32 wheel = m_input->wheelDelta();
            if (wheel != 0.f) {
                f32 factor = std::pow(m_config.wheelSpeedStep, wheel);
                m_speed *= factor;
                if (m_speed < m_config.minSpeed) m_speed = m_config.minSpeed;
                if (m_speed > m_config.maxSpeed) m_speed = m_config.maxSpeed;
            }

            // --- Векторы базиса камеры ---
            f32 cy = std::cos(m_yaw), sy = std::sin(m_yaw);
            f32 cp = std::cos(m_pitch), sp = std::sin(m_pitch);

            vec3 forward{ sy * cp,  sp,  cy * cp };
            vec3 right{ cy,       0.f, -sy }; // перпендикулярно forward на плоскости XZ
            vec3 worldUp{ 0.f, 1.f, 0.f };

            // --- Движение ---
            vec3 move{ 0.f, 0.f, 0.f };
            if (m_input->isDown(Key::W)) { move.x += forward.x; move.y += forward.y; move.z += forward.z; }
            if (m_input->isDown(Key::S)) { move.x -= forward.x; move.y -= forward.y; move.z -= forward.z; }
            if (m_input->isDown(Key::D)) { move.x += right.x;   move.y += right.y;   move.z += right.z; }
            if (m_input->isDown(Key::A)) { move.x -= right.x;   move.y -= right.y;   move.z -= right.z; }
            if (m_input->isDown(Key::E)) { move.y += worldUp.y; }
            if (m_input->isDown(Key::Q)) { move.y -= worldUp.y; }

            // Нормализация (чтобы по диагонали не было быстрее)
            f32 mlen = std::sqrt(move.x * move.x + move.y * move.y + move.z * move.z);
            if (mlen > 1e-5f) {
                move.x /= mlen; move.y /= mlen; move.z /= mlen;
            }

            f32 speed = m_speed;
            if (m_input->isDown(Key::LeftShift) || m_input->isDown(Key::RightShift))
                speed *= m_config.fastMultiplier;
            if (m_input->isDown(Key::LeftCtrl) || m_input->isDown(Key::RightCtrl))
                speed *= m_config.slowMultiplier;

            m_position.x += move.x * speed * dt;
            m_position.y += move.y * speed * dt;
            m_position.z += move.z * speed * dt;

            // --- Записываем в Camera ---
            m_camera->position = m_position;
            m_camera->target = vec3{
                m_position.x + forward.x,
                m_position.y + forward.y,
                m_position.z + forward.z
            };
            m_camera->up = worldUp;
        }

    } // namespace render
} // namespace grit
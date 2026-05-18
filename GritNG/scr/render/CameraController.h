#pragma once

#include "../core/Types.h"
#include "../core/Math.h"

namespace grit {

    class Input;

    namespace render { struct  Camera; }

    namespace render {

        // FPS-style free-fly camera controller.
        // - WASD     : вперёд/назад/влево/вправо
        // - Q/E      : вниз/вверх
        // - Mouse    : look (когда захвачен ПКМ или mouseCaptured)
        // - Shift    : ускорение
        // - Ctrl     : замедление
        // - Wheel    : регулировка базовой скорости
        class CameraController {
        public:
            struct Config {
                f32 mouseSensitivity = 0.0025f;    // рад на пиксель
                f32 baseSpeed = 5.0f;       // ед./сек
                f32 fastMultiplier = 4.0f;       // Shift
                f32 slowMultiplier = 0.25f;      // Ctrl
                f32 wheelSpeedStep = 1.25f;      // множитель за щелчок колеса
                f32 minSpeed = 0.1f;
                f32 maxSpeed = 200.0f;
                // Захватывать мышь автоматически при зажатии ПКМ.
                // Если false — управление мышью только когда уже captured (toggle снаружи).
                bool rightMouseToLook = true;
                bool invertY = false;
            };

            void setCamera(Camera* cam) { m_camera = cam; }
            void setInput(Input* in) { m_input = in; }

            Config& config() { return m_config; }
            const Config& config() const { return m_config; }

            // Установить позицию и направление (через target).
            // Внутренне вычисляет yaw/pitch.
            void setPositionAndTarget(const vec3& pos, const vec3& target);

            // Прямой доступ
            void  setPosition(const vec3& p) { m_position = p; }
            vec3  position() const { return m_position; }
            f32   yaw()      const { return m_yaw; }
            f32   pitch()    const { return m_pitch; }

            void update(f32 dt);

        private:
            Camera* m_camera = nullptr;
            Input* m_input = nullptr;

            Config  m_config;

            vec3    m_position{ 0.f, 0.f, 0.f };
            f32     m_yaw = 0.f;   // вокруг Y, 0 = смотрим вдоль +Z
            f32     m_pitch = 0.f;   // вверх/вниз, в радианах
            f32     m_speed = 5.0f;  // текущая базовая скорость (меняется колесом)

            bool    m_looking = false; // активен ли look-режим
        };

    } // namespace render
} // namespace grit
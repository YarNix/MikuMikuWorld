#pragma once
#include <array>
#include <vector>
#include <map>
#include <memory>
#include "Math.h"
#include "Score.h"
#include "Rendering/Texture.h"
#include "Rendering/Sprite.h"
#include "PreviewData.h"
#include "DirectXMath.h"

namespace DirectX 
{
    inline bool XMMatrixIsNull(FXMMATRIX M)
    {
        XMVECTOR zero = XMVectorZero();
        return  XMVector4Equal(M.r[0], zero) &&
                XMVector4Equal(M.r[1], zero) &&
                XMVector4Equal(M.r[2], zero) &&
                XMVector4Equal(M.r[3], zero);
    }
}

namespace MikuMikuWorld
{
    enum class SpriteType: int
    {
        NoteLeft,
        NoteMiddle,
        NoteRight,
        TraceDiamond,
        FlickArrowLeft,
        FlickArrowUp = FlickArrowLeft + 6,
        SimultaneousLine = FlickArrowUp + 6,
        HoldTick
    };

    enum class SpriteLayer : uint8_t // The max is 
    {
        OVER_NOTE_EFFECT,
        FLICK_ARROW,
        DIAMOND,
        BASE_NOTE,
        TICK_NOTE,
        HOLD_MID,
        HOLD_PATH,
        UNDER_NOTE_EFFECT
    };

    enum class ParticleEffectType : uint32_t
    {
        Lane,
        
        NoteTapCircular, 
        NoteTapLinear,

        NoteLongCircular,
        NoteLongLinear,

        NoteFlickCircular,
        NoteFlickLinear,
        NoteFlickDirectional,

        NoteCriticalCircular,
        NoteCriticalLinear,
        NoteCriticalDirectional,

        NoteFrictionCircular,
        NoteFrictionLinear,

        NoteFrictionCriticalCircular,
        NoteFrictionCriticalLinear,

        NoteLongAmongCircular,
        NoteLongAmongCriticalCircular,

        NoteLongSegmentCircular,
        NoteLongSegmentLinear,

        NoteLongCriticalSegmentCircular,
        NoteLongCriticalSegmentLinear,

        NoteTapLane,
        NoteCriticalLane,
        NoteCriticalFlickLane,

        NoteCriticalFlickCircular,
        NoteCriticalFlickLinear,

        NoteLongCriticalCircular,
        NoteLongCriticalLinear,

        NoteLongSegmentLinearEx,
        NoteLongCriticalSegmentLinearEx,

        SlotLongLinear,
        SlotLongCriticalLinear,

        Invalid = UINT32_MAX
    };

    class SpriteTransform
    {
        DirectX::XMMATRIX xx;
        std::unique_ptr<DirectX::XMMATRIX> xy;
        std::unique_ptr<DirectX::XMMATRIX> yx;
        DirectX::XMMATRIX yy;
        
        public:
        SpriteTransform(float v[64]);
        std::array<DirectX::XMFLOAT4, 4> apply(const std::array<DirectX::XMFLOAT4, 4>& vPos) const;
    };

    struct ParticleProperty
    {
        float from, to;
        Engine::Easing easing;
    };

    struct PropertyCoeff
    {
        std::unique_ptr<DirectX::XMMATRIX> r1_4, r5_8, sinr1_4, sinr5_8, cosr1_4, cosr5_8;

        DirectX::XMVECTOR compute(const DirectX::XMVECTOR & v1_4, const DirectX::XMVECTOR & v5_8) const;
    };

    struct Particle
    {
        int groupID;
        int spriteID;
        Color color;
        float start;
        float duration;
        std::array<ParticleProperty, 6> xywhta;
        PropertyCoeff xyCoeff, whCoeff, taCoeff;

        // Compute the static range[from, to] for each property from an instance of values
        std::array<Engine::Range, 6> compute(const std::array<float, 8>& values) const;
    };

    struct ParticleEffect
    {
        std::vector<int> groupSizes;
        std::vector<Particle> particles;
    };

    extern std::map<ParticleEffectType, float> particleEffectDuration;
    extern std::map<ParticleEffectType, ParticleEffectType> particleEffectFallback;
}

namespace MikuMikuWorld::Engine
{
    std::array<DirectX::XMFLOAT4, 4> quadvPos(float left, float right, float top, float bottom);
	std::array<DirectX::XMFLOAT4, 4> perspectiveQuadvPos(float left, float right, float top, float bottom);
	std::array<DirectX::XMFLOAT4, 4> perspectiveQuadvPos(float leftStart, float leftStop, float rightStart, float rightStop, float top, float bottom);
	std::array<DirectX::XMFLOAT4, 4> quadUV(const Sprite& sprite, const Texture& texture);
    std::array<DirectX::XMFLOAT4, 4> circularQuadvPos(float lane, float width, float height);
    std::array<DirectX::XMFLOAT4, 4> linearQuadvPos(float lane, float width, float height, float shear);

    Range getNoteVisualTime(Note const& note, Score const& score, float noteSpeed);

    std::array<DirectX::XMFLOAT4, 4> particleTransformQuad(const std::array<DirectX::XMFLOAT4, 4>& quad, const std::array<Engine::DrawingParticleProperty, 6>& props, float p);

    /// General helper functions for fixed values in the engine
    static inline float getNoteDuration(float noteSpeed)
    {
        return lerp(0.35, 4.f, std::pow(unlerp(12, 1, noteSpeed), 1.31f));
    }
    static inline float approach(float start_time, float end_time, float current_time)
    {
        return std::pow(1.06, 45 * lerp(-1, 0, unlerp(start_time, end_time, current_time)));
    }
    inline constexpr float STAGE_LANE_TOP = 47;
    inline constexpr float STAGE_LANE_BOTTOM = 803;
    inline constexpr float STAGE_LANE_HEIGHT = 850;
    inline constexpr float STAGE_LANE_WIDTH = 1420;
    inline constexpr float STAGE_NUM_LANES = 12;
    inline constexpr float STAGE_TEX_WIDTH = 2048;
    inline constexpr float STAGE_TEX_HEIGHT = 1176;
    inline constexpr float STAGE_NOTE_HEIGHT = 75;
    inline constexpr float STAGE_TARGET_WIDTH = 1920;
    inline constexpr float STAGE_TARGET_HEIGHT = 1080;
    inline constexpr float STAGE_ASPECT_RATIO = STAGE_TARGET_WIDTH / STAGE_TARGET_HEIGHT;
    inline constexpr float STAGE_ZOOM = 927 / 800.f; // Magic value
    inline constexpr float BACKGROUND_SIZE = 2462.25;

    // Scale the screen width such that 1 unit correspond to 1 lane 
    inline constexpr float STAGE_WIDTH_RATIO =
        Engine::STAGE_ZOOM * Engine::STAGE_LANE_WIDTH / (Engine::STAGE_TEX_HEIGHT * Engine::STAGE_ASPECT_RATIO) / Engine::STAGE_NUM_LANES;
    // Scale the screen height such that 1 unit correspond to the stage height
    inline constexpr float STAGE_HEIGHT_RATIO =
        Engine::STAGE_ZOOM * Engine::STAGE_LANE_HEIGHT / Engine::STAGE_TEX_HEIGHT;
    // Shift the stage up to align with the stage texture and make the top of the screen is 0
    inline constexpr float STAGE_TOP_RATIO = 
        0.5f + Engine::STAGE_ZOOM * Engine::STAGE_LANE_TOP / Engine::STAGE_TEX_HEIGHT;

    static inline float laneToLeft(float lane)
    {
        return lane - 6;
    }
    static inline float getNoteCenter(Note const& note)
    {
        return laneToLeft(note.lane) + note.width / 2.f; 
    }
    static inline float getNoteHeight()
    {
        return STAGE_NOTE_HEIGHT / STAGE_LANE_HEIGHT / 2.f;
    }
    static inline int getZIndex(SpriteLayer layer, float xOffset, float yOffset)
    {
        static_assert(sizeof(int) == sizeof(int32_t));
        // Implicitly clamp NaN to max value unlike normal clamp
        const auto floatClamp = [](float value, float min, float max) { return value < min ? min : (value <= max ? value : max); };
        const auto masknShift = [](int32_t value, int32_t mask, int offset) { return (value & mask) << offset; };
        const int32_t mask24 = 0xFFFFFF, mask4 = 0x0F;
        int32_t y = static_cast<int32_t>(floatClamp(1 - yOffset, 0, 1) * float(mask24) + 0.5f);
        int32_t x = static_cast<int32_t>(floatClamp(xOffset / 12.f + 0.5f, 0, 1) * float(12) + 0.5f);
        return INT32_MAX
            - masknShift(static_cast<int32_t>(layer), mask4, 32 - 4)
            - masknShift(y, mask24, 4)
            - masknShift(x, mask4, 0);
    }
}
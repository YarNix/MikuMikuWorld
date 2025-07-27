#include "PreviewEngine.h"
#include "Constants.h"

namespace MikuMikuWorld
{
	std::map<ParticleEffectType, float> particleEffectDuration = {
		{ ParticleEffectType::Lane, 0.3f },
		{ ParticleEffectType::NoteTapLane, 1.f },
		{ ParticleEffectType::NoteCriticalLane, 1.f },
        { ParticleEffectType::NoteCriticalFlickLane, 1.f },

        { ParticleEffectType::NoteTapCircular, 0.6f },
        { ParticleEffectType::NoteTapLinear, 0.5f },

        { ParticleEffectType::NoteLongCircular, 0.6f },
        { ParticleEffectType::NoteLongLinear, 0.5f },
        
        { ParticleEffectType::NoteFlickCircular, 0.6f },
        { ParticleEffectType::NoteFlickLinear, 0.5f },
        { ParticleEffectType::NoteFlickDirectional, 0.32f },
        
        { ParticleEffectType::NoteCriticalCircular, 0.6f },
        { ParticleEffectType::NoteCriticalLinear, 0.5f },
        
        { ParticleEffectType::NoteLongCriticalCircular, 0.6f },
        { ParticleEffectType::NoteLongCriticalLinear, 0.5f },
        
        { ParticleEffectType::NoteCriticalFlickCircular, 0.6f },
        { ParticleEffectType::NoteCriticalFlickLinear, 0.5f },
        { ParticleEffectType::NoteCriticalDirectional, 0.32f },

        { ParticleEffectType::NoteFrictionCircular, 0.6f },
        { ParticleEffectType::NoteFrictionLinear, 0.5f },

        { ParticleEffectType::NoteFrictionCriticalCircular, 0.6f },
        { ParticleEffectType::NoteFrictionCriticalLinear, 0.5f },

        { ParticleEffectType::NoteLongAmongCircular, 0.6f },
        { ParticleEffectType::NoteLongAmongCriticalCircular, 0.6f },

        { ParticleEffectType::NoteLongSegmentCircular, 1.f },
        { ParticleEffectType::NoteLongSegmentLinear, 1.f },

        { ParticleEffectType::NoteLongCriticalSegmentCircular, 1.f },
        { ParticleEffectType::NoteLongCriticalSegmentLinear, 1.f }
	};

    std::map<ParticleEffectType, ParticleEffectType> particleEffectFallback = {
        { ParticleEffectType::NoteTapLane, ParticleEffectType::Lane },
        { ParticleEffectType::NoteCriticalLane, ParticleEffectType::Lane },
        { ParticleEffectType::NoteCriticalFlickLane, ParticleEffectType::NoteCriticalLane },

        { ParticleEffectType::NoteCriticalFlickCircular, ParticleEffectType::NoteCriticalCircular },
        { ParticleEffectType::NoteCriticalFlickLinear, ParticleEffectType::NoteCriticalLinear },

        { ParticleEffectType::NoteFrictionCircular, ParticleEffectType::NoteLongAmongCircular },
        { ParticleEffectType::NoteFrictionLinear, ParticleEffectType::NoteLongLinear },

        { ParticleEffectType::NoteFrictionCriticalCircular, ParticleEffectType::NoteLongAmongCriticalCircular },
        { ParticleEffectType::NoteFrictionCriticalLinear, ParticleEffectType::NoteCriticalLinear },

        { ParticleEffectType::NoteLongCriticalCircular, ParticleEffectType::NoteCriticalCircular },
        { ParticleEffectType::NoteLongCriticalLinear, ParticleEffectType::NoteCriticalLinear },
    };

    SpriteTransform::SpriteTransform(float v[64]) : xx(v), xy(nullptr), yx(nullptr), yy(v + 48) 
    {
        DirectX::XMMATRIX tmp(v + 16);
        if (!DirectX::XMMatrixIsNull(tmp))
            xy = std::make_unique<DirectX::XMMATRIX>(tmp);
        tmp = DirectX::XMMATRIX{v + 32};
        if (!DirectX::XMMatrixIsNull(tmp))
            yx = std::make_unique<DirectX::XMMATRIX>(tmp);
    }

    std::array<DirectX::XMFLOAT4, 4> SpriteTransform::apply(const std::array<DirectX::XMFLOAT4, 4> &vPos) const
    {
        DirectX::XMVECTOR x = DirectX::XMVectorSet(vPos[0].x, vPos[1].x, vPos[2].x, vPos[3].x);
        DirectX::XMVECTOR y = DirectX::XMVectorSet(vPos[0].y, vPos[1].y, vPos[2].y, vPos[3].y);
        DirectX::XMVECTOR
            tx = !xy ? DirectX::XMVector4Transform(x, xx) : DirectX::XMVectorAdd(DirectX::XMVector4Transform(x, xx), DirectX::XMVector4Transform(x, *xy)),
            ty = !yx ? DirectX::XMVector4Transform(y, yy) : DirectX::XMVectorAdd(DirectX::XMVector4Transform(y, *yx), DirectX::XMVector4Transform(y, yy));
        return {{
            { DirectX::XMVectorGetX(tx), DirectX::XMVectorGetX(ty), vPos[0].z, vPos[0].z },
            { DirectX::XMVectorGetY(tx), DirectX::XMVectorGetY(ty), vPos[1].z, vPos[1].z },
            { DirectX::XMVectorGetZ(tx), DirectX::XMVectorGetZ(ty), vPos[2].z, vPos[2].z },
            { DirectX::XMVectorGetW(tx), DirectX::XMVectorGetW(ty), vPos[3].z, vPos[3].z }
        }};
    }

	DirectX::XMVECTOR PropertyCoeff::compute(const DirectX::XMVECTOR & v1_4, const DirectX::XMVECTOR & v5_8) const
    {
        DirectX::XMVECTOR result = DirectX::g_XMZero;
        if (r1_4)
            result = DirectX::XMVector4Transform(v1_4, *r1_4);
        if (r5_8)
            result = DirectX::XMVectorAdd(result, DirectX::XMVector4Transform(v5_8, *r5_8));
        DirectX::XMVECTOR two_pi_v1_4 = DirectX::XMVectorScale(v1_4, NUM_PI * 2);
        DirectX::XMVECTOR two_pi_v5_8 = DirectX::XMVectorScale(v5_8, NUM_PI * 2);
        if (sinr1_4)
            result = DirectX::XMVectorAdd(result, DirectX::XMVector4Transform(DirectX::XMVectorSinEst(two_pi_v1_4), *sinr1_4));
        if (sinr5_8)
            result = DirectX::XMVectorAdd(result, DirectX::XMVector4Transform(DirectX::XMVectorSinEst(two_pi_v5_8), *sinr5_8));
        if (cosr1_4)
            result = DirectX::XMVectorAdd(result, DirectX::XMVector4Transform(DirectX::XMVectorCosEst(two_pi_v1_4), *cosr1_4));
        if (cosr5_8)
            result = DirectX::XMVectorAdd(result, DirectX::XMVector4Transform(DirectX::XMVectorCosEst(two_pi_v5_8), *cosr5_8));
        return result;
    }

    std::array<Engine::Range, 6> Particle::compute(const std::array<float, 8> &values) const
    {
        DirectX::XMVECTOR r1_4 = DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(values.data()));
        DirectX::XMVECTOR r5_8 = DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(values.data() + 4));
        DirectX::XMVECTOR xyFromTo = DirectX::XMVectorAdd(DirectX::XMVectorSet(xywhta[0].from, xywhta[0].to, xywhta[1].from, xywhta[1].to), xyCoeff.compute(r1_4, r5_8));
        DirectX::XMVECTOR whFromTo = DirectX::XMVectorAdd(DirectX::XMVectorSet(xywhta[2].from, xywhta[2].to, xywhta[3].from, xywhta[3].to), whCoeff.compute(r1_4, r5_8));
        DirectX::XMVECTOR taFromTo = DirectX::XMVectorAdd(DirectX::XMVectorSet(xywhta[4].from, xywhta[4].to, xywhta[5].from, xywhta[5].to), taCoeff.compute(r1_4, r5_8));
        return {{
            { DirectX::XMVectorGetX(xyFromTo), DirectX::XMVectorGetY(xyFromTo) },
            { DirectX::XMVectorGetZ(xyFromTo), DirectX::XMVectorGetW(xyFromTo) },
            { DirectX::XMVectorGetX(whFromTo), DirectX::XMVectorGetY(whFromTo) },
            { DirectX::XMVectorGetZ(whFromTo), DirectX::XMVectorGetW(whFromTo) },
            { DirectX::XMVectorGetX(taFromTo), DirectX::XMVectorGetY(taFromTo) },
            { DirectX::XMVectorGetZ(taFromTo), DirectX::XMVectorGetW(taFromTo) },
        }};
    }
}

namespace MikuMikuWorld::Engine
{
	Range getNoteVisualTime(Note const& note, Score const& score, float noteSpeed)
	{
		float targetTime = accumulateScaledDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
		return {targetTime - getNoteDuration(noteSpeed), targetTime};
	}

    std::array<DirectX::XMFLOAT4, 4> quadvPos(float left, float right, float top, float bottom)
	{
		// Quad order, right->left, bottom->top
		return {{
			{ right, bottom, 0.0f, 1.0f },
			{ right,    top, 0.0f, 1.0f },
			{  left,    top, 0.0f, 1.0f },
			{  left, bottom, 0.0f, 1.0f }
		}};
	}

	std::array<DirectX::XMFLOAT4, 4> perspectiveQuadvPos(float left, float right, float top, float bottom)
	{
		// Quad order, right->left, bottom->top
		float x1 = right * top,    y1 = top,
				x2 = right * bottom, y2 = bottom,
				x3 = left * bottom,  y3 = bottom,
				x4 = left * top,     y4 = top;
		return {{
			{ x1, y1, 0.0f, 1.0f },
			{ x2, y2, 0.0f, 1.0f },
			{ x3, y3, 0.0f, 1.0f },
			{ x4, y4, 0.0f, 1.0f }
		}};
	}

    std::array<DirectX::XMFLOAT4, 4> perspectiveQuadvPos(float leftStart, float leftStop, float rightStart, float rightStop, float top, float bottom)
    {
        float 
			x1 = rightStart * top,   y1 = top,
			x2 = rightStop * bottom, y2 = bottom,
			x3 = leftStop * bottom,  y3 = bottom,
			x4 = leftStart * top,    y4 = top;
		return {{
			{ x1, y1, 0.0f, 1.0f },
			{ x2, y2, 0.0f, 1.0f },
			{ x3, y3, 0.0f, 1.0f },
			{ x4, y4, 0.0f, 1.0f }
		}};
    }

    std::array<DirectX::XMFLOAT4, 4> quadUV(const Sprite& sprite, const Texture &texture)
    {
		float left = sprite.getX1() / texture.getWidth();
		float right = sprite.getX2() / texture.getWidth();
		float top = sprite.getY1() / texture.getHeight();
		float bottom = sprite.getY2() / texture.getHeight();
        return quadvPos(left, right, top, bottom);
    }

    std::array<DirectX::XMFLOAT4, 4> circularQuadvPos(float lane, float width, float height)
    {
        float bottom = 1 + height;
        float top = 1 - height;

        return perspectiveQuadvPos(
            lane - width / top,
            lane - width / bottom,
            lane + width / top,
            lane + width / bottom,
            top, bottom
        );
    }

    std::array<DirectX::XMFLOAT4, 4> linearQuadvPos(float lane, float width, float height, float shear)
    {
        const float p = 1.125;

        float bottom = 1;
        float top = 1 - 2 * height;

        return {{
            { lane * p + width + shear, top, 0.f, 1.f },
            { lane + width, bottom, 0.f, 1.f },
            { lane - width, bottom, 0.f, 1.f },
            { lane * p - width + shear, top, 0.f, 1.f }
        }};
    }

    std::array<DirectX::XMFLOAT4, 4> particleTransformQuad(const std::array<DirectX::XMFLOAT4, 4>& quad, const std::array<Engine::DrawingParticleProperty, 6>& props, float p)
    {
		const float x = props[0].at(p);
		const float y = props[1].at(p);
		const float w = props[2].at(p);
		const float h = props[3].at(p);
		const float t = props[4].at(p);
		const DirectX::XMVECTOR sx = DirectX::XMVectorSet( w, w, -w, -w);
		const DirectX::XMVECTOR sy = DirectX::XMVectorSet(-h, h,  h, -h);
		const DirectX::XMVECTOR px = DirectX::XMVectorScale(
			DirectX::XMVectorAdd(
				DirectX::XMVectorReplicate(x + 1),
				DirectX::XMVectorSubtract(
					DirectX::XMVectorScale(sx, std::cos(t)),
					DirectX::XMVectorScale(sy, std::sin(t))
				)
			),
			0.5f
		);
		
		const DirectX::XMVECTOR py = DirectX::XMVectorScale(
			DirectX::XMVectorAdd(
				DirectX::XMVectorReplicate(y + 1),
				DirectX::XMVectorAdd(
					DirectX::XMVectorScale(sy, std::cos(t)),
					DirectX::XMVectorScale(sx, std::sin(t))
				)
			),
			0.5f
		);
		const DirectX::XMVECTOR tx = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(quad[2].x), DirectX::XMVectorReplicate(quad[1].x), px);
		const DirectX::XMVECTOR ty = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(quad[2].y), DirectX::XMVectorReplicate(quad[1].y), px);
		const DirectX::XMVECTOR bx = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(quad[3].x), DirectX::XMVectorReplicate(quad[0].x), px);
		const DirectX::XMVECTOR by = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(quad[3].y), DirectX::XMVectorReplicate(quad[0].y), px);

		const DirectX::XMVECTOR vx = DirectX::XMVectorLerpV(tx, bx, py);
		const DirectX::XMVECTOR vy = DirectX::XMVectorLerpV(ty, by, py);

		DirectX::XMFLOAT4 posX, posY;
		DirectX::XMStoreFloat4(&posX, vx);
		DirectX::XMStoreFloat4(&posY, vy);

        return {{
            {posX.x, posY.x, 0, 1},
            {posX.y, posY.y, 0, 1},
            {posX.z, posY.z, 0, 1},
            {posX.w, posY.w, 0, 1}
        }};
    }
}


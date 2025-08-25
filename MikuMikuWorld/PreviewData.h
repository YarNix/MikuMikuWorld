#pragma once
#include <random>
#include <array>
#include "Score.h"
#include "Math.h"

namespace MikuMikuWorld::Engine
{
	struct DrawingNote
	{
		int refID;
		Range visualTime;
	};

	struct DrawingLine
	{
		Range xPos;
		Range visualTime;
	};

	struct DrawingHoldTick
	{
		int refID;
		float center;
		Range visualTime;
	};

	struct DrawingHoldSegment
	{
		int endID;
		float headTime, headLeft, headRight;
		float tailTime, tailLeft, tailRight;
		float startTime, endTime;
		float activeTime;
		EaseType ease;
		bool isGuide;
	};

	struct DrawingParticleProperty
	{
		Range value;
		EasingFunc ease;
		inline float at(float p) const { return lerp(value.min, value.max, this->ease(p)); }
	};

	struct DrawingParticle
	{
		int effectType;
		int particleId;
		int refID;
		Range time;
		std::array<DrawingParticleProperty, 6> xywhta;
	};

	struct DrawData
	{
		float noteSpeed;
		int maxTicks;
		std::default_random_engine rng;
		std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
		std::vector<DrawingHoldTick> drawingHoldTicks;
		std::vector<DrawingHoldSegment> drawingHoldSegments;
		std::vector<DrawingParticle> drawingLaneEffects;
		std::vector<DrawingParticle> drawingCircularEffects;
		std::vector<DrawingParticle> drawingLinearEffects;
		std::vector<DrawingParticle> drawingFlatEffects;
		std::vector<DrawingParticle> drawingSlotEffects;
		std::vector<DrawingParticle> drawingSlotGlowEffects;
		bool hasLaneEffect;
		bool hasNoteEffect;
		bool hasSlotEffect;

		void clear();
		void calculateDrawData(Score const& score);
	};
}
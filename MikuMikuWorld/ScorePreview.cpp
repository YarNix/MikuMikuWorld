#include "ScorePreview.h"
#include "PreviewEngine.h"
#include "Rendering/Camera.h"
#include "ResourceManager.h"
#include "Colors.h"
#include "ImageCrop.h"
#include "Application.h"
#include "ApplicationConfiguration.h"

namespace MikuMikuWorld
{
	namespace Utils
	{
		// Scale a rectangle with specified aspect ratio to be visible inside the target rectangle
		inline void fitRect(float target_width, float target_height, long double source_aspect_ratio, float& width, float& height)
		{
			const float target_aspect_ratio = target_width / target_height;
			width  = target_aspect_ratio > source_aspect_ratio ? source_aspect_ratio * target_height : target_width;
			height = target_aspect_ratio < source_aspect_ratio ? target_width / source_aspect_ratio : target_height;
			return;
		}

		// Scale a rectangle with specified aspect ratio so it fill the area of the target rectangle
		inline void fillRect(float target_width, float target_height, long double source_aspect_ratio, float& width, float& height)
		{
			const float target_aspect_ratio = target_width / target_height;
			width  = target_aspect_ratio < source_aspect_ratio ? source_aspect_ratio * target_height : target_width;
			height = target_aspect_ratio > source_aspect_ratio ? target_width / source_aspect_ratio : target_height;
			return;
		}
	};

	static const int NOTE_SIDE_WIDTH = 91;
	static const int NOTE_SIDE_PAD = 10;
	static const int MAX_FLICK_SPRITES = 6;
	static const int HOLD_XCUTOFF = 36;
	static const int GUIDE_XCUTOFF = 3;
	static const int GUIDE_Y_TOP_CUTOFF = -41;
	static const int GUIDE_Y_BOTTOM_CUTOFF = -12;
	static Color defaultTint { 1.f, 1.f, 1.f, 1.f };

	ScorePreviewBackground::ScorePreviewBackground() : backgroundFile(), jacketFile{}, brightness(0.5f), frameBuffer{2048, 2048}, init{false}
	{

	}

	ScorePreviewBackground::~ScorePreviewBackground()
	{
		frameBuffer.dispose();
	}

	void ScorePreviewBackground::setBrightness(float value)
	{
		brightness = value;
	}

	void ScorePreviewBackground::update(Renderer* renderer, const Jacket& jacket)
	{
		init = true;
		backgroundFile = config.backgroundImage;
		jacketFile = jacket.getFilename();
		brightness = config.backgroundBrightness;
		bool useDefaultTexture = backgroundFile.empty();
		Texture backgroundTex = { useDefaultTexture ? IO::File::pathConcat(Application::getResDir(), "textures", "default.png") : backgroundFile };
		const float bgWidth = backgroundTex.getWidth(), bgHeight = backgroundTex.getHeight();
		if (bgWidth != frameBuffer.getWidth() || bgHeight != frameBuffer.getHeight())
			frameBuffer.resize(bgWidth, bgHeight);
		frameBuffer.bind();
		frameBuffer.clear();
		int shaderId;
		if ((shaderId = ResourceManager::getShader("basic2d")) == -1) return;
		Shader* basicShader = ResourceManager::shaders[shaderId];
		int index = ResourceManager::getTexture("stage");
		if (index == -1)
			return;
		const Texture& stage = ResourceManager::textures[ResourceManager::getTexture("stage")];
		basicShader->use();
		basicShader->setMatrix4("projection", Camera::getOffCenterOrthographicProjection(0, bgWidth, 0, bgHeight));
		renderer->beginBatch();
		renderer->drawRectangle({0, 0}, {bgWidth, bgHeight}, backgroundTex, 0, bgWidth, 0, bgHeight, defaultTint, 0);
		renderer->endBatch();
		if (useDefaultTexture && IO::File::exists(jacket.getFilename()))
			updateDrawDefaultJacket(renderer, jacket);
		frameBuffer.unblind();
		backgroundTex.dispose();
	}

	void ScorePreviewBackground::updateDrawDefaultJacket(Renderer* renderer, const Jacket& jacket)
	{
		if (jacket.getFilename().empty()) return;
		int index = ResourceManager::getTexture("stage");
		if (index == -1) return;
		const Texture& stage = ResourceManager::textures[index];
		const Sprite* sprite;
		if (stage.sprites.size() < STAGE_SPR_COUNT) return;
		int shaderId;
		if ((shaderId = ResourceManager::getShader("basic2d")) == -1) return;
		Shader* basicShader = ResourceManager::shaders[shaderId];
		if ((shaderId = ResourceManager::getShader("masking")) == -1) return;
		Shader* maskShader = ResourceManager::shaders[shaderId];
		
		DirectX::XMFLOAT4 defCol = {defaultTint.r, defaultTint.g, defaultTint.b, defaultTint.a};
		DirectX::XMFLOAT4 mainCol = { 1, 1, 1, 0.65 };
		DirectX::XMFLOAT4 mirrorCol = { 1, 1, 1, 0.35 };
		auto mainLeftPos = Engine::quadvPos(602, 602 + 264, 816, 816 + 174);
		auto mainRightPos = Engine::quadvPos(1205, 1205 + 200, 629, 629 + 114);
		auto mirrorLeftPos = Engine::quadvPos(615, 615 + 256, 1170, 1170 + 162);
		auto mirrorRightPos = Engine::quadvPos(1186, 1186 + 196, 1387, 1387 + 105);
		auto mainLeftMask = Engine::quadUV(stage.sprites[SPR_JACKET_LEFT_MASK], stage);
		auto mainRightMask = Engine::quadUV(stage.sprites[SPR_JACKET_RIGHT_MASK], stage);
		auto mirrorLeftMask = Engine::quadUV(stage.sprites[SPR_MIRROR_JACKET_LEFT_MASK], stage);
		auto mirrorRightMask = Engine::quadUV(stage.sprites[SPR_MIRROR_JACKET_RIGHT_MASK], stage);

		maskShader->use();
		maskShader->setInt("baseTex", 0);
		maskShader->setInt("maskTex", 1);
		maskShader->setMatrix4("projection", Camera::getOffCenterOrthographicProjection(0, 2048, 0, 2048));
		renderer->beginBatch();
		renderer->pushQuadMasked(mainLeftPos, DefaultJacket::getLeftUV(), mainLeftMask, mainCol, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mainRightPos, DefaultJacket::getRightUV(), mainRightMask, mainCol, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mirrorLeftPos, DefaultJacket::getLeftMirrorUV(), mirrorLeftMask, mirrorCol, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mirrorRightPos, DefaultJacket::getRightMirrorUV(), mirrorRightMask, mirrorCol, jacket.getTexID(), stage.getID());
		renderer->endBatch();
		
		basicShader->use();
		basicShader->setMatrix4("projection", Camera::getOffCenterOrthographicProjection(0, 2048, 0, 2048));
		renderer->beginBatch();
		renderer->pushQuad(mainLeftPos, mainLeftMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mainRightPos, mainRightMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mirrorLeftPos, mirrorLeftMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mirrorRightPos, mirrorRightMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);

		sprite = &stage.sprites[SPR_JACKET_WINDOW];
		renderer->drawRectangle({682, 497}, {686, 686}, stage, sprite->getX1(), sprite->getX2(), sprite->getY1(), sprite->getY2(), defaultTint, 1);
		sprite = &stage.sprites[SPR_MIRROR_JACKET_WINDOW];
		renderer->drawRectangle({699, 958}, {651, 650}, stage, sprite->getX1(), sprite->getX2(), sprite->getY1(), sprite->getY2(), defaultTint.scaleAlpha(0.6f), 1);
		renderer->endBatch();

		auto mainWindowPos = Engine::quadvPos(824, 824 + 400, 666, 666 + 384);
		auto mainWindowMask = Engine::quadUV(stage.sprites[SPR_JACKET_MASK], stage);
		auto mirrorWindowPos = Engine::quadvPos(834, 834 + 386, 1120, 1120 + 336);
		auto mirrorWindowMask = Engine::quadUV(stage.sprites[SPR_MIRROR_JACKET_MASK], stage);
		maskShader->use();
		renderer->beginBatch();
		renderer->pushQuadMasked(mainWindowPos, DefaultJacket::getCenterUV(), mainWindowMask, {1, 1, 1, 0.8}, jacket.getTexID(), stage.getID());
		renderer->pushQuadMasked(mirrorWindowPos, DefaultJacket::getMirrorCenterUV(), mirrorWindowMask, {1, 1, 1, 0.5}, jacket.getTexID(), stage.getID());
		renderer->endBatch();

		basicShader->use();
		renderer->beginBatch();
		renderer->pushQuad(mainWindowPos, mainWindowMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		renderer->pushQuad(mirrorWindowPos, mirrorWindowMask, DirectX::XMMatrixIdentity(), defCol, stage.getID(), 0);
		sprite = &stage.sprites[SPR_SEKAI_FLOOR];
		renderer->drawRectangle({0, 1251}, {sprite->getWidth(), sprite->getHeight()}, stage, sprite->getX1(), sprite->getX2(), sprite->getY1(), sprite->getY2(), defaultTint.scaleAlpha(0.8f), 1);
		renderer->endBatch();
	}

	bool ScorePreviewBackground::shouldUpdate(const Jacket& jacket) const
	{
		return !init || backgroundFile != config.backgroundImage || jacketFile != jacket.getFilename();
	}

	void ScorePreviewBackground::draw(Renderer *renderer, float scrWidth, float scrHeight) const
	{
		float bgScrWidth = frameBuffer.getWidth(), bgScrHeight = frameBuffer.getHeight(), targetWidth, targetHeight;
		if (!backgroundFile.empty())
		{
			Utils::fillRect(scrWidth, scrHeight, bgScrWidth / bgScrHeight, bgScrWidth, bgScrHeight);
			targetWidth = config.pvLockAspectRatio ? Engine::STAGE_TARGET_WIDTH : scrWidth;
			targetHeight = config.pvLockAspectRatio ? Engine::STAGE_TARGET_HEIGHT : scrHeight;
		}
		else
		{
			bgScrWidth = Engine::BACKGROUND_SIZE;
			bgScrHeight = Engine::BACKGROUND_SIZE;
			targetWidth = Engine::STAGE_TARGET_WIDTH;
			targetHeight = Engine::STAGE_TARGET_HEIGHT;
		}
		
		const float bgWidth = bgScrWidth / (targetWidth * Engine::STAGE_WIDTH_RATIO);
		const float bgLeft = -bgWidth / 2;
		const float bgHeight = bgScrHeight / (targetHeight * Engine::STAGE_HEIGHT_RATIO);
		const float centerY = 0.5f / Engine::STAGE_HEIGHT_RATIO + Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		const float bgTop = centerY + -bgHeight / 2;
		auto vPos = Engine::quadvPos(bgLeft, bgLeft + bgWidth, bgTop, bgTop + bgHeight);
		auto uv = Engine::quadvPos(0, 1, 0, 1);
		renderer->pushQuad(vPos, uv, DirectX::XMMatrixIdentity(), DirectX::XMFLOAT4(brightness, brightness, brightness, 1), frameBuffer.getTexture(), -10);
	}

	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getLeftUV()
	{
		return {{
			{  303.8 / 740, 504.8 / 740, 0, 0 },
			{  317.5 / 740, 297.7 / 740, 0, 0 },
			{    5.5 / 740, 278.3 / 740, 0, 0 },
			{     -8 / 740, 497.4 / 740, 0, 0 }
		}};
	}

	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getRightUV()
	{
		return {{
			{ 749.5 / 740, 377.7 / 740, 0, 0 },
			{ 738.2 / 740, 188.1 / 740, 0, 0 },
			{ 415.0 / 740, 171.4 / 740, 0, 0 },
			{ 432.1 / 740, 363.9 / 740, 0, 0 },
		}};
	}

	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getLeftMirrorUV()
	{
		return {{
			{ 292.761414 / 740, 247.401382 / 740, 0, 0 },
			{ 310.765869 / 740, 491.944763 / 740, 0, 0 },
			{   6.892246 / 740, 498.470642 / 740, 0, 0 },
			{  -6.246704 / 740, 258.264862 / 740, 0, 0 }
		}};
	}

	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getRightMirrorUV()
	{
		return {{
			{ 733.444458 / 740, 183.954681 / 740, 0, 0 },	
			{ 743.541321 / 740, 355.960449 / 740, 0, 0 },
			{ 418.899414 / 740, 332.759491 / 740, 0, 0 },
			{ 410.746246 / 740, 155.907684 / 740, 0, 0 },
		}};
	}

	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getCenterUV()
	{
		return {{
			{ 755.541687 / 740, 744.057861 / 740, 0, 0 },
			{ 739.961182 / 740,  -1.859504 / 740, 0, 0 },
			{   0.043696 / 740,  -1.859504 / 740, 0, 0 },
			{ -17.484388 / 740, 744.057861 / 740, 0, 0 }
		}};
	}

	std::array<DirectX::XMFLOAT4, 4> ScorePreviewBackground::DefaultJacket::getMirrorCenterUV()
	{
		return {{
			{ 747.697083 / 740,   2.164453 / 740, 0, 0 },
			{ 743.909424 / 740, 731.297241 / 740, 0, 0 },
			{  -1.864066 / 740, 731.297241 / 740, 0, 0 },
			{   3.837242 / 740,   2.164453 / 740, 0, 0 }
		}};
	}

	ScorePreviewWindow::ScorePreviewWindow() : previewBuffer{1920, 1080}, notesTex(), background()
	{

	}

	ScorePreviewWindow::~ScorePreviewWindow()
	{
		if(notesTex) notesTex->dispose();
	}

	void ScorePreviewWindow::update(ScoreContext& context, Renderer* renderer)
	{
		bool isWindowActive =  !ImGui::IsWindowDocked() || ImGui::GetCurrentWindow()->TabId == ImGui::GetWindowDockNode()->SelectedTabId;
		if (!isWindowActive)
			// Don't draw anything if the window is not active.
			// SFX and music updates are handled in the timeline
			return;
		if (context.scorePreviewDrawData.noteSpeed != config.pvNoteSpeed
		|| context.scorePreviewDrawData.hasLaneEffect != config.pvLaneEffect
		|| context.scorePreviewDrawData.hasNoteEffect != config.pvNoteEffect
		|| context.scorePreviewDrawData.hasSlotEffect != config.pvNoteGlow)
			context.scorePreviewDrawData.calculateDrawData(context.score);
		ImVec2 size = ImGui::GetContentRegionAvail() - ImVec2{ this->getScrollbarWidth(), 0 }; // Reserve space for the scrollbar
		ImVec2 position = ImGui::GetCursorScreenPos();
		ImRect boundaries = ImRect(position, position + size);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(boundaries.Min, boundaries.Max, 0xff202020);
		
		if (config.drawBackground && background.shouldUpdate(context.workingData.jacket))
			background.update(renderer, context.workingData.jacket);

		static int shaderId = ResourceManager::getShader("basic2d");
		static int pteShaderId = ResourceManager::getShader("particles");
		if (shaderId == -1) return;
		if (pteShaderId == -1) return;
		Shader* shader = ResourceManager::shaders[shaderId];
		Shader* pteShader = ResourceManager::shaders[pteShaderId];
		shader->use();

		// Create a projection to the engine coordinate system
		float width  = size.x, height = size.y;
		float scaledWidth = (config.pvLockAspectRatio ? Engine::STAGE_TARGET_WIDTH : size.x) * Engine::STAGE_WIDTH_RATIO;
		float scaledHeight = (config.pvLockAspectRatio ? Engine::STAGE_TARGET_HEIGHT : size.y) * Engine::STAGE_HEIGHT_RATIO;
		float scrTop  = (config.pvLockAspectRatio ? Engine::STAGE_TARGET_HEIGHT : size.y) * Engine::STAGE_TOP_RATIO;
		if (config.pvLockAspectRatio) Utils::fillRect(Engine::STAGE_TARGET_WIDTH, Engine::STAGE_TARGET_HEIGHT, size.x / size.y, width, height);
		scaledAspectRatio = scaledWidth / scaledHeight;

		auto view = DirectX::XMMatrixScaling(scaledWidth, scaledHeight, 1.f) * DirectX::XMMatrixTranslation(0.f, -scrTop, 0.f);
		auto projection = Camera::getOffCenterOrthographicProjection(-width / 2, width / 2, -height / 2, height / 2);
		auto viewProjection = view * projection;
		shader->setMatrix4("projection", viewProjection);

		if (previewBuffer.getWidth() != size.x || previewBuffer.getHeight() != size.y)
			previewBuffer.resize(size.x, size.y);
		previewBuffer.bind();
		previewBuffer.clear();

		renderer->beginBatch();
		if (config.drawBackground)
		{
			background.setBrightness(config.backgroundBrightness);
			background.draw(renderer, width, height);
		}
		drawStage(renderer);
		renderer->endBatch();

		renderer->beginBatch();
		drawLines(context, renderer);
		drawHoldCurves(context, renderer);
		drawHoldTicks(context, renderer);
		drawNotes(context, renderer);
		drawUnblendParticles(context, renderer);
		if (config.pvStageCover != 0){
			drawStageCover(renderer);
			renderer->endBatchWithDepthTest(GL_LEQUAL); // using depth test to cull the notes drawn
		}
		else
			renderer->endBatch();

		pteShader->use();
		pteShader->setMatrix4("viewProjection", viewProjection);
		pteShader->setFloat("blendFactor", 0.5); // full additive 0 - 1 full alpha blend
		renderer->beginBatch();
		drawParticles(context, renderer);
		renderer->endBatchWithBlending(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		previewBuffer.unblind();
		drawList->AddImage((ImTextureID)(size_t)previewBuffer.getTexture(), position, position + size);
	}

	void ScorePreviewWindow::updateUI(ScoreEditorTimeline& timeline, ScoreContext& context) const
	{
		updateToolbar(timeline, context);
		ImGuiIO io = ImGui::GetIO();
		float mouseWheel = io.MouseWheel * -1;
		if (!timeline.isPlaying() && ImGui::IsWindowHovered() && mouseWheel != 0)
		{
			context.currentTick += std::max(mouseWheel * TICKS_PER_BEAT / 2, (float)-context.currentTick);
			//timeline.focusCursor(context.currentTick, mouseWheel > 0 ? Direction::Up : Direction::Down);
		}
		updateScrollbar(timeline, context);
	}

	const Texture &ScorePreviewWindow::getNoteTexture()
	{
		// At smaller widths, due to mipmapping, the texture becomes blurry.
		// So we make a copy texture which have no mipmapping
		if (!notesTex)
		{
			const Texture& target_texture = ResourceManager::textures[noteTextures.notes];
			notesTex = std::make_unique<Texture>(target_texture.getFilename(), TextureFilterMode::Linear);
		}
		return *notesTex;
	}

	std::pair<float, float> ScorePreviewWindow::getNoteBound(const Note &note) const
	{
		float left = Engine::laneToLeft(note.lane), right = left + note.width;
		if (config.pvMirrorScore)
			std::swap(left *= -1, right *= -1);
		return std::make_pair(left, right);
	}

	std::pair<float, float> ScorePreviewWindow::getHoldStepBound(const Note &note, const Score &score, int curTick) const
	{
		auto& holdNotes = score.holdNotes.at(note.parentID);
		int curStepIdx = findHoldStep(holdNotes, note.ID);
		if (holdNotes.steps[curStepIdx].type != HoldStepType::Skip)
			return getNoteBound(note);
		
		int startStepIdx = curStepIdx;
		while (startStepIdx != 0 && holdNotes.steps[startStepIdx - 1].type == HoldStepType::Skip)
			startStepIdx--;
		const HoldStep& lastHoldStep = startStepIdx != 0 ? holdNotes.steps[startStepIdx - 1] : holdNotes.start;
		auto easeFunc = getEaseFunction(lastHoldStep.ease);

		const Note& startNote = score.notes.at(lastHoldStep.ID);
		auto [leftStart, rightStart] = getNoteBound(startNote);

		auto it = std::find_if(holdNotes.steps.begin() + curStepIdx, holdNotes.steps.end(), [](const HoldStep& step) { return step.type != HoldStepType::Skip; });
		const Note& endNote = score.notes.at(it == holdNotes.steps.end() ? holdNotes.end : it->ID);
		auto [leftStop, rightStop] = getNoteBound(endNote);

		float progress = unlerp(
			accumulateScaledDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
			accumulateScaledDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
			accumulateScaledDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges)
		);

		return std::make_pair(
			easeFunc(leftStart, leftStop, progress),
			easeFunc(rightStart, rightStop, progress)
		);
	}

	std::pair<float, float> MikuMikuWorld::ScorePreviewWindow::getHoldSegmentBound(const Note &note, const Score &score, int curTick) const
	{
		const auto isNotHoldSkip = [](const HoldStep& step) { return step.type != HoldStepType::Skip; };
		const HoldNote& holdNotes = score.holdNotes.at(note.ID);
		auto it = std::lower_bound(holdNotes.steps.begin(), holdNotes.steps.end(), curTick, [&score](const HoldStep& step, int tick) { return score.notes.at(step.ID).tick < tick; });
		auto rit = std::find_if(std::make_reverse_iterator(it), holdNotes.steps.rend(), isNotHoldSkip);
		const HoldStep& startHoldStep = rit == holdNotes.steps.rend() ? holdNotes.start : *rit;

		const Note& startNote = rit == holdNotes.steps.rend() ? note : score.notes.at(rit->ID);
		auto [leftStart, rightStart] = getNoteBound(startNote);
		if (startNote.tick == curTick) return std::make_pair(leftStart, rightStart);

		auto end = std::find_if(it, holdNotes.steps.end(), isNotHoldSkip);
		const Note& endNote = score.notes.at(end == holdNotes.steps.end() ? holdNotes.end : end->ID);
		auto [leftStop, rightStop] = getNoteBound(endNote);
		if (endNote.tick == curTick) return std::make_pair(leftStop, rightStop);

		auto easeFunc = getEaseFunction(startHoldStep.ease);
		float progress = unlerp(
			accumulateScaledDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
			accumulateScaledDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
			accumulateScaledDuration(curTick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges)
		);

		return std::make_pair(
			easeFunc(leftStart, leftStop, progress),
			easeFunc(rightStart, rightStop, progress)
		);
	}

	void ScorePreviewWindow::drawStage(Renderer* renderer)
	{
		int index = ResourceManager::getTexture("stage");
		if (index == -1)
			return;
		const Texture& stage = ResourceManager::textures[index];
		if (!isArrayIndexInBounds(SPR_SEKAI_STAGE, stage.sprites))
			return;
		const Sprite& stageSprite = stage.sprites[SPR_SEKAI_STAGE];
		const float stageWidth = (Engine::STAGE_TEX_WIDTH / Engine::STAGE_LANE_WIDTH) * Engine::STAGE_NUM_LANES;
		const float stageLeft  = -stageWidth / 2;
		const float stageTop = Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		const float stageHeight = Engine::STAGE_TEX_HEIGHT / Engine::STAGE_LANE_HEIGHT;
		renderer->drawRectangle({stageLeft, stageTop}, {stageWidth, stageHeight}, stage, stageSprite.getX1(), stageSprite.getX2(), stageSprite.getY1(), stageSprite.getY2(), defaultTint.scaleAlpha(config.pvStageOpacity), -1);
	}

	void ScorePreviewWindow::drawStageCover(Renderer* renderer)
	{
		int index = ResourceManager::getTexture("stage");
		if (index == -1)
			return;
		const Texture& stage = ResourceManager::textures[index];
		if (!isArrayIndexInBounds(SPR_SEKAI_STAGE, stage.sprites))
			return;
		const Sprite& stageSprite = stage.sprites[SPR_SEKAI_STAGE];
		const float stageWidth = (Engine::STAGE_TEX_WIDTH / Engine::STAGE_LANE_WIDTH) * Engine::STAGE_NUM_LANES;
		const float stageLeft = -stageWidth / 2, stageRight = stageWidth / 2;

		const float stageTop = Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT;
		const float stageHeight = config.pvStageCover * (1 - stageTop);
		const float spriteHeight = config.pvStageCover * (Engine::STAGE_LANE_HEIGHT - Engine::STAGE_LANE_TOP);

		auto vPos = Engine::quadvPos(stageLeft, stageRight, stageTop + stageHeight, stageTop);
		auto model = DirectX::XMMatrixTranslation(0, 0, 1);
		// The mask that cull the notes
		renderer->drawQuad(
			Engine::quadvPos(stageLeft, stageRight, stageTop + stageHeight, 0), model, stage,
			0, 0, 1, 1, defaultTint.scaleAlpha(0), 0
		);
		// The darken part to indicate notes won't spawn
		renderer->drawQuad(
			Engine::quadvPos(stageLeft, stageRight, stageTop + stageHeight, stageTop), model, stage,
			stageSprite.getX1(), stageSprite.getX2(), stageSprite.getY1(), stageSprite.getY1() + spriteHeight,
			Color{0.f, 0.f, 0.f, config.pvStageOpacity}, 0
		);

		const Texture& noteTex = getNoteTexture();
		size_t sprIndex = SPR_SIMULTANEOUS_CONNECTION;
		size_t transIndex = static_cast<size_t>(SpriteType::SimultaneousLine);
		if (!isArrayIndexInBounds(sprIndex, noteTex.sprites)) return;
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms)) return;
		const SpriteTransform& lineTransform = ResourceManager::spriteTransforms[transIndex];
		const Sprite& sprite = noteTex.sprites[sprIndex];
		float x = 0.12 * (1 - config.pvStageCover);
		vPos = lineTransform.apply(Engine::perspectiveQuadvPos(-6 - x, 6 + x, 1. + Engine::getNoteHeight(), 1. - Engine::getNoteHeight()));
		float y = stageTop + config.pvStageCover * (1 - stageTop);
		model = DirectX::XMMatrixScaling(y, y, 1.f);
		renderer->drawQuad(vPos, model, noteTex,
			sprite.getX1(), sprite.getX2(), sprite.getY1(), sprite.getY2(),
			defaultTint.scaleAlpha(config.pvStageOpacity), -1
		);
	}

	void ScorePreviewWindow::drawNotes(const ScoreContext& context, Renderer *renderer)
	{
		float current_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const auto& drawData = context.scorePreviewDrawData;
	
		for (auto& note : drawData.drawingNotes)
		{
			if (scaled_tm < note.visualTime.min || scaled_tm > note.visualTime.max)
				continue;
			const Note& noteData = context.score.notes.at(note.refID);
			float y = Engine::approach(note.visualTime.min, note.visualTime.max, scaled_tm);
			float l = Engine::laneToLeft(noteData.lane), r = Engine::laneToLeft(noteData.lane) + noteData.width;
			drawNoteBase(renderer, noteData, l, r, y);
			if (noteData.friction)
				drawTraceDiamond(renderer, noteData, l, r, y);
			if (noteData.isFlick()) 
				drawFlickArrow(renderer, noteData, y, current_tm);
		}
	}

	void ScorePreviewWindow::drawLines(const ScoreContext& context, Renderer* renderer)
	{
		if (!config.pvSimultaneousLine)
			return;
		if (noteTextures.notes == -1)
			return;
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const auto& drawData = context.scorePreviewDrawData.drawingLines;

		const Texture& texture = getNoteTexture();
		size_t sprIndex = SPR_SIMULTANEOUS_CONNECTION;
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& sprite = texture.sprites[sprIndex];
		size_t transIndex = static_cast<size_t>(SpriteType::SimultaneousLine);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms))
			return;
		const SpriteTransform& lineTransform = ResourceManager::spriteTransforms[transIndex];
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();

		for (auto& line : drawData)
		{
			if (scaled_tm < line.visualTime.min || scaled_tm > line.visualTime.max)
				continue;
			float noteLeft = line.xPos.min, noteRight = line.xPos.max;
			if (config.pvMirrorScore)
				std::swap(noteLeft *= -1, noteRight *= -1);
			float y = Engine::approach(line.visualTime.min, line.visualTime.max, scaled_tm);
			auto vPos = lineTransform.apply(Engine::perspectiveQuadvPos(noteLeft, noteRight, noteTop, noteBottom));
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);
			renderer->drawQuad(vPos, model, texture, sprite.getX1(), sprite.getX2(), sprite.getY1(), sprite.getY2(), defaultTint, Engine::getZIndex(SpriteLayer::UNDER_NOTE_EFFECT, 0, y));
		}
	}

	void ScorePreviewWindow::drawHoldTicks(const ScoreContext &context, Renderer *renderer)
	{
		if (noteTextures.notes == -1)
			return;
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const float w = Engine::getNoteHeight() / scaledAspectRatio;
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();
		const Texture& texture = getNoteTexture();

		for (auto& tick : context.scorePreviewDrawData.drawingHoldTicks)
		{
			if (scaled_tm < tick.visualTime.min || scaled_tm > tick.visualTime.max)
				continue;
			int sprIndex = getNoteSpriteIndex(context.score.notes.at(tick.refID));
			if (!isArrayIndexInBounds(sprIndex, texture.sprites))
				continue;
			const Sprite& sprite = texture.sprites[sprIndex];
			size_t transIndex = static_cast<size_t>(SpriteType::HoldTick);
			if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms))
				return;
			const SpriteTransform& transform = ResourceManager::spriteTransforms[transIndex];
			float y = Engine::approach(tick.visualTime.min, tick.visualTime.max, scaled_tm);
			const float tickCenter = tick.center * (config.pvMirrorScore ? -1 : 1);
			
			auto vPos = transform.apply(Engine::quadvPos(
				tickCenter - w, tickCenter + w,
				noteTop, noteBottom
			));
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);
			int zIndex = Engine::getZIndex(SpriteLayer::DIAMOND, tickCenter, y);
			renderer->drawQuad(vPos, model, texture, sprite.getX1(), sprite.getX2(), sprite.getY1(), sprite.getY2(), defaultTint, zIndex);
		}
	}

	void ScorePreviewWindow::drawHoldCurves(const ScoreContext &context, Renderer *renderer)
	{
		const float total_tm = accumulateScaledDuration(context.scorePreviewDrawData.maxTicks, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const float current_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		const float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const float noteDuration = Engine::getNoteDuration(config.pvNoteSpeed);
		const float mirror = config.pvMirrorScore ? -1 : 1;
		const auto& drawData = context.scorePreviewDrawData;
		
		for (auto& segment : drawData.drawingHoldSegments)
		{
			if (scaled_tm < (segment.headTime - noteDuration) || scaled_tm >= segment.tailTime)
				continue;
			
			const Note& holdEnd = context.score.notes.at(segment.endID);
			const Note& holdStart = context.score.notes.at(holdEnd.parentID);
			float holdStartCenter = Engine::getNoteCenter(holdStart) * mirror, holdStart_tm, holdEnd_tm;
			bool isActivated = current_tm >= segment.startTime;

			int textureID = segment.isGuide ? noteTextures.touchLine : noteTextures.holdPath;
			if (textureID == -1)
				continue;
			const Texture& pathTexture = ResourceManager::textures[textureID];
			const int sprIndex = (!holdStart.critical ? 1 : 3);
			if (!isArrayIndexInBounds(sprIndex, pathTexture.sprites))
				continue;
			const Sprite& pathSprite = pathTexture.sprites[sprIndex];

			// Clamp the segment to be within the visible stage
			float pathStart_tm = std::max(segment.headTime, scaled_tm);
			float pathEnd_tm = std::min(segment.tailTime, scaled_tm + noteDuration);
			
			const int steps = (segment.ease == EaseType::Linear ? 10 : 15)
				+ static_cast<int>(std::log(std::max((pathEnd_tm - pathStart_tm) / noteDuration, 4.5399e-5f)) + 0.5f); // Reduce steps if the segment is relatively small
			const auto ease = getEaseFunction(segment.ease);
			const Note& segmentStart = context.score.notes.at(segment.headID);
			float startLeft = Engine::laneToLeft(segmentStart.lane);
			float startRight = Engine::laneToLeft(segmentStart.lane) + segmentStart.width;
			const Note& segmentEnd = context.score.notes.at(segment.tailID);
			float endLeft = Engine::laneToLeft(segmentEnd.lane);
			float endRight = Engine::laneToLeft(segmentEnd.lane) + segmentEnd.width;

			if (scaled_tm > segment.headTime && context.score.holdNotes.at(holdStart.ID).startType == HoldNoteType::Normal)
			{
				float t = unlerp(segment.headTime, segment.tailTime, scaled_tm);
				float l = ease(startLeft, endLeft, t), r = ease(startRight, endRight, t);
				drawNoteBase(renderer, holdStart, l, r, 1, segment.startTime / total_tm);
				if (holdStart.friction)
					drawTraceDiamond(renderer, holdStart, l, r, 1);
			}

			if (config.pvMirrorScore)
			{
				std::swap(startLeft *= -1, startRight *= -1);
				std::swap(endLeft *= -1, endRight *= -1);
			}

			if (segment.isGuide)
			{
				holdStart_tm = accumulateScaledDuration(holdStart.tick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
				holdEnd_tm = accumulateScaledDuration(holdEnd.tick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
			}

			for (int i = 0; i < steps; i++)
			{
				float from_percentage = float(i) / steps, to_percentage = float(i + 1) / steps;
				float stepStart_tm = lerp(pathStart_tm, pathEnd_tm, from_percentage);
				float stepEnd_tm = lerp(pathStart_tm, pathEnd_tm, to_percentage);

				float stepTop    = Engine::approach(stepStart_tm - noteDuration, stepStart_tm, scaled_tm);
				float stepBottom = Engine::approach(stepEnd_tm   - noteDuration, stepEnd_tm  , scaled_tm);

				// Since we clamped the segment to only the visible parts
				// We need to figure out which part of the segment we are on
				float t_stepStart = unlerp(segment.headTime, segment.tailTime, stepStart_tm);
				float   t_stepEnd = unlerp(segment.headTime, segment.tailTime, stepEnd_tm);

				float stepStartLeft = ease(startLeft, endLeft, t_stepStart);
				float   stepEndLeft = ease(startLeft, endLeft, t_stepEnd);
				float stepStartRight = ease(startRight, endRight, t_stepStart);
				float   stepEndRight = ease(startRight, endRight, t_stepEnd);

				auto vPos = Engine::perspectiveQuadvPos(stepStartLeft, stepEndLeft, stepStartRight, stepEndRight, stepTop, stepBottom);
				auto model = DirectX::XMMatrixIdentity();
				float alpha = config.pvHoldAlpha;
				int zIndex = Engine::getZIndex(SpriteLayer::HOLD_PATH, holdStartCenter, segment.headTime / total_tm);

				float spr_x1, spr_x2, spr_y1, spr_y2;
				if (segment.isGuide)
				{
					spr_x1 = pathSprite.getX1() + GUIDE_XCUTOFF;
					spr_x2 = pathSprite.getX2() - GUIDE_XCUTOFF;
					spr_y1 = lerp(pathSprite.getY2() - GUIDE_Y_BOTTOM_CUTOFF, pathSprite.getY1() + GUIDE_Y_TOP_CUTOFF, unlerp(holdStart_tm, holdEnd_tm, stepStart_tm));
					spr_y2 = lerp(pathSprite.getY2() - GUIDE_Y_BOTTOM_CUTOFF, pathSprite.getY1() + GUIDE_Y_TOP_CUTOFF, unlerp(holdStart_tm, holdEnd_tm, stepEnd_tm));
				}
				else
				{
					spr_x1 = pathSprite.getX1() + HOLD_XCUTOFF;
					spr_x2 = pathSprite.getX2() - HOLD_XCUTOFF;
					spr_y1 = pathSprite.getY1();
					spr_y2 = pathSprite.getY2();
				}

				if (config.pvHoldAnimation && isActivated && isArrayIndexInBounds(sprIndex - 1, pathTexture.sprites))
				{
					const Sprite& activeSprite = pathTexture.sprites[sprIndex - 1];
					const int norm2ActiveOffset = activeSprite.getY1() - pathSprite.getY1();
					float delta_tm = current_tm - segment.startTime;
					float normalAplha = (std::cos(delta_tm * NUM_PI * 2) + 2) / 3.;

					renderer->drawQuad(vPos, model, pathTexture, spr_x1, spr_x2, spr_y1, spr_y2, defaultTint.scaleAlpha(alpha * normalAplha), zIndex);
					renderer->drawQuad(vPos, model, pathTexture, spr_x1, spr_x2, spr_y1 + norm2ActiveOffset, spr_y2 + norm2ActiveOffset, defaultTint.scaleAlpha(alpha * (1.f - normalAplha)), zIndex);
				}
				else
					renderer->drawQuad(vPos, model, pathTexture, spr_x1, spr_x2, spr_y1, spr_y2, defaultTint.scaleAlpha(alpha), zIndex);
			}
		}
	}

	void ScorePreviewWindow::drawUnblendParticles(const ScoreContext &context, Renderer *renderer)
	{
		const float current_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		int particleTexId = ResourceManager::getTexture("particles");
		if (particleTexId < 0) return;
		const Texture& texture = ResourceManager::textures[particleTexId];
		const Engine::DrawData& drawData = context.scorePreviewDrawData;
		for (auto& particle : drawData.drawingLaneEffects)
		{
			if (current_tm < particle.time.min || current_tm >= particle.time.max)
				continue;
			Particle& particleData = ResourceManager::particleEffects[particle.effectType].particles[particle.particleId];
			float progress = Engine::getParticleProgress((ParticleEffectType)particle.effectType, particleData, current_tm, particle.time.min, particle.time.max);
			const Note& note = context.score.notes.at(particle.refID);
			float noteLeft = Engine::laneToLeft(note.lane), noteRight = Engine::laneToLeft(note.lane) + note.width;
			if (config.pvMirrorScore) std::swap(noteLeft *= -1, noteRight *= -1);
			float noteCenter = noteLeft + (noteRight - noteLeft) / 2;
			auto vPos =  Engine::perspectiveQuadvPos(noteLeft, noteRight, Engine::STAGE_LANE_TOP / Engine::STAGE_LANE_HEIGHT, Engine::STAGE_TEX_HEIGHT / Engine::STAGE_LANE_HEIGHT);
			drawParticle(renderer, vPos, particle, progress, texture, texture.sprites[particleData.spriteID], Engine::getZIndex(SpriteLayer::UNDER_NOTE_EFFECT, noteCenter, 1 - progress));
		}
		size_t transIdx = static_cast<size_t>(SpriteType::Slot);
		if (!isArrayIndexInBounds(transIdx, ResourceManager::spriteTransforms)) return;
		const auto& slotTransform = ResourceManager::spriteTransforms[transIdx];
		for (auto& particle : drawData.drawingSlotEffects)
		{
			if (current_tm < particle.time.min || current_tm >= particle.time.max)
				continue;
			Particle& particleData = ResourceManager::particleEffects[particle.effectType].particles[particle.particleId];
			float progress = Engine::getParticleProgress((ParticleEffectType)particle.effectType, particleData, current_tm, particle.time.min, particle.time.max);
			const Note& note = context.score.notes.at(particle.refID);
			const Sprite& sprite = texture.sprites[particleData.spriteID];
			DirectX::XMMATRIX model = DirectX::XMMatrixIdentity();
			std::array<DirectX::XMFLOAT4, 4> vPos;
			float slotLeft = Engine::laneToLeft(note.lane), slotTop = 1 - Engine::getNoteHeight(), slotBottom = 1 + Engine::getNoteHeight();
			float alpha = particle.xywhta[5].at(progress);
			if (config.pvMirrorScore) slotLeft = (slotLeft + note.width) * -1;
			for (int i = 0; i < note.width; i++)
			{
				vPos = slotTransform.apply(Engine::perspectiveQuadvPos(slotLeft + i, slotLeft + i + 1.f, slotTop, slotBottom));
				renderer->drawQuad(vPos, model, texture, sprite.getX1(), sprite.getX2(), sprite.getY2(), sprite.getY1(),
					defaultTint.scaleAlpha(alpha), Engine::getZIndex(SpriteLayer::UNDER_NOTE_EFFECT, slotLeft + 0.5f, 1 - progress)
				);
			}
		}
	}

	void ScorePreviewWindow::drawParticles(const ScoreContext &context, Renderer *renderer)
	{
		const float current_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		int particleTexId = ResourceManager::getTexture("particles");
		if (particleTexId < 0) return;
		const Texture& texture = ResourceManager::textures[particleTexId];
		const Engine::DrawData& drawData = context.scorePreviewDrawData;
		for (auto& particle : drawData.drawingCircularEffects)
		{
			if (current_tm <= particle.time.min || current_tm >= particle.time.max)
				continue;
			Particle& particleData = ResourceManager::particleEffects[particle.effectType].particles[particle.particleId];
			float progress = Engine::getParticleProgress((ParticleEffectType)particle.effectType, particleData, current_tm, particle.time.min, particle.time.max);
			const Note& note = context.score.notes.at(particle.refID);
			float cirularWidth, circularHeight, noteLeft, noteRight;
			switch ((ParticleEffectType)particle.effectType)
			{
			case ParticleEffectType::NoteLongSegmentCircular:
			case ParticleEffectType::NoteLongCriticalSegmentCircular:
				cirularWidth = 3.5;
				circularHeight = 2.1;
				std::tie(noteLeft, noteRight) = getHoldSegmentBound(note, context.score, context.currentTick);
				break;
			default:
				cirularWidth = 1.75;
				circularHeight = 1.05;
				std::tie(noteLeft, noteRight) = getNoteBound(note);
				break;
			}
			float noteCenter = noteLeft + (noteRight - noteLeft) / 2;
			drawParticle(renderer,
				Engine::circularQuadvPos(noteCenter, cirularWidth, circularHeight * scaledAspectRatio),
				particle, progress, texture, texture.sprites[particleData.spriteID],
				Engine::getZIndex(SpriteLayer::PARTICLE_EFFECT, noteCenter, float(note.tick) / context.scorePreviewDrawData.maxTicks)
			);
		}
		for (auto& particle : drawData.drawingLinearEffects)
		{
			if (current_tm <= particle.time.min || current_tm >= particle.time.max)
				continue;
			Particle& particleData = ResourceManager::particleEffects[particle.effectType].particles[particle.particleId];
			float progress = Engine::getParticleProgress((ParticleEffectType)particle.effectType, particleData, current_tm, particle.time.min, particle.time.max);
			const Note& note = context.score.notes.at(particle.refID);
			float noteLeft, noteRight, shear = 0;
			switch ((ParticleEffectType)particle.effectType)
			{
			case ParticleEffectType::NoteLongSegmentLinear:
			case ParticleEffectType::NoteLongCriticalSegmentLinear:
			case ParticleEffectType::NoteLongSegmentCircularEx:
			case ParticleEffectType::NoteLongCriticalSegmentCircularEx:
				std::tie(noteLeft, noteRight) = getHoldSegmentBound(note, context.score, context.currentTick);
				break;
			case ParticleEffectType::NoteFlickDirectional:
			case ParticleEffectType::NoteCriticalDirectional:
				shear = (config.pvMirrorScore ? -1 : 1) * (note.flick == FlickType::Left ? -1 : (note.flick == FlickType::Right ? 1 : 0));
			default:
				std::tie(noteLeft, noteRight) = getNoteBound(note);
				break;
			}
			float noteCenter = noteLeft + (noteRight - noteLeft) / 2;
			drawParticle(renderer,
				Engine::linearQuadvPos(noteCenter, 1.0, 1.0 * scaledAspectRatio, shear),
				particle, progress, texture, texture.sprites[particleData.spriteID],
				Engine::getZIndex(SpriteLayer::PARTICLE_EFFECT, noteCenter, float(note.tick) / context.scorePreviewDrawData.maxTicks)
			);
		}
		for (auto& particle : drawData.drawingFlatEffects)
		{
			if (current_tm <= particle.time.min || current_tm >= particle.time.max)
				continue;
			Particle& particleData = ResourceManager::particleEffects[particle.effectType].particles[particle.particleId];
			float progress = Engine::getParticleProgress((ParticleEffectType)particle.effectType, particleData, current_tm, particle.time.min, particle.time.max);
			const Note& note = context.score.notes.at(particle.refID);
			float noteLeft, noteRight;
			switch ((ParticleEffectType)particle.effectType)
			{
			case ParticleEffectType::NoteLongAmongCircular:
			case ParticleEffectType::NoteLongAmongCriticalCircular:
				std::tie(noteLeft, noteRight) = getHoldStepBound(note, context.score, context.currentTick);
				break;
			default:
				std::tie(noteLeft, noteRight) = getNoteBound(note);
				break;
			}
			float noteCenter = noteLeft + (noteRight - noteLeft) / 2;
			drawParticle(
				renderer, Engine::quadvPos(noteCenter - 4, noteCenter + 4, 1 - 4 * scaledAspectRatio, 1 + 4 * scaledAspectRatio),
				particle, progress, texture, texture.sprites[particleData.spriteID],
				Engine::getZIndex(SpriteLayer::PARTICLE_EFFECT, noteCenter, float(note.tick) / context.scorePreviewDrawData.maxTicks)
			);
		}
		for (auto& particle : drawData.drawingSlotGlowEffects)
		{
			if (current_tm <= particle.time.min || current_tm >= particle.time.max)
				continue;
			Particle& particleData = ResourceManager::particleEffects[particle.effectType].particles[particle.particleId];
			float progress = Engine::getParticleProgress((ParticleEffectType)particle.effectType, particleData, current_tm, particle.time.min, particle.time.max);
			const Note& note = context.score.notes.at(particle.refID);
			float noteLeft, noteRight;
			switch ((ParticleEffectType)particle.effectType)
			{
			case ParticleEffectType::SlotGlowNoteLongSegment:
			case ParticleEffectType::SlotGlowNoteLongCriticalSegment:
				std::tie(noteLeft, noteRight) = getHoldSegmentBound(note, context.score, context.currentTick);
				break;
			default:
				std::tie(noteLeft, noteRight) = getNoteBound(note);
				break;
			}
			std::array<DirectX::XMFLOAT4, 4> layout = {{
				{ noteRight * 1.25f, scaledAspectRatio * 4.25f, 0.0f, 1.0f },
				{ noteRight, 		 1.0f,                      0.0f, 1.0f },
				{ noteLeft, 		 1.0f,                      0.0f, 1.0f },
				{ noteLeft * 1.25f,  scaledAspectRatio * 4.25f, 0.0f, 1.0f }
			}};
			float noteCenter = noteLeft + (noteRight - noteLeft) / 2;
			
			drawParticle(
				renderer, layout, particle, progress, texture, texture.sprites[particleData.spriteID],
				Engine::getZIndex(SpriteLayer::SLOT_GLOW_EFFECT, noteCenter, float(note.tick) / context.scorePreviewDrawData.maxTicks)
			);
		}
	}

	void ScorePreviewWindow::drawNoteBase(Renderer* renderer, const Note& note, float noteLeft, float noteRight, float y, float zScalar)
	{
		if (noteTextures.notes == -1)
			return;

		const Texture& texture = getNoteTexture();
		const int sprIndex = getNoteSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& sprite = texture.sprites[sprIndex];

		size_t transIndex = static_cast<size_t>(SpriteType::NoteMiddle);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms))
			return;
		const SpriteTransform& mTransform = ResourceManager::spriteTransforms[transIndex];

		transIndex = static_cast<size_t>(SpriteType::NoteLeft);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms))
			return;
		const SpriteTransform& lTransform = ResourceManager::spriteTransforms[transIndex];

		transIndex = static_cast<size_t>(SpriteType::NoteRight);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms))
			return;
		const SpriteTransform& rTransform = ResourceManager::spriteTransforms[transIndex];

		const float noteHeight = Engine::getNoteHeight();
		const float noteTop = 1. - noteHeight, noteBottom = 1. + noteHeight;
		if (config.pvMirrorScore)
			std::swap(noteLeft *= -1, noteRight *= -1);
		int zIndex = Engine::getZIndex(
			!note.friction ? SpriteLayer::BASE_NOTE : SpriteLayer::TICK_NOTE,
			noteLeft + (noteRight - noteLeft) / 2.f, y * zScalar
		);

		auto model = DirectX::XMMatrixScaling(y, y, 1.f);
		std::array<DirectX::XMFLOAT4, 4> vPos;
		// Middle
		vPos = mTransform.apply(Engine::perspectiveQuadvPos(noteLeft + 0.3f, noteRight - 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX1() + NOTE_SIDE_WIDTH, sprite.getX2() - NOTE_SIDE_WIDTH, sprite.getY1(), sprite.getY2(),
			defaultTint, zIndex
		);

		// Left slice
		vPos = lTransform.apply(Engine::perspectiveQuadvPos(noteLeft, noteLeft + 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX1() + NOTE_SIDE_PAD, sprite.getX1() + NOTE_SIDE_WIDTH, sprite.getY1(), sprite.getY2(),
			defaultTint, zIndex
		);
		
		// Right slice
		vPos = rTransform.apply(Engine::perspectiveQuadvPos(noteRight - 0.3, noteRight, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX2() - NOTE_SIDE_WIDTH, sprite.getX2() - NOTE_SIDE_PAD, sprite.getY1(), sprite.getY2(),
			defaultTint, zIndex
		);
	}

	void ScorePreviewWindow::drawTraceDiamond(Renderer *renderer, const Note &note, float noteLeft, float noteRight, float y)
	{
		if (noteTextures.notes == -1)
			return;
			
		const Texture& texture = getNoteTexture();
		int frictionSprIndex = getFrictionSpriteIndex(note);
		if (!isArrayIndexInBounds(frictionSprIndex, texture.sprites))
			return;
		const Sprite& frictionSpr = texture.sprites[frictionSprIndex];
		size_t transIndex = static_cast<size_t>(SpriteType::TraceDiamond);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransforms))
			return;
		const SpriteTransform& transform = ResourceManager::spriteTransforms[transIndex];

		const float w = Engine::getNoteHeight() / scaledAspectRatio;
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();
		if (config.pvMirrorScore)
			std::swap(noteLeft *= -1, noteRight *= -1);
		const float noteCenter = noteLeft + (noteRight - noteLeft) / 2;
		int zIndex = Engine::getZIndex(SpriteLayer::DIAMOND, noteCenter, y);

		auto vPos = transform.apply(Engine::quadvPos(
			noteCenter - w,
			noteCenter + w,
			noteTop, noteBottom)
		);
		auto model = DirectX::XMMatrixScaling(y, y, 1.f);
		
		renderer->drawQuad(vPos, model, texture, frictionSpr.getX1(), frictionSpr.getX2(), frictionSpr.getY1(), frictionSpr.getY2(), defaultTint, zIndex);
	}

	void ScorePreviewWindow::drawFlickArrow(Renderer *renderer, const Note &note, float y, float time)
	{
		if (noteTextures.notes == -1)
			return;

		const Texture& texture = getNoteTexture();
		const int sprIndex = getFlickArrowSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& arrowSprite = texture.sprites[sprIndex];
		size_t flickTransformIdx = clamp(note.width, 1, MAX_FLICK_SPRITES) - 1 + static_cast<int>((note.flick == FlickType::Left || note.flick == FlickType::Right) ? SpriteType::FlickArrowLeft : SpriteType::FlickArrowUp);
		if (!isArrayIndexInBounds(flickTransformIdx, ResourceManager::spriteTransforms))
			return;
		const SpriteTransform& transform = ResourceManager::spriteTransforms[flickTransformIdx];

		const int mirror = config.pvMirrorScore ? -1 : 1;
		const int flickDirection = mirror * (note.flick == FlickType::Left ? -1 : (note.flick == FlickType::Right ? 1 : 0));
		const float center = Engine::getNoteCenter(note) * mirror;
		const float w = clamp(note.width, 0, MAX_FLICK_SPRITES) * (note.flick == FlickType::Right ? -1 : 1) * mirror / 4.f;
		
		auto vPos = transform.apply(
			Engine::quadvPos(
				center - w,
				center + w,
				1, 1 - 2 * std::abs(w) * scaledAspectRatio
			)
		);
		int zIndex = Engine::getZIndex(SpriteLayer::FLICK_ARROW, center, y);
		
		if (config.pvFlickAnimation)
		{
			float t = std::fmod(time, 0.5f) / 0.5f;
			const auto cubicEaseIn = [](float t) { return t * t * t; };
			auto animationVector = DirectX::XMVectorScale(DirectX::XMVectorSet(flickDirection, -2 * scaledAspectRatio, 0.f, 0.f), t);
			auto model = DirectX::XMMatrixTranslationFromVector(animationVector) * DirectX::XMMatrixScaling(y, y, 1.f);
			
			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				defaultTint.scaleAlpha(1 - cubicEaseIn(t)), zIndex
			);
		}
		else
		{
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);

			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				defaultTint, zIndex
			);
		}
	}

	void ScorePreviewWindow::drawParticle(Renderer* renderer, const std::array<DirectX::XMFLOAT4, 4>& layout, const Engine::DrawingParticle& particle,
		float progress, const Texture& texture, const Sprite& sprite, int zIndex)
	{
		const float x = particle.xywhta[0].at(progress);
		const float y = particle.xywhta[1].at(progress);
		const float w = particle.xywhta[2].at(progress);
		const float h = particle.xywhta[3].at(progress);
		const float t = particle.xywhta[4].at(progress);
		const float a = particle.xywhta[5].at(progress);
		const DirectX::XMVECTOR sx = DirectX::XMVectorSet( w,  w, -w, -w );
		const DirectX::XMVECTOR sy = DirectX::XMVectorSet( h, -h, -h,  h );
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
		const DirectX::XMVECTOR tx = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(layout[2].x), DirectX::XMVectorReplicate(layout[1].x), px);
		const DirectX::XMVECTOR ty = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(layout[2].y), DirectX::XMVectorReplicate(layout[1].y), px);
		const DirectX::XMVECTOR bx = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(layout[3].x), DirectX::XMVectorReplicate(layout[0].x), px);
		const DirectX::XMVECTOR by = DirectX::XMVectorLerpV(DirectX::XMVectorReplicate(layout[3].y), DirectX::XMVectorReplicate(layout[0].y), px);

		const DirectX::XMVECTOR vx = DirectX::XMVectorLerpV(tx, bx, py);
		const DirectX::XMVECTOR vy = DirectX::XMVectorLerpV(ty, by, py);

		DirectX::XMFLOAT4 posX, posY;
		DirectX::XMStoreFloat4(&posX, vx);
		DirectX::XMStoreFloat4(&posY, vy);

		std::array<DirectX::XMFLOAT4, 4> vPos = {{
			{posX.x, posY.x, 0, 1},
			{posX.y, posY.y, 0, 1},
			{posX.z, posY.z, 0, 1},
			{posX.w, posY.w, 0, 1}
		}};

		renderer->drawQuad(vPos, DirectX::XMMatrixIdentity(), texture, sprite.getX1(), sprite.getX2(), sprite.getY1(), sprite.getY2(), defaultTint.scaleAlpha(a), zIndex);
	}
	
	void ScorePreviewWindow::updateToolbar(ScoreEditorTimeline &timeline, ScoreContext &context) const
	{
		static float lastHoveredTime = -1;
		constexpr float maxNoHoverTime = 1.5f;
		static float toolBarWidth = UI::btnNormal.x * 2;
		ImGuiIO io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImGui::GetWindowPos() + ImVec2{
			ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x * 4 - toolBarWidth,
			ImGui::GetStyle().WindowPadding.y * 5
		});
		ImGui::SetNextWindowSizeConstraints({48, 0}, {120, FLT_MAX}, NULL);
		float childBgAlpha = clamp(Engine::easeInCubic(unlerp(maxNoHoverTime, 0, lastHoveredTime)), 0.25f, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
		
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_WindowBg, childBgAlpha));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		
		ImGui::BeginChild("###toolbar", {0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		toolBarWidth = ImGui::GetWindowWidth();
		float centeredXBtn = toolBarWidth / 2 - UI::btnNormal.x / 2;
		if (ImGui::IsWindowHovered())
			lastHoveredTime = 0;
		else
			lastHoveredTime = std::min(io.DeltaTime + lastHoveredTime, maxNoHoverTime);

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(ICON_FA_ANGLE_DOUBLE_UP, UI::btnNormal, true, context.currentTick > 0))
		{
			if (timeline.isPlaying()) timeline.setPlaying(context, false);
			context.currentTick = std::max(timeline.roundTickDown(context.currentTick, timeline.getDivision()) - (TICKS_PER_BEAT / (timeline.getDivision() / 4)), 0);
		}

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(ICON_FA_ANGLE_UP, UI::btnNormal, true, context.currentTick > 0))
		{
			if (timeline.isPlaying()) timeline.setPlaying(context, false);
			context.currentTick--;
		}

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(ICON_FA_STOP, UI::btnNormal, false))
			timeline.stop(context);

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(timeline.isPlaying() ? ICON_FA_PAUSE : ICON_FA_PLAY, UI::btnNormal))
			timeline.setPlaying(context, !timeline.isPlaying());

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(ICON_FA_ANGLE_DOWN, UI::btnNormal, true, context.currentTick < context.scorePreviewDrawData.maxTicks + TICKS_PER_BEAT))
		{
			if (timeline.isPlaying()) timeline.setPlaying(context, false);
			context.currentTick++;
		}

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(ICON_FA_ANGLE_DOUBLE_DOWN, UI::btnNormal, true, context.currentTick < context.scorePreviewDrawData.maxTicks + TICKS_PER_BEAT))
		{
			if (timeline.isPlaying()) timeline.setPlaying(context, false);
			context.currentTick = timeline.roundTickDown(context.currentTick, timeline.getDivision()) + (TICKS_PER_BEAT / (timeline.getDivision() / 4));
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(ICON_FA_MINUS, UI::btnNormal, false, timeline.getPlaybackSpeed() > 0.25f))
			timeline.setPlaybackSpeed(context, timeline.getPlaybackSpeed() - 0.25f);

		const float playbackStrWidth = ImGui::CalcTextSize("0000%").x;
		ImGui::SetCursorPosX(toolBarWidth / 2 - playbackStrWidth / 2);
		UI::transparentButton(IO::formatString("%.0f%%", timeline.getPlaybackSpeed() * 100).c_str(), ImVec2{playbackStrWidth, UI::btnNormal.y }, false, false);

		ImGui::SetCursorPosX(centeredXBtn);
		if (UI::transparentButton(ICON_FA_PLUS, UI::btnNormal, false, timeline.getPlaybackSpeed() < 1.0f))
			timeline.setPlaybackSpeed(context, timeline.getPlaybackSpeed() + 0.25f);

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		float currentTm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		float currentScaledTm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		int currentMeasure = accumulateMeasures(context.currentTick, TICKS_PER_BEAT, context.score.timeSignatures);
		const TimeSignature& ts = context.score.timeSignatures[findTimeSignature(currentMeasure, context.score.timeSignatures)];
		const Tempo& tempo = getTempoAt(context.currentTick, context.score.tempoChanges);
		int hiSpeedIdx = findHighSpeedChange(context.currentTick, context.score.hiSpeedChanges);
		float speed = (hiSpeedIdx == -1 ? 1.0f : context.score.hiSpeedChanges[hiSpeedIdx].speed);

		char rhythmString[256];
		snprintf(rhythmString, sizeof(rhythmString), "%02d:%02d:%02d|%.2fs|%d/%d|%g BPM|%gx",
			static_cast<int>(currentTm / 60), static_cast<int>(std::fmod(currentTm, 60.f)), static_cast<int>(std::fmod(currentTm * 100, 100.f)),
			currentScaledTm,
			ts.numerator, ts.denominator,
			tempo.bpm,
			speed
		);
		char* str = strtok(rhythmString, "|");
		ImGui::SetCursorPosX(toolBarWidth / 2 - ImGui::CalcTextSize(str).x / 2);
		ImGui::Text(str);
		for (auto&& col : {feverColor, timeColor, tempoColor, speedColor})
		{
			str = strtok(NULL, "|");
			ImGui::SetCursorPosX(toolBarWidth / 2 - ImGui::CalcTextSize(str).x / 2);
			ImGui::TextColored(ImColor(col), str);
		}
		ImGui::EndChild();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
	}

	float ScorePreviewWindow::getScrollbarWidth() const
	{
		ImGuiStyle& style = ImGui::GetStyle();
		return style.ScrollbarSize + 4;
	}

    void ScorePreviewWindow::updateScrollbar(ScoreEditorTimeline &timeline, ScoreContext &context) const
	{
		constexpr float scrollpadY = 30.f;
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
		ImVec2 cursorBegPos = ImGui::GetCursorStartPos();
		ImVec2 scrollbarSize = { getScrollbarWidth(), contentSize.y - cursorBegPos.y };
		
		ImGui::SetCursorPos(cursorBegPos + ImVec2{ contentSize.x - scrollbarSize.x - style.WindowPadding.x / 2, 0 });
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg));
		ImGui::BeginChild("###scrollbar", scrollbarSize, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleColor();

		ImVec2 scrollContentSize = ImGui::GetContentRegionAvail();
		ImVec2 scrollMaxSize = ImGui::GetWindowContentRegionMax();
		int maxTicks = std::max(context.scorePreviewDrawData.maxTicks, 1);
		float scrollRatio = std::min(Engine::getNoteDuration(config.pvNoteSpeed) / accumulateDuration(context.scorePreviewDrawData.maxTicks, TICKS_PER_BEAT, context.score.tempoChanges), 1.f);
		float progress = std::min(float(context.currentTick) / maxTicks, 1.f);
		float handleHeight = std::max(20.f, scrollContentSize.y * scrollRatio);
		
		bool scrollbarActive = false;
		ImGui::BeginDisabled(timeline.isPlaying());
		ImGui::SetCursorPos(ImGui::GetCursorStartPos());
		ImGui::InvisibleButton("##scroll_bg", contentSize, ImGuiButtonFlags_NoNavFocus);
		scrollbarActive |= ImGui::IsItemActive();

		ImVec2 handleSize = {style.ScrollbarSize, handleHeight};
		ImVec2 handlePos = {scrollMaxSize.x / 2 - handleSize.x / 2, lerp(0, scrollMaxSize.y - handleHeight, progress)};
		ImVec2 absHandlePos = ImGui::GetWindowPos() + handlePos;

		ImGui::SetCursorPos(handlePos);
		ImGui::InvisibleButton("##scroll_handle", handleSize);
		scrollbarActive |= ImGui::IsItemActive();

		ImGuiCol_ handleColBase = scrollbarActive ? ImGuiCol_ScrollbarGrabActive : ImGui::IsItemHovered() ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab;
		
		ImGui::RenderFrame(absHandlePos, absHandlePos + ImGui::GetItemRectSize(), ImGui::GetColorU32(handleColBase), true, 3.f);
		ImGui::EndDisabled();

		if (scrollbarActive)
		{
			float absScrollStart = ImGui::GetWindowPos().y + handleSize.y / 2;
			float absScrollEnd = ImGui::GetWindowPos().y + scrollMaxSize.y - handleSize.y / 2;
			float mouseProgress = clamp(unlerp(absScrollStart, absScrollEnd, io.MousePos.y), 0.f, 1.f);
			context.currentTick = std::round(lerp(0, maxTicks, mouseProgress));
			
		}
		
		ImGui::EndChild();
	}
}
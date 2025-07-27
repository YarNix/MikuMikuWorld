#pragma once
#include <vector>
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"
#include "PreviewEngine.h"

namespace MikuMikuWorld
{
	class ResourceManager
	{
	public:
		static std::vector<Texture> textures;
		static std::vector<Shader*> shaders;
		static std::vector<SpriteTransform> spriteTransforms;
		static std::vector<ParticleEffect> particleEffects;

		static void loadTexture(const std::string& filename, TextureFilterMode minFilter = TextureFilterMode::Linear, TextureFilterMode magFilter = TextureFilterMode::Linear);
		static int getTexture(const std::string& name);
		static int getTextureByFilename(const std::string& filename);

		static void loadShader(const std::string& filename);
		static int getShader(const std::string& name);

		static void loadTransforms(const std::string& filename);

		static void loadParticleEffects(const std::string& filename);

		static void disposeTexture(int texID);
	};
}


#pragma once
#include "Graphics/Obj.h"
#include "Graphics/GLUtils.h"
#include "Misc/Const.h"

namespace TX {
	using namespace UI;
	class ObjViewer {
	public:
		struct LightSource {
			Color ambient;
			Color diffuse;
			Color specular;
			Vec4 position;
			Vec3 spotDirection;
			float spotExponent;
			float spotCutoff;
			float constantAttenuation;
			float linearAttenuation;
			float quadraticAttenuation;
			LightSource() :
				ambient(0.4, 0.4, 0.4, 1),
				diffuse(Color::WHITE),
				specular(Color::WHITE),
				position(0, 0, 1, 0),
				spotDirection(0, 0, -1),
				spotCutoff(180),
				constantAttenuation(1),
				linearAttenuation(0),
				quadraticAttenuation(0) {
			}
		};
		struct Prim {
			GL::Mesh mesh;
			std::shared_ptr<Primitive> prim;
			void SetMaterial(GL::Program& program) const;
			void Draw() const;
		};
	public:
		LightSource lightSource;
	public:
		ObjViewer(std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene);
	public:
		void Render();
	private:
		void UploadLight();
	private:
		enum Attribute {
			ATTRIB_POS,
			ATTRIB_NORMAL,
			ATTRIB_COUNT
		};
		enum Uniform {
			UNIFORM_M,
			UNIFORM_V,
			UNIFORM_P,
			UNIFORM_M_3X3_INV_TRANSP,
			UNIFORM_V_INV,

			UNIFORM_COUNT
		};
		GL::Program										program;
		GLuint											uniform[UNIFORM_COUNT];
		std::vector<Prim>								prims;
		std::shared_ptr<Camera>							camera;
		std::shared_ptr<Scene>							scene;

		/// <summary>
		/// Vertex shader code.
		/// </summary>
		constexpr static ConstString VertShaderSrc =
#include "phong.vs.glsl"
			;
		/// <summary>
		/// Fragment shader code.
		/// </summary>
		constexpr static ConstString FragShaderSrc =
#include "phong.fs.glsl"
			;
	};
}

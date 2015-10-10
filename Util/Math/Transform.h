#pragma once
#include "Util.h"
#include "Math/Matrix.h"
#include "Math/Ray.h"

namespace TX{
	using namespace Math;
	class Transform {
	public:
		Transform(){}
		Transform(const Transform& tr) : position_(tr.position_), local_world_(tr.local_world_), world_local_(tr.world_local_){}
		~Transform(){}
		inline Transform& operator = (const Transform& tr){ 
			position_ = tr.position_;
			local_world_ = tr.local_world_;
			world_local_ = tr.world_local_;
		}
		inline Vec3 Right() const { return Matrix4x4::TVec(local_world_, Vec3::X); }
		inline Vec3 Up() const { return Matrix4x4::TVec(local_world_, Vec3::Y); }
		inline Vec3 Forward() const { return Matrix4x4::TVec(local_world_, -Vec3::Z); }
		inline const Matrix4x4& WorldToLocalMatrix() const { return world_local_; }
		inline const Matrix4x4& LocalToWorldMatrix() const { return local_world_; }
		inline const Vec3& Position() const { return position_; }

		Transform& SetPosition(const Vec3& position);
		Transform& Translate(const Vec3& translation);
		Transform& Rotate(const Vec3& rotation);
		Transform& Rotate(float angle, const Vec3& axis);
		Transform& Scale(const Vec3& scale);
		Transform& LookAt(const Vec3& dir);
		Transform& LookAt(const Vec3& dir, const Vec3& up);
		//Transform& LookAt(const Vec3& pt_eye, const Vec3& pt_target, const Vec3& up);

		inline Transform& Translate(float x, float y, float z){ return Translate(Vec3(x, y, z)); }
		inline Transform& Rotate(float x, float y, float z){ return Rotate(Vec3(x, y, z)); }
		inline Transform& Scale(float x, float y, float z){ return Scale(Vec3(x, y, z)); }

		// Transforms a local ray to world space and normalize it.
		inline void ToWorld(Ray& ray) const {
			ray.origin = Matrix4x4::TPoint(local_world_, ray.origin);
			ray.dir = Normalize(Matrix4x4::TVec(local_world_, ray.dir));
		}

		// Transforms a global ray to local space WITHOUT normalizing it (so that t_max is valid)
		inline void ToLocal(Ray& ray) const{
			ray.origin = Matrix4x4::TPoint(world_local_, ray.origin);
			ray.dir = Matrix4x4::TVec(world_local_, ray.dir);
		}

	private:
		Vec3 position_;
		Matrix4x4 local_world_;
		Matrix4x4 world_local_;
	};
}
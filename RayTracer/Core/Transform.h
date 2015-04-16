#pragma once
#include "fwddecl.h"

#include "Matrix.h"
#include "Ray.h"

namespace Cruisky{
	namespace RayTracer
	{
		class Transform {
		public:
			Transform(){}
			Transform(Vector3 position, Vector3 rotation = Vector3::ZERO, Vector3 scale = Vector3::ONE){
				if(position != Vector3::ZERO) Translate(position);
				if(rotation != Vector3::ZERO) Rotate(rotation);
				if(scale != Vector3::ONE) Scale(scale);
			}
			~Transform(){}

			const Vector3& Position() const { return position_; }
			const Matrix4x4& WorldToLocalMatrix() const { return world_local_; }
			const Matrix4x4& LocalToWorldMatrix() const { return local_world_; }

			Transform& Translate(const Vector3& translation);
			Transform& Rotate(const Vector3& rotation);
			Transform& Rotate(float angle, const Vector3& axis);
			Transform& Scale(const Vector3& scale);
			Transform& LookAt(const Vector3& dir);
			Transform& LookAt(const Vector3& dir, const Vector3& up);
			//Transform& LookAt(const Vector3& pt_eye, const Vector3& pt_target, const Vector3& up);

			inline Transform& Translate(float x, float y, float z){ return Translate(Vector3(x, y, z)); }
			inline Transform& Rotate(float x, float y, float z){ return Rotate(Vector3(x, y, z)); }
			inline Transform& Scale(float x, float y, float z){ return Scale(Vector3(x, y, z)); }

			inline void ToWorld(Ray& ray) const {
				ray.origin = Matrix4x4::TPoint(local_world_, ray.origin);
				ray.dir = Matrix4x4::TVector(local_world_, ray.dir).Normalize();
			}

			inline void ToLocal(Ray& ray) const{
				ray.origin = Matrix4x4::TPoint(world_local_, ray.origin);
				ray.dir = Matrix4x4::TVector(world_local_, ray.dir).Normalize();
			}

		private:
			Vector3 position_;
			Matrix4x4 local_world_;
			Matrix4x4 world_local_;
		};
	}
}
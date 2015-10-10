#include "UtilStdAfx.h"
#include "Transform.h"

namespace TX{
	Transform& Transform::SetPosition(const Vec3& position){
		return Translate(position - position_);
	}

	Transform& Transform::Translate(const Vec3& translation){
		const Matrix4x4& mtr = Matrix4x4::Translate(translation);
		position_ = Matrix4x4::TPoint(mtr, position_);
		local_world_ = local_world_ * mtr;
		world_local_ = Matrix4x4::Translate(-translation) * world_local_;
		return *this;
	}

	Transform& Transform::Rotate(const Vec3& rotation){
		const Matrix4x4& mrot = Matrix4x4::Rotate(rotation);
		position_ = Matrix4x4::TPoint(mrot, position_);
		local_world_ = local_world_ * mrot;
		world_local_ = mrot.Transpose() * world_local_;
		return *this;
	}

	Transform& Transform::Rotate(float angle, const Vec3& axis){
		const Matrix4x4& mrot = Matrix4x4::Rotate(angle, axis);
		position_ = Matrix4x4::TPoint(mrot, position_);
		local_world_ = local_world_ * mrot;
		world_local_ = mrot.Transpose() * world_local_;
		return *this;
	}

	Transform& Transform::LookAt(const Vec3& dir){
		// find arbitrary vector that's perp to dir
		return LookAt(dir, Vec3(-1.f, -1.f, dir.x + dir.y));
	}

	Transform& Transform::LookAt(const Vec3& dir, const Vec3& up){
		local_world_ = Matrix4x4::LookAt(position_, position_ + dir, up);
		world_local_ = local_world_.Inverse(); //local_world_.InverseRotation();
		return *this;
	}

	Transform& Transform::Scale(const Vec3& scale){
		const Matrix4x4& mscal = Matrix4x4::Scale(scale);
		position_ = Matrix4x4::TPoint(mscal, position_);
		local_world_ = local_world_ * mscal;
		world_local_ = Matrix4x4::Scale(Vec3(1.f / scale.x, 1.f / scale.y, 1.f / scale.z)) * world_local_;
		return *this;
	}
}
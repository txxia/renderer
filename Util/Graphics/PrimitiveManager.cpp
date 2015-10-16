#include "UtilStdAfx.h"
#include "PrimitiveManager.h"
#include "Primitive.h"

namespace TX
{
	PrimitiveManager::PrimitiveManager(std::vector<std::shared_ptr<Primitive>> *prims) : prims_(prims) {}
	bool PrimitiveManager::Intersect(const Ray& ray, Intersection& intxn) const{
		bool hasIntxn = false;
		for (auto p : *prims_){
			if (p->Intersect(ray, intxn) && !hasIntxn)
				hasIntxn = true;
		}
		return hasIntxn;
	}

	bool PrimitiveManager::Occlude(const Ray& ray) const{
		for (auto p : *prims_){
			if (p->Occlude(ray))
				return true;
		}
		return false;
	}
}

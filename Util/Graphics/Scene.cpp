#include "UtilStdAfx.h"
#include "Scene.h"
#include "PrimitiveManager.h"
#include "Primitive.h"
#include "Intersection.h"

namespace TX {
	Scene::Scene(std::unique_ptr<PrimitiveManager> primmgr){
		primmgr_ = std::move(primmgr);
	}
	void Scene::AddPrimitive(std::shared_ptr<Primitive> prim){
		prim->transform.UpdateMatrix();
		prims_.push_back(prim);
	}
	void Scene::AddLight(std::shared_ptr<Light> light){
		light->scene_ = this;
		light->transform.UpdateMatrix();
		lights.push_back(light);
	}
	void Scene::Construct(){
		primmgr_->Construct(prims_);
	}
	bool Scene::Intersect(const Ray& ray, Intersection& intxn) const {
		return primmgr_->Intersect(ray, intxn);
	}
	void Scene::PostIntersect(const Ray& ray, LocalGeo& geo) const {
		geo.prim->PostIntersect(ray, geo);
	}
	bool Scene::Occlude(const Ray& ray) const {
		return primmgr_->Occlude(ray);
	}
}

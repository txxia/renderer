#pragma once
#include "fwddecl.h"

#include "SceneObject.h"
#include "Intersection.h"

namespace Cruisky {
	namespace RayTracer {
		
		class Light : public SceneObject {
		public:
			virtual ~Light(){};

			// Emits a light ray from point to the light source.
			virtual void Emit(const LocalGeo& geo, Ray *out, Color *lightcolor) const = 0;
		};
	}
}
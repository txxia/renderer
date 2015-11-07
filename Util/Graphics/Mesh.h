#pragma once
#include "Util.h"
#include "Shape.h"
#include "Math/Vector.h"

namespace TX {
	class Mesh {
	public:
		std::vector<Vec3> vertices;
		std::vector<Vec3> normals;
		std::vector<uint> indices;
		std::vector<Vec2> uv;
	public:
		Mesh() {}
		Mesh(const Mesh& ot) :
			vertices(ot.vertices),
			normals(ot.normals),
			indices(ot.indices),
			uv(ot.uv) {}
		inline Mesh& operator = (const Mesh& ot) {
			vertices = ot.vertices;
			normals = ot.normals;
			indices = ot.indices;
			uv = ot.uv;
			return *this;
		}
		~Mesh() {}

		inline uint VertexCount() const { return vertices.size(); }
		inline uint IndexCount() const { return indices.size(); }
		inline uint TriangleCount() const { return indices.size() / 3; }
		inline const uint* GetIndicesOfTriangle(uint triId) const { return &indices[triId * 3]; }
		inline void GetPoint(uint triId, float u, float v, Vec3 *out, Vec3 *normal) const {
			const uint *idx = GetIndicesOfTriangle(triId);
			const Vec3& v0 = vertices[*idx];
			const Vec3 e1 = vertices[*(++idx)] - v0;
			const Vec3 e2 = vertices[*(++idx)] - v0;
			if (out)
				*out = v0 + e1 * u + e2 * v;
			if (normal)
				*normal = Math::Normalize(Math::Cross(e1, e2));
		}

		/// <summary>
		/// Clear the vertices, normals, texcoords and triangle indices.
		/// </summary>
		virtual void Clear();
		/// <summary>
		/// Apply a transform to this mesh.
		/// </summary>
		virtual void ApplyTransform(const Transform& transform);
		/// <summary>
		/// Load a sphere to this mesh.
		/// </summary>
		/// <param name="radius"> Radius of the sphere </param>
		/// <param name="slices"> Number of slices (meridian) </param>
		/// <param name="stacks"> Number of stacks (ring of latitude on xz plane) </param>
		Mesh& LoadSphere(float radius = 1.f, uint slices = 72, uint stacks = 48);
		/// <summary>
		/// Load a plane to this mesh.
		/// The plane faces towards z-axis.
		/// </summary>
		/// <param name="size"> Edge size </param>
		Mesh& LoadPlane(float size = 1.f);

		bool Intersect(uint triId, const Ray& ray) const;
		bool Occlude(uint triId, const Ray& ray) const;
		void PostIntersect(LocalGeo& geo) const;
		float Area() const;
		float Area(uint triId) const;
	};

	class MeshSampler {
	public:
		std::shared_ptr<const Mesh> mesh;
		std::vector<float> areas;
		float sumArea, sumAreaRcp;
		std::unique_ptr<Distribution1D> areaDistro;
	public:
		MeshSampler(std::shared_ptr<const Mesh> mesh);
		inline float Area() const { return sumArea; }
		void SamplePoint(const Sample *sample, Vec3 *point, uint *id, Vec3 *normal) const;

		float Pdf(uint id, const Ray& wi) const;
		float Pdf(uint id, const Vec3& p) const;
	};
}

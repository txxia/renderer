#pragma once

#include "Util.h"
#include <intrin.h>
#include "Bool.h"

namespace TX
{
	namespace SSE
	{
		struct V4Int {
		public:
			union{
				__m128i m;
				struct{ int32 x, y, z, w; };
				int32 v[4];
			};
			static const V4Int ZERO;
			static const V4Int ONE;
		public:
			inline V4Int() : m(_mm_setzero_si128()) {}
			inline V4Int(__m128i mi) : m(mi) {}
			inline V4Int(const V4Int& ot) : m(ot.m) {}
			inline V4Int(int32 a) : m(_mm_set1_epi32(a)){}
			inline V4Int(int32 a, int32 b) : m(_mm_setr_epi32(a, b, a, b)){}
			inline V4Int(int32 a, int32 b, int32 c, int32 d) : m(_mm_setr_epi32(d, c, b, a)){}

			inline explicit V4Int(__m128 m) : m(_mm_cvtps_epi32(m)) {}
			inline V4Int& operator = (const V4Int& ot){ m = ot.m; return *this; }

			inline operator const __m128i&(void) const { return m; }
			inline operator       __m128i&(void)       { return m; }

			inline const int32& operator [] (const size_t idx) const { return v[idx]; }
			inline       int32& operator [] (const size_t idx)       { return v[idx]; }

			inline const V4Int operator + () const { return *this; }
			inline const V4Int operator - () const { return _mm_sub_epi32(_mm_setzero_si128(), m); }
			inline const V4Int operator + (const V4Int& ot) const { return _mm_add_epi32(m, ot.m); }
			inline const V4Int operator + (const int32& ot) const { return *this + V4Int(ot); }
			inline const V4Int operator - (const V4Int& ot) const { return _mm_sub_epi32(m, ot.m); }
			inline const V4Int operator - (const int32& ot) const { return *this - V4Int(ot); }
			inline const V4Int operator * (const V4Int& ot) const { return _mm_mullo_epi32(m, ot.m); }
			inline const V4Int operator * (const int32& ot) const { return *this * V4Int(ot); }
			inline const V4Int operator & (const V4Int& ot) const { return _mm_and_si128(m, ot.m); }
			inline const V4Int operator & (const int32& ot) const { return *this & V4Int(ot); }
			inline const V4Int operator | (const V4Int& ot) const { return _mm_or_si128(m, ot.m); }
			inline const V4Int operator | (const int32& ot) const { return *this | V4Int(ot); }
			inline const V4Int operator ^ (const V4Int& ot) const { return _mm_xor_si128(m, ot.m); }
			inline const V4Int operator ^ (const int32& ot) const { return *this ^ V4Int(ot); }
			inline const V4Int operator << (const int32& n) const { return _mm_slli_epi32(m, n); }
			inline const V4Int operator >> (const int32& n) const { return _mm_srai_epi32(m, n); }

			inline V4Int& operator += (const V4Int& ot) { return *this = *this + ot; }
			inline V4Int& operator += (const int32& ot) { return *this = *this + ot; }
			inline V4Int& operator -= (const V4Int& ot) { return *this = *this - ot; }
			inline V4Int& operator -= (const int32& ot) { return *this = *this - ot; }
			inline V4Int& operator *= (const V4Int& ot) { return *this = *this * ot; }
			inline V4Int& operator *= (const int32& ot) { return *this = *this * ot; }
			inline V4Int& operator &= (const V4Int& ot) { return *this = *this & ot; }
			inline V4Int& operator &= (const int32& ot) { return *this = *this & ot; }
			inline V4Int& operator |= (const V4Int& ot) { return *this = *this | ot; }
			inline V4Int& operator |= (const int32& ot) { return *this = *this | ot; }
			inline V4Int& operator <<= (const int32& ot) { return *this = *this << ot; }
			inline V4Int& operator >>= (const int32& ot) { return *this = *this >> ot; }

			inline const V4Bool operator == (const V4Int& ot) const { return _mm_cmpeq_epi32(m, ot.m); }
			inline const V4Bool operator == (const int32& ot) const { return *this == V4Int(ot); }
			inline const V4Bool operator != (const V4Int& ot) const { return !(*this == ot); }
			inline const V4Bool operator != (const int32& ot) const { return *this != V4Int(ot); }
			inline const V4Bool operator < (const V4Int& ot) const { return _mm_cmplt_epi32(m, ot.m); }
			inline const V4Bool operator < (const int32& ot) const { return *this < V4Int(ot); }
			inline const V4Bool operator >= (const V4Int& ot) const { return !(*this < ot); }
			inline const V4Bool operator >= (const int32& ot) const { return *this >= V4Int(ot); }
			inline const V4Bool operator > (const V4Int& ot) const { return _mm_cmpgt_epi32(m, ot.m); }
			inline const V4Bool operator > (const int32& ot) const { return *this > V4Int(ot); }
			inline const V4Bool operator <= (const V4Int& ot) const { return !(*this > ot); }
			inline const V4Bool operator <= (const int32& ot) const { return *this <= V4Int(ot); }
		};

		inline const V4Int Abs(const V4Int& v) { return _mm_abs_epi32(v.m); }
		inline const V4Int Min(const V4Int& a, const V4Int& b) { return _mm_min_epi32(a.m, b.m); }
		inline const V4Int Max(const V4Int& a, const V4Int& b) { return _mm_max_epi32(a.m, b.m); }

		template<bool n0, bool n1, bool n2, bool n3>
		inline const V4Int Negate(const V4Int& v){ return _mm_xor_si128(v.m, SSE::SIGN_MASK[(n3 << 3) | (n2 << 2) | (n1 << 1) | n0]); }

		inline const V4Int Blend(const V4Int& a, const V4Int& b, const V4Bool& fb){ return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(b), _mm_castsi128_ps(a), _mm_castsi128_ps(fb))); }
		inline V4Int UnpackLow(const V4Int& a, const V4Int& b) { return _mm_unpacklo_epi32(a.m, b.m); }
		inline V4Int UnpackHigh(const V4Int& a, const V4Int& b) { return _mm_unpackhi_epi32(a.m, b.m); }
		
		template<size_t v0, size_t v1, size_t v2, size_t v3>
		inline const V4Int Shuffle(const V4Int& a){ return _mm_shuffle_epi32(a, _MM_SHUFFLE(v3, v2, v1, v0)); }
		template<size_t a0, size_t a1, size_t b2, size_t b3> 
		inline const V4Int Shuffle(const V4Int& a, const V4Int& b){ return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(b3, b2, a1, a0))); }
	}
}

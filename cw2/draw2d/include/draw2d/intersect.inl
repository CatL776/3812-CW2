#include <algorithm>
#include <cmath>

inline
bool intersect_circle_aabb( Vec2f aCircleCenter, float aRadius, Vec2f aAABBMin, Vec2f aAABBMax ) noexcept
{
	if( aRadius < 0.f || !std::isfinite(aRadius)
		|| !std::isfinite(aCircleCenter.x) || !std::isfinite(aCircleCenter.y)
		|| !std::isfinite(aAABBMin.x) || !std::isfinite(aAABBMin.y)
		|| !std::isfinite(aAABBMax.x) || !std::isfinite(aAABBMax.y) )
		return false;
	double const x = std::clamp( double(aCircleCenter.x),
		double(std::min(aAABBMin.x, aAABBMax.x)), double(std::max(aAABBMin.x, aAABBMax.x)) );
	double const y = std::clamp( double(aCircleCenter.y),
		double(std::min(aAABBMin.y, aAABBMax.y)), double(std::max(aAABBMin.y, aAABBMax.y)) );
	double const dx = double(aCircleCenter.x) - x;
	double const dy = double(aCircleCenter.y) - y;
	return dx * dx + dy * dy <= double(aRadius) * aRadius;
}


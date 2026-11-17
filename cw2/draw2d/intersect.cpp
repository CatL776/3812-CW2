#include <draw2d/intersect.hpp>

#include <algorithm>
#include <cmath>

bool intersect_circle_tri( Vec2f aCircleCenter, float aRadius, Vec2f aP0, Vec2f aP1, Vec2f aP2 ) noexcept
{
	auto finite = []( Vec2f p ) {
		return std::isfinite(p.x) && std::isfinite(p.y);
	};
	if( aRadius < 0.f || !std::isfinite(aRadius) || !finite(aCircleCenter)
		|| !finite(aP0) || !finite(aP1) || !finite(aP2) )
		return false;
	auto edge = []( Vec2f a, Vec2f b, Vec2f p ) {
		return (double(b.x) - a.x) * (double(p.y) - a.y)
			- (double(b.y) - a.y) * (double(p.x) - a.x);
	};
	double const area = edge( aP0, aP1, aP2 );
	double const e0 = edge( aP0, aP1, aCircleCenter );
	double const e1 = edge( aP1, aP2, aCircleCenter );
	double const e2 = edge( aP2, aP0, aCircleCenter );
	if( area != 0.0
		&& ((e0 >= 0.0 && e1 >= 0.0 && e2 >= 0.0)
			|| (e0 <= 0.0 && e1 <= 0.0 && e2 <= 0.0)) )
		return true;

	double const radiusSquared = double(aRadius) * aRadius;
	auto touchesSegment = [&]( Vec2f a, Vec2f b ) {
		double const dx = double(b.x) - a.x;
		double const dy = double(b.y) - a.y;
		double const px = double(aCircleCenter.x) - a.x;
		double const py = double(aCircleCenter.y) - a.y;
		double const lengthSquared = dx * dx + dy * dy;
		double const t = lengthSquared == 0.0 ? 0.0
			: std::clamp( (px * dx + py * dy) / lengthSquared, 0.0, 1.0 );
		double const distanceX = px - t * dx;
		double const distanceY = py - t * dy;
		return distanceX * distanceX + distanceY * distanceY <= radiusSquared;
	};
	//Handling cases where the circle contains a triangle or a degenerate triangle
	return touchesSegment(aP0, aP1) || touchesSegment(aP1, aP2) || touchesSegment(aP2, aP0);
}


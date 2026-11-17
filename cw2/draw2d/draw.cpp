#include <draw2d/draw.hpp>

#include <algorithm>

#include <cmath>
#include <cassert>

#include <draw2d/sampler.hpp>
#include <draw2d/surface.hpp>

namespace
{
	inline
	unsigned classify_( Rect2F const& aRect, Vec2f const& aPoint )
	{
		unsigned ret{};
		if( aPoint.x < aRect.xmin )
			ret |= 1;
		else if( aPoint.x > aRect.xmin + aRect.width - 1.f )
			ret |= 2;
		
		if( aPoint.y < aRect.ymin )
			ret |= 4;
		else if( aPoint.y > aRect.ymin + aRect.height - 1.f )
			ret |= 8;
		
		return ret;
	}
}

// Implement in Task 1.3
void draw_triangle_interp( Surface& aSurface, Vec2f aP0, Vec2f aP1, Vec2f aP2, ColorF aC0, ColorF aC1, ColorF aC2 )
{
	// Evaluate barycentric coordinates at pixel centers and visit only the
	// triangle's clipped integer bounding box.
	float const denominator = (aP1.y - aP2.y) * (aP0.x - aP2.x)
		+ (aP2.x - aP1.x) * (aP0.y - aP2.y);
	if( std::abs(denominator) <= 1e-8f )
		return;

	//Only check the screen area where the triangle is located
	float const minX = std::min({ aP0.x, aP1.x, aP2.x });
	float const maxX = std::max({ aP0.x, aP1.x, aP2.x });
	float const minY = std::min({ aP0.y, aP1.y, aP2.y });
	float const maxY = std::max({ aP0.y, aP1.y, aP2.y });

	int const xBegin = std::max( 0, int(std::floor(minX)) );
	int const yBegin = std::max( 0, int(std::floor(minY)) );
	int const xEnd = std::min( int(aSurface.get_width()), int(std::ceil(maxX)) );
	int const yEnd = std::min( int(aSurface.get_height()), int(std::ceil(maxY)) );
	if( xBegin >= xEnd || yBegin >= yEnd )
		return;

	float const invDenominator = 1.f / denominator;
	for( int y = yBegin; y < yEnd; ++y )
	{
		for( int x = xBegin; x < xEnd; ++x )
		{
			//Calculate the color using the position of the pixel center
			float const px = float(x) + 0.5f;
			float const py = float(y) + 0.5f;

			float const w0 = ((aP1.y - aP2.y) * (px - aP2.x)
				+ (aP2.x - aP1.x) * (py - aP2.y)) * invDenominator;
			float const w1 = ((aP2.y - aP0.y) * (px - aP2.x)
				+ (aP0.x - aP2.x) * (py - aP2.y)) * invDenominator;
			float const w2 = 1.f - w0 - w1;

			//Pixels outside the triangle are skipped
			if( w0 < 0.f || w1 < 0.f || w2 < 0.f )
				continue;

			//First blend the colors, then convert them to colors for screen use
			ColorF const color{
				w0 * aC0.r + w1 * aC1.r + w2 * aC2.r,
				w0 * aC0.g + w1 * aC1.g + w2 * aC2.g,
				w0 * aC0.b + w1 * aC1.b + w2 * aC2.b
			};
			aSurface.set_pixel_srgb( Surface::Index(x), Surface::Index(y), linear_to_srgb(color) );
		}
	}
}

// Implement in Task 2.3
void draw_triangle_tex_masked( Surface& aSurface, Vec2f aP0, Vec2f aP1, Vec2f aP2, Sampler2D const& aSampler, Vec2f aT0, Vec2f aT1, Vec2f aT2 )
{
	auto finite = []( Vec2f p ) {
		return std::isfinite(p.x) && std::isfinite(p.y);
	};
	if( !finite(aP0) || !finite(aP1) || !finite(aP2)
		|| !finite(aT0) || !finite(aT1) || !finite(aT2) )
		return;

	auto edge = []( Vec2f a, Vec2f b, double x, double y ) {
		return (double(b.x) - a.x) * (y - a.y)
			- (double(b.y) - a.y) * (x - a.x);
	};
	double area = edge( aP0, aP1, aP2.x, aP2.y );
	if( area == 0.0 )
		return;
	if( area < 0.0 )
	{
		std::swap( aP1, aP2 );
		std::swap( aT1, aT2 );
		area = -area;
	}

	//Crop to the canvas area, then convert to pixel coordinates
	double const width = double(aSurface.get_width());
	double const height = double(aSurface.get_height());
	auto const xBegin = Surface::Index(std::clamp(
		std::ceil(double(std::min({ aP0.x, aP1.x, aP2.x })) - 0.5), 0.0, width ));
	auto const yBegin = Surface::Index(std::clamp(
		std::ceil(double(std::min({ aP0.y, aP1.y, aP2.y })) - 0.5), 0.0, height ));
	auto const xEnd = Surface::Index(std::clamp(
		std::floor(double(std::max({ aP0.x, aP1.x, aP2.x })) - 0.5) + 1.0, 0.0, width ));
	auto const yEnd = Surface::Index(std::clamp(
		std::floor(double(std::max({ aP0.y, aP1.y, aP2.y })) - 0.5) + 1.0, 0.0, height ));

	//Shared edges belong to only one triangle, avoiding redundant drawing
	auto inclusive = []( Vec2f a, Vec2f b ) {
		return b.y < a.y || (b.y == a.y && b.x < a.x);
	};
	bool const include0 = inclusive( aP1, aP2 );
	bool const include1 = inclusive( aP2, aP0 );
	bool const include2 = inclusive( aP0, aP1 );
	auto inside = []( double value, bool include ) {
		return value > 0.0 || (value == 0.0 && include);
	};
	for( auto y = yBegin; y < yEnd; ++y )
	{
		for( auto x = xBegin; x < xEnd; ++x )
		{
			double const px = double(x) + 0.5;
			double const py = double(y) + 0.5;
			double const e0 = edge( aP1, aP2, px, py );
			double const e1 = edge( aP2, aP0, px, py );
			double const e2 = edge( aP0, aP1, px, py );
			if( !inside(e0, include0) || !inside(e1, include1) || !inside(e2, include2) )
				continue;

			Vec2f const uv{
				float((e0 * aT0.x + e1 * aT1.x + e2 * aT2.x) / area),
				float((e0 * aT0.y + e1 * aT1.y + e2 * aT2.y) / area)
			};
			ColorF_Alpha const sample = aSampler.sample( uv );
			if( sample.a < 0.5f )
				continue;
			aSurface.set_pixel_srgb( x, y,
				linear_to_srgb(ColorF{ sample.r, sample.g, sample.b }) );
		}
	}
}


// See Lecture 8
bool clip_line( Rect2F const& aTargetArea, Vec2f& aBegin, Vec2f& aEnd )
{
	// Note: If the line is clipped, update aBegin and aEnd correspondingly.
	// They are non-const references (&), meaning we can modify the values
	// passed in through them.
	//
	// Return true to indicate that the line should be drawn and false that
	// the line should be discarded.

	// See https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
	auto b = classify_( aTargetArea, aBegin );
	auto e = classify_( aTargetArea, aEnd );

	while( true )
	{
		if( 0 == b && 0 == e )
			return true;
		
		if( b & e )
			return false;

		auto const next = e > b ? e : b;
		assert( next );

		float const dx = aEnd.x - aBegin.x;
		float const dy = aEnd.y - aBegin.y;

		float x, y;
		if( next & 8 )
		{
			x = aBegin.x + dx * (aTargetArea.ymin + aTargetArea.height -1.f - aBegin.y) / dy;
			y = aTargetArea.ymin + aTargetArea.height -1.f;
		}
		else if( next & 4 )
		{
			x = aBegin.x + dx * (aTargetArea.ymin - aBegin.y) / dy;
			y = aTargetArea.ymin;
		}
		else if( next & 2 )
		{
			x = aTargetArea.xmin + aTargetArea.width -1.f;
			y = aBegin.y + dy * (aTargetArea.xmin + aTargetArea.width -1.f - aBegin.x) / dx;
		}
		else // next & 1
		{
			x = aTargetArea.xmin;
			y = aBegin.y + dy * (aTargetArea.xmin - aBegin.x) / dx;
		}

		if( next == b )
		{
			aBegin.x = x;
			aBegin.y = y;
			b = classify_( aTargetArea, aBegin );
		}
		else
		{
			aEnd.x = x;
			aEnd.y = y;
			e = classify_( aTargetArea, aEnd );
		}
	}

	return true;
}

void draw_line_solid( Surface& aSurface, Vec2f aBegin, Vec2f aEnd, ColorU8_sRGB aColor )
{
	draw_line_solid( aSurface, aSurface.clip_area(), aBegin, aEnd, aColor );
}
void draw_line_solid( Surface& aSurface, Rect2F const& aClipArea, Vec2f aBegin, Vec2f aEnd, ColorU8_sRGB aColor )
{
	if( !clip_line( aClipArea, aBegin, aEnd ) )
		return;

	Vec2f dd = aEnd - aBegin;

	if( std::abs(dd.x) > std::abs(dd.y) )
	{
		float const ystep = dd.y / dd.x;

		if( aBegin.x > aEnd.x ) 
			std::swap( aBegin, aEnd );

		float const end = aEnd.x + 0.5f;
		for( float x = aBegin.x+0.5f, y = aBegin.y+0.5f; x <= end; x += 1.f, y += ystep )
			aSurface.set_pixel_srgb( std::uint32_t(x), std::uint32_t(y), aColor );
	}
	else
	{
		float const xstep = dd.x / dd.y;

		if( aBegin.y > aEnd.y ) 
			std::swap( aBegin, aEnd );

		float const end = aEnd.y + 0.5f;
		for( float x = aBegin.x+0.5f, y = aBegin.y+0.5f; y <= end; y += 1.f, x += xstep )
			aSurface.set_pixel_srgb( std::uint32_t(x), std::uint32_t(y), aColor );
	}
}



void draw_rectangle_solid( Surface& aSurface, Vec2f aMinCorner, Vec2f aMaxCorner, ColorU8_sRGB aColor )
{
	// TODO: (Exercise G.1) your implementation goes here. Can ignore for CW2.
}

void draw_rectangle_outline( Surface& aSurface, Vec2f aMinCorner, Vec2f aMaxCorner, ColorU8_sRGB aColor )
{
	// TODO: (Exercise G.1) your implementation goes here. Can ignore for CW2.
}


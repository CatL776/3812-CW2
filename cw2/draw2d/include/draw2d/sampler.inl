#include <algorithm>
#include <cmath>
#include <draw2d/image.hpp>

inline
Sampler2D::Sampler2D( ImageRGBA const& aImage )
	: mSrcImage( &aImage )
{}

inline
ColorF_Alpha Sampler2D::sample( Vec2f aImageCoord ) const
{
	auto const width = mSrcImage->get_width();
	auto const height = mSrcImage->get_height();
	if( width == 0 || height == 0
		|| !std::isfinite(aImageCoord.x) || !std::isfinite(aImageCoord.y) )
		return { 0.f, 0.f, 0.f, 0.f };

	//Sample at pixel centers, for areas extending beyond the boundaries, use the edge pixels
	double const x = std::clamp( double(aImageCoord.x) - 0.5,
		0.0, double(width - 1) );
	double const y = std::clamp( double(aImageCoord.y) - 0.5,
		0.0, double(height - 1) );
	ImageRGBA::Index const x0 = ImageRGBA::Index(std::floor(x));
	ImageRGBA::Index const y0 = ImageRGBA::Index(std::floor(y));
	ImageRGBA::Index const x1 = std::min( x0 + 1, width - 1 );
	ImageRGBA::Index const y1 = std::min( y0 + 1, height - 1 );
	float const tx = float(x - double(x0));
	float const ty = float(y - double(y0));

	ColorF_Alpha const c00 = linear_from_srgb( mSrcImage->get_pixel(x0, y0) );
	ColorF_Alpha const c10 = linear_from_srgb( mSrcImage->get_pixel(x1, y0) );
	ColorF_Alpha const c01 = linear_from_srgb( mSrcImage->get_pixel(x0, y1) );
	ColorF_Alpha const c11 = linear_from_srgb( mSrcImage->get_pixel(x1, y1) );
	auto interpolate = [tx, ty]( float a, float b, float c, float d ) {
		float const lower = (1.f - tx) * a + tx * b;
		float const upper = (1.f - tx) * c + tx * d;
		return (1.f - ty) * lower + ty * upper;
	};
	return {
		interpolate( c00.r, c10.r, c01.r, c11.r ),
		interpolate( c00.g, c10.g, c01.g, c11.g ),
		interpolate( c00.b, c10.b, c01.b, c11.b ),
		interpolate( c00.a, c10.a, c01.a, c11.a )
	};
}

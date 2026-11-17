#include <draw2d/image.hpp>

#include <memory>
#include <cmath>
#include <cstdint>
#include <algorithm>

#include <cstdio>
#include <cstring>
#include <cassert>

#include <stb_image.h>

#include <draw2d/surface.hpp>
#include <support/error.hpp>

namespace
{
	struct STBImageRGBA_ : public ImageRGBA
	{
		STBImageRGBA_( Index, Index, std::uint8_t* );
		virtual ~STBImageRGBA_();
	};
}

ImageRGBA::ImageRGBA()
	: mWidth( 0 )
	, mHeight( 0 )
	, mData( nullptr )
{}

ImageRGBA::~ImageRGBA() = default;


std::unique_ptr<ImageRGBA> load_image( char const* aPath )
{
	assert( aPath );

	stbi_set_flip_vertically_on_load( true );

	int w, h, channels;
	stbi_uc* ptr = stbi_load( aPath, &w, &h, &channels, 4 );
	if( !ptr )
		throw Error( "Unable to load image \"{}\"", aPath );

	return std::make_unique<STBImageRGBA_>(
		ImageRGBA::Index(w),
		ImageRGBA::Index(h),
		ptr
	);
}

// Task 1.4
void blit_masked( Surface& aSurface, ImageRGBA const& aImage, Vec2f aPosition )
{
    using Coord = std::int64_t;
    Coord const width = Coord(aImage.get_width());
    Coord const height = Coord(aImage.get_height());

    //Convert the image center to its bottom-left corner
    Coord const x0 = std::lround(aPosition.x - 0.5f * float(width));
    Coord const y0 = std::lround(aPosition.y - 0.5f * float(height));

    //Clip once, so large images only visit visible pixels
    Coord const sxBegin = std::max(Coord(0), -x0);
    Coord const syBegin = std::max(Coord(0), -y0);
    Coord const sxEnd = std::min(width, Coord(aSurface.get_width()) - x0);
    Coord const syEnd = std::min(height, Coord(aSurface.get_height()) - y0);
    if (sxBegin >= sxEnd || syBegin >= syEnd)
        return;

    for (Coord sy = syBegin; sy < syEnd; ++sy)
    {
        for (Coord sx = sxBegin; sx < sxEnd; ++sx)
        {
            auto const color = aImage.get_pixel(
                ImageRGBA::Index(sx), ImageRGBA::Index(sy));
            //Keep the background when alpha is below 128
            if (color.a < 128)
                continue;

            aSurface.set_pixel_srgb(
                Surface::Index(x0 + sx), Surface::Index(y0 + sy),
                ColorU8_sRGB{color.r, color.g, color.b});
        }
    }
}

// Task 2.1
void blit_masked_subregion( Surface& aSurface, ImageRGBA const& aImage, Vec2i aSourceOrigin, Vec2i aSourceSize, Vec2f aPosition )
{
	if( aSourceSize.x <= 0 || aSourceSize.y <= 0
		|| !std::isfinite(aPosition.x) || !std::isfinite(aPosition.y) )
		return;

	using Coord = std::int64_t;
	Coord const sourceX = aSourceOrigin.x;
	Coord const sourceY = aSourceOrigin.y;
	Coord const width = aSourceSize.x;
	Coord const height = aSourceSize.y;

	double const left = std::round( double(aPosition.x) - 0.5 * double(width) );
	double const bottom = std::round( double(aPosition.y) - 0.5 * double(height) );
	if( left >= double(aSurface.get_width()) || bottom >= double(aSurface.get_height())
		|| left + double(width) <= 0.0 || bottom + double(height) <= 0.0 )
		return;

	Coord const x0 = Coord(left);
	Coord const y0 = Coord(bottom);

	//Crop both the source image and the canvas, processing only the valid pixels
	Coord const xBegin = std::max({ Coord(0), -sourceX, -x0 });
	Coord const yBegin = std::max({ Coord(0), -sourceY, -y0 });
	Coord const xEnd = std::min({ width,
		Coord(aImage.get_width()) - sourceX,
		Coord(aSurface.get_width()) - x0 });
	Coord const yEnd = std::min({ height,
		Coord(aImage.get_height()) - sourceY,
		Coord(aSurface.get_height()) - y0 });

	for( Coord y = yBegin; y < yEnd; ++y )
	{
		for( Coord x = xBegin; x < xEnd; ++x )
		{
			ColorU8_sRGB_Alpha const color = aImage.get_pixel(
				ImageRGBA::Index(sourceX + x), ImageRGBA::Index(sourceY + y) );
			if( color.a < 128 )
				continue;

			aSurface.set_pixel_srgb(
				Surface::Index(x0 + x), Surface::Index(y0 + y),
				ColorU8_sRGB{ color.r, color.g, color.b } );
		}
	}
}

// Task 2.2
void blit_blend_subregion( Surface& aSurface, ImageRGBA const& aImage, Vec2i aSourceOrigin, Vec2i aSourceSize, Vec2f aPosition )
{
	if( aSourceSize.x <= 0 || aSourceSize.y <= 0
		|| !std::isfinite(aPosition.x) || !std::isfinite(aPosition.y) )
		return;

	using Coord = std::int64_t;
	Coord const sourceX = aSourceOrigin.x;
	Coord const sourceY = aSourceOrigin.y;
	Coord const width = aSourceSize.x;
	Coord const height = aSourceSize.y;

	double const left = std::round( double(aPosition.x) - 0.5 * double(width) );
	double const bottom = std::round( double(aPosition.y) - 0.5 * double(height) );
	if( left >= double(aSurface.get_width()) || bottom >= double(aSurface.get_height())
		|| left + double(width) <= 0.0 || bottom + double(height) <= 0.0 )
		return;

	Coord const x0 = Coord(left);
	Coord const y0 = Coord(bottom);
	Coord const xBegin = std::max({ Coord(0), -sourceX, -x0 });
	Coord const yBegin = std::max({ Coord(0), -sourceY, -y0 });
	Coord const xEnd = std::min({ width,
		Coord(aImage.get_width()) - sourceX,
		Coord(aSurface.get_width()) - x0 });
	Coord const yEnd = std::min({ height,
		Coord(aImage.get_height()) - sourceY,
		Coord(aSurface.get_height()) - y0 });

	for( Coord y = yBegin; y < yEnd; ++y )
	{
		for( Coord x = xBegin; x < xEnd; ++x )
		{
			ColorU8_sRGB_Alpha const source = aImage.get_pixel(
				ImageRGBA::Index(sourceX + x), ImageRGBA::Index(sourceY + y) );
			if( source.a == 0 )
				continue;

			Surface::Index const dx = Surface::Index(x0 + x);
			Surface::Index const dy = Surface::Index(y0 + y);
			if( source.a == 255 )
			{
				aSurface.set_pixel_srgb( dx, dy,
					ColorU8_sRGB{ source.r, source.g, source.b } );
				continue;
			}

			//When blending in linear RGB space, the alpha value is calculated by direct proportional scaling
			ColorF_Alpha const foreground = linear_from_srgb( source );
			ColorF const background = linear_from_srgb( aSurface.get_pixel_srgb(dx, dy) );
			float const alpha = foreground.a;
			float const remaining = 1.f - alpha;
			ColorF const blended{
				alpha * foreground.r + remaining * background.r,
				alpha * foreground.g + remaining * background.g,
				alpha * foreground.b + remaining * background.b
			};
			aSurface.set_pixel_srgb( dx, dy, linear_to_srgb(blended) );
		}
	}
}


namespace
{
	STBImageRGBA_::STBImageRGBA_( Index aWidth, Index aHeight, std::uint8_t* aPtr )
	{
		mWidth = aWidth;
		mHeight = aHeight;
		mData = aPtr;
	}

	STBImageRGBA_::~STBImageRGBA_()
	{
		if( mData )
			stbi_image_free( mData );
	}
}

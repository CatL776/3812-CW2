#include "spaceship.hpp"

#include <numbers>

#include <cmath>
#include <cassert>

#include <draw2d/draw.hpp>
#include <draw2d/image.hpp>
#include <draw2d/sampler.hpp>
#include <draw2d/surface.hpp>

#include <vmlib/transform2d.hpp>

// Line-based space ship
void SpaceShipLines::draw( Surface& aSurface, float aAngleRadians, Vec2f aTranslation ) const
{
	auto rot = make_rotation_2d( aAngleRadians );
	lines.draw( aSurface, color, rot, aTranslation );
}

SpaceShipLines make_spaceship_lines()
{
	static constexpr float xs[] = { 250.f, 200.f, 150.f, 100.f, 000.f, 040.f, -50.f, -140.f, -170.f };
	static constexpr float ys[] = { 190.f, 180.f, 70.f, 50.f, 30.f, 20.f };

	LineStrip spaceship{ { 
		{ 0.2f * xs[0], 0.2f * +ys[5] }, // upper half. starts at front, goes towards the back
		{ 0.2f * xs[1], 0.2f * +ys[3] },
		{ 0.2f * xs[2], 0.2f * +ys[3] },
		{ 0.2f * xs[3], 0.2f * +ys[4] }, 
		{ 0.2f * xs[4], 0.2f * +ys[4] },
		{ 0.2f * xs[4], 0.2f * +ys[2] },
		{ 0.2f * xs[5], 0.2f * +ys[1] },
		{ 0.2f * xs[6], 0.2f * +ys[0] },
		{ 0.2f * xs[8], 0.2f * +ys[2] },
		{ 0.2f * xs[7], 0.2f * +ys[3] },

		{ 0.2f * xs[7], 0.2f * -ys[3] }, // lower half, starts at the back and goes towards the front
		{ 0.2f * xs[8], 0.2f * -ys[2] }, // this is essentially the same as the upper half, except in reverse.
		{ 0.2f * xs[6], 0.2f * -ys[0] },
		{ 0.2f * xs[5], 0.2f * -ys[1] },
		{ 0.2f * xs[4], 0.2f * -ys[2] },
		{ 0.2f * xs[4], 0.2f * -ys[4] },
		{ 0.2f * xs[3], 0.2f * -ys[4] }, 
		{ 0.2f * xs[2], 0.2f * -ys[3] },
		{ 0.2f * xs[1], 0.2f * -ys[3] },
		{ 0.2f * xs[0], 0.2f * -ys[5] }, 

		{ 0.2f * xs[0], 0.2f * +ys[5] } // link back to beginning (connects both sides at the "front")
	} };

	return SpaceShipLines{ std::move(spaceship), { 0.2f, 0.4f, 0.7f } };
}

// Image atlas space ship
void SpaceShipAtlas::draw( Surface& aSurface, float aAngleRadians, Vec2f aTranslation ) const
{
	assert( atlas );
	assert( atlasWidth > 0 && atlasSize > 0 );
	if( !atlas || atlasWidth <= 0 || atlasSize <= 0
		|| !std::isfinite(aAngleRadians) )
		return;

	int const rows = (atlasSize - 1) / atlasWidth + 1;
	auto const imageWidth = atlas->get_width();
	auto const imageHeight = atlas->get_height();
	assert( imageWidth % atlasWidth == 0 && imageHeight % rows == 0 );
	int const frameWidth = int(imageWidth / atlasWidth);
	int const frameHeight = int(imageHeight / rows);
	if( frameWidth <= 0 || frameHeight <= 0
		|| imageWidth % atlasWidth != 0 || imageHeight % rows != 0 )
		return;

	double constexpr twoPi = 2.0 * std::numbers::pi_v<double>;
	double angle = std::fmod( double(aAngleRadians), twoPi );
	if( angle < 0.0 )
		angle += twoPi;
	int const frame = int(std::floor(angle * double(atlasSize) / twoPi + 0.5)) % atlasSize;
	int const column = frame % atlasWidth;
	int const rowFromTop = frame / atlasWidth;

	//The image is flipped vertically upon loading, so the row index for the atlas must be calculated in reverse
	Vec2i const sourceOrigin{
		column * frameWidth,
		(rows - 1 - rowFromTop) * frameHeight
	};
	blit_masked_subregion( aSurface, *atlas, sourceOrigin,
		Vec2i{ frameWidth, frameHeight }, aTranslation );
}

SpaceShipAtlas make_spaceship_atlas()
{
	SpaceShipAtlas ship;
	ship.atlas = load_image( SpaceShipAtlas::kSourceImage );
	ship.atlasSize = 64;
	ship.atlasWidth = 8;
	return ship;
}


// Sampler-based space ship
void SpaceShipSampler::draw( Surface& aSurface, float aAngleRadians, Vec2f aTranslation ) const
{
	assert( texture );
	if( !texture || !std::isfinite(aAngleRadians) )
		return;
	float const width = float(texture->get_width());
	float const height = float(texture->get_height());
	auto const rotation = make_rotation_2d( aAngleRadians );
	Vec2f const p0 = rotation * Vec2f{ -0.5f * width, -0.5f * height } + aTranslation;
	Vec2f const p1 = rotation * Vec2f{ +0.5f * width, -0.5f * height } + aTranslation;
	Vec2f const p2 = rotation * Vec2f{ +0.5f * width, +0.5f * height } + aTranslation;
	Vec2f const p3 = rotation * Vec2f{ -0.5f * width, +0.5f * height } + aTranslation;
	Sampler2D const sampler( *texture );
	draw_triangle_tex_masked( aSurface, p0, p1, p2, sampler,
		{ 0.f, 0.f }, { width, 0.f }, { width, height } );
	draw_triangle_tex_masked( aSurface, p0, p2, p3, sampler,
		{ 0.f, 0.f }, { width, height }, { 0.f, height } );
}

SpaceShipSampler make_spaceship_sampler()
{
	SpaceShipSampler ship;
	ship.texture = load_image( SpaceShipSampler::kSourceImage );
	return ship;
}


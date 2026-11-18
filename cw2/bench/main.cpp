#include <nanobench.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include <draw2d/draw.hpp>
#include <draw2d/image.hpp>
#include <draw2d/sampler.hpp>
#include <draw2d/surface.hpp>

#if defined(NDEBUG)
namespace
{
	constexpr Surface::Index kFramebufferSize = 1024;
	constexpr Vec2f kPosition{ 512.f, 512.f };
	constexpr ColorU8_sRGB kBackground{ 24, 40, 64 };

	void draw_quad( Surface& surface, Sampler2D const& sampler, int size )
	{
		float const side = float(size);
		float const half = 0.5f * side;
		Vec2f const p0{ kPosition.x - half, kPosition.y - half };
		Vec2f const p1{ kPosition.x + half, kPosition.y - half };
		Vec2f const p2{ kPosition.x + half, kPosition.y + half };
		Vec2f const p3{ kPosition.x - half, kPosition.y + half };
		draw_triangle_tex_masked( surface, p0, p1, p2, sampler,
			{ 0.f, 0.f }, { side, 0.f }, { side, side } );
		draw_triangle_tex_masked( surface, p0, p2, p3, sampler,
			{ 0.f, 0.f }, { side, side }, { 0.f, side } );
	}

	void reset_region( Surface& surface, int size )
	{
		auto const begin = Surface::Index(kPosition.x - 0.5f * float(size));
		auto const end = begin + Surface::Index(size);
		for( auto y = begin; y < end; ++y )
			for( auto x = begin; x < end; ++x )
				surface.set_pixel_srgb( x, y, kBackground );
	}

	//Before starting the timer, check whether the pixel dimensions are consistent across the three drawing styles
	void verify_binary( ImageRGBA const& image, Sampler2D const& sampler, int size )
	{
		Surface masked( kFramebufferSize, kFramebufferSize );
		Surface blended( kFramebufferSize, kFramebufferSize );
		Surface textured( kFramebufferSize, kFramebufferSize );
		masked.fill( kBackground );
		blended.fill( kBackground );
		textured.fill( kBackground );
		blit_masked_subregion( masked, image, { 0, 0 }, { size, size }, kPosition );
		blit_blend_subregion( blended, image, { 0, 0 }, { size, size }, kPosition );
		draw_quad( textured, sampler, size );
		for( Surface::Index y = 0; y < kFramebufferSize; ++y )
		{
			for( Surface::Index x = 0; x < kFramebufferSize; ++x )
			{
				auto const a = masked.get_pixel_srgb( x, y );
				auto const b = blended.get_pixel_srgb( x, y );
				auto const c = textured.get_pixel_srgb( x, y );
				auto close = []( int p, int q ) { return std::abs(p - q) <= 1; };
				if( !close(a.r,b.r) || !close(a.g,b.g) || !close(a.b,b.b)
					|| !close(a.r,c.r) || !close(a.g,c.g) || !close(a.b,c.b) )
					throw std::runtime_error( "Binary-alpha output mismatch: check rasterization, sampler, and earlier tasks" );
			}
		}
	}
}
#endif

int main( int argc, char* argv[] ) try
{
#if !defined(NDEBUG)
	(void)argc;
	(void)argv;
	std::cerr << "Build and run the RELEASE benchmark (NDEBUG required).\n";
	return 1;
#else
	std::string const outputPath = argc > 1 ? argv[1] : "benchmark_results.csv";
	std::ofstream csv( outputPath );
	if( !csv )
		throw std::runtime_error( "Cannot create benchmark CSV" );
	csv << "profile,size,method,framebuffer,measured_ns,reset_baseline_ns,estimated_draw_ns,median_relative_error\n";
	csv << std::setprecision(12);

	for( std::string const& profile : { std::string("binary"), std::string("smooth") } )
	{
		for( int const size : std::array{ 32, 64, 128, 256, 512 } )
		{
			std::string const path = "assets/bench/sprite_" + profile + "_" + std::to_string(size) + ".png";
			auto const image = load_image( path.c_str() );
			if( image->get_width() != ImageRGBA::Index(size)
				|| image->get_height() != ImageRGBA::Index(size) )
				throw std::runtime_error( "Unexpected benchmark image dimensions" );
			Sampler2D const sampler( *image );
			if( profile == "binary" )
				verify_binary( *image, sampler, size );

			Surface surface( kFramebufferSize, kFramebufferSize );
			surface.fill( kBackground );
			ankerl::nanobench::Bench bench;
			bench.title( profile + " " + std::to_string(size) + "x" + std::to_string(size) )
				.unit("image").epochs(15).warmup(3)
				.minEpochTime(std::chrono::milliseconds(10))
				.performanceCounters(false);

			bool const resetEachIteration = profile == "smooth";
			double resetSeconds = 0.0;
			if( resetEachIteration )
			{
				bench.run( "reset_only", [&] {
					reset_region( surface, size );
					ankerl::nanobench::doNotOptimizeAway( surface.get_surface_ptr() );
				} );
				resetSeconds = bench.results().back().median(ankerl::nanobench::Result::Measure::elapsed);
			}

			for( int method = 0; method < 3; ++method )
			{
				char const* const name = method == 0 ? "masked" : method == 1 ? "blended" : "textured_quad";
				surface.fill( kBackground );
				bench.run( name, [&] {
					//Restore the background before drawing each semi-transparent element
					if( resetEachIteration )
						reset_region( surface, size );
					if( method == 0 )
						blit_masked_subregion( surface, *image, { 0, 0 }, { size, size }, kPosition );
					else if( method == 1 )
						blit_blend_subregion( surface, *image, { 0, 0 }, { size, size }, kPosition );
					else
						draw_quad( surface, sampler, size );
					ankerl::nanobench::doNotOptimizeAway( surface.get_surface_ptr() );
				} );
				auto const& result = bench.results().back();
				double const seconds = result.median(ankerl::nanobench::Result::Measure::elapsed);
				csv << profile << ',' << size << ',' << name << ",1024x1024,"
					<< seconds * 1e9 << ',' << resetSeconds * 1e9 << ','
					<< (seconds - resetSeconds) * 1e9 << ','
					<< result.medianAbsolutePercentError(ankerl::nanobench::Result::Measure::elapsed) << '\n';
			}
		}
	}
	csv.close();
	if( !csv )
		throw std::runtime_error( "Error writing benchmark CSV" );
	std::cout << "Saved " << outputPath << '\n';
	return 0;
#endif
}
catch( std::exception const& error )
{
	std::cerr << error.what() << '\n';
	return 1;
}

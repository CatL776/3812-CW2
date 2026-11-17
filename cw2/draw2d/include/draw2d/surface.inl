/* Inline functions need to have a definition that is visible to the compiler
 * whenever the function is used. They could be defined in the header that
 * declares the function. However, to keep the definitions and declarations
 * somewhat apart, it is a common practice to move them to an "inline" file
 * such as this one (*.inl extension). This file is then #include:ed at the end
 * of the header, to ensure that whoever includes the header also automatically
 * includes the inline file.
 *
 * Inlining allows us to avoid any overheads related to call (when building in
 * "release mode" / with optimizations enabled). This makes functions like
 * set_pixel_srgb() zero overhead abstractions.
 *
 */
inline
void Surface::set_pixel_srgb( Index aX, Index aY, ColorU8_sRGB const& aColor )
{
	assert( aX < mWidth && aY < mHeight ); // IMPORTANT! This line must remain the first line in this function!

	//Each pixel occupies four bytes, with red, green, and blue values ​​written in sequence
	Index const index = get_linear_index( aX, aY ) * 4;
	mSurface[index + 0] = aColor.r;
	mSurface[index + 1] = aColor.g;
	mSurface[index + 2] = aColor.b;
	mSurface[index + 3] = 0;
}

inline 
auto Surface::get_width() const noexcept -> Index
{
	return mWidth;
}
inline
auto Surface::get_height() const noexcept -> Index
{
	return mHeight;
}

inline
Rect2F Surface::clip_area() const noexcept
{
	return Rect2F{ 0.f, 0.f, float(mWidth), float(mHeight) };
}

inline
auto Surface::get_linear_index( Index aX, Index aY ) const noexcept -> Index
{
	//First locate this row, then add the horizontal coordinate
	return aY * mWidth + aX;
}


inline
ColorU8_sRGB Surface::get_pixel_srgb( Index aX, Index aY ) const noexcept
{
	assert( aX < mWidth && aY < mHeight ); // IMPORTANT! This line must remain the first line in this function!

	Index const index = get_linear_index( aX, aY ) * 4;
	return { mSurface[index + 0], mSurface[index + 1], mSurface[index + 2] };
}

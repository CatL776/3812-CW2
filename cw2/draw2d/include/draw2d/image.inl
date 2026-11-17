inline
ColorU8_sRGB_Alpha ImageRGBA::get_pixel( Index aX, Index aY ) const
{
	assert( aX < mWidth && aY < mHeight ); // Leave this at the top of the function.

	//Read out red, green, blue, and transparency values ​​at once
	Index const index = get_linear_index( aX, aY ) * 4;
	return ColorU8_sRGB_Alpha{
		mData[index + 0],
		mData[index + 1],
		mData[index + 2],
		mData[index + 3]
	};
}

inline
auto ImageRGBA::get_width() const noexcept -> Index
{
	return mWidth;
}
inline
auto ImageRGBA::get_height() const noexcept -> Index
{
	return mHeight;
}

inline
std::uint8_t* ImageRGBA::get_image_ptr() noexcept
{
	return mData;
}
inline
std::uint8_t const* ImageRGBA::get_image_ptr() const noexcept
{
	return mData;
}

inline
ImageRGBA::Index ImageRGBA::get_linear_index( Index aX, Index aY ) const noexcept
{
	//Locate this pixel by its row
	return aY * mWidth + aX;
}

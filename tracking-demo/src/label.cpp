/* Optimized label code */
#include "marker.h"
#include "emmintrin.h"
namespace marker {

void Scanner::ccLabels(cv::Mat& _src, cv::Mat& _dst) {
    int i, j, j_scalar = 0;
    uchar tab[256];
    Size roi = _src.size();
    roi.width *= _src.channels();
    size_t src_step = _src.step;
    size_t dst_step = _dst.step;

    if( _src.isContinuous() && _dst.isContinuous() )
    {
        roi.width *= roi.height;
        roi.height = 1;
        src_step = dst_step = roi.width;
    }
    int thresh = 12;
    int maxval = 1;
    __m128i _x80 = _mm_set1_epi8('\x80');
    __m128i thresh_u = _mm_set1_epi8(thresh);
    __m128i thresh_s = _mm_set1_epi8(thresh ^ 0x80);
    __m128i maxval_ = _mm_set1_epi8(maxval);
    j_scalar = roi.width & -8;

    for( i = 0; i < roi.height; i++ )
    {
        const uchar* src = _src.ptr() + src_step*i;
        uchar* dst = _dst.ptr() + dst_step*i;

		for( j = 0; j <= roi.width - 32; j += 32 )
		{
			__m128i v0, v1;
			v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
			v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
			v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
			v1 = _mm_cmpgt_epi8( _mm_xor_si128(v1, _x80), thresh_s );
			v0 = _mm_and_si128( v0, maxval_ );
			v1 = _mm_and_si128( v1, maxval_ );
			_mm_storeu_si128( (__m128i*)(dst + j), v0 );
			_mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
		}

		for( ; j <= roi.width - 8; j += 8 )
		{
			__m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
			v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
			v0 = _mm_and_si128( v0, maxval_ );
			_mm_storel_epi64( (__m128i*)(dst + j), v0 );
		}
    }
}

}




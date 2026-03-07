#ifndef BSO_HELPERS_PLATFORM_HPP
#define BSO_HELPERS_PLATFORM_HPP

#if defined(_WIN32) || defined(_WIN64)
	#define BSO_IS_WINDOWS 1
#else
	#define BSO_IS_WINDOWS 0
#endif

#endif // BSO_HELPERS_PLATFORM_HPP
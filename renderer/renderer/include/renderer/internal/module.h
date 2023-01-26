#if !defined(RENDERER_API)
	#define RENDERER_API /* NOTHING */

	#if defined(WIN32) || defined(WIN64)
		#undef RENDERER_API
		#if defined(renderer_EXPORTS)
			#define RENDERER_API __declspec(dllexport)
		#else
			#define RENDERER_API __declspec(dllimport)
		#endif
	#endif // defined(WIN32) || defined(WIN64)

#endif // !defined(RENDERER_API)


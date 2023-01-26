#if !defined(LOADERS_API)
	#define LOADERS_API /* NOTHING */

	#if defined(WIN32) || defined(WIN64)
		#undef LOADERS_API
		#if defined(loaders_EXPORTS)
			#define LOADERS_API __declspec(dllexport)
		#else
			#define LOADERS_API __declspec(dllimport)
		#endif
	#endif // defined(WIN32) || defined(WIN64)

#endif // !defined(LOADERS_API)


#if !defined(SERIALIZER_API)
	#define SERIALIZER_API /* NOTHING */

	#if defined(WIN32) || defined(WIN64)
		#undef SERIALIZER_API
		#if defined(serializer_EXPORTS)
			#define SERIALIZER_API __declspec(dllexport)
		#else
			#define SERIALIZER_API __declspec(dllimport)
		#endif
	#endif // defined(WIN32) || defined(WIN64)

#endif // !defined(SERIALIZER_API)
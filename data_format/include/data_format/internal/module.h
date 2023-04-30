#if !defined(DATA_FORMAT_API)
	#define DATA_FORMAT_API /* NOTHING */

	#if defined(WIN32) || defined(WIN64)
		#undef DATA_FORMAT_API
		#if defined(data_format_EXPORTS)
			#define DATA_FORMAT_API __declspec(dllexport)
		#else
			#define DATA_FORMAT_API __declspec(dllimport)
		#endif
	#endif // defined(WIN32) || defined(WIN64)

#endif // !defined(DATA_FORMAT_API)
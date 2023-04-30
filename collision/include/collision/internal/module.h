#if !defined(COLLISION_API)
	#define COLLISION_API /* NOTHING */

	#if defined(WIN32) || defined(WIN64)
		#undef COLLISION_API
		#if defined(collision_EXPORTS)
			#define COLLISION_API __declspec(dllexport)
		#else
			#define COLLISION_API __declspec(dllimport)
		#endif
	#endif // defined(WIN32) || defined(WIN64)

#endif // !defined(COLLISION_API)
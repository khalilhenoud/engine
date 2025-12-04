Headers organization
====================
Headers should follow this specific grouping, all in alphabetical order. Add
empty lines between sections if readability requires it:
1. System specific headers, ie: *windows.h*, *timeapi.h* etc...
2. Standard library (clib or std) headers, ie: *stdio.h*, *stdlib.h* etc...
3. Current package headers in the following order:
    1. internal headers: *internal/module.h*, etc...
    2. any other header that belongs to the current package.
4. Any other headers, sorted alphabetically by package.

__Notes__:
* Leave 2 lines between preprocessor definitions (including headers) and
any other code piece.
* Directory depth plays no role in the inclusion order, only alphanumerical
ordering does, see 'Example 1' below.
* Grouping, in cases such as system specific includes, the rules are segragated
per group. See 'Example 2'.

Examples
--------
Example 1:

    #include <math/c/matrix3f.h>
    #include <math/vector3f.h>

Example 2:

    #include <renderer/internal/module.h>
    #if defined(WIN32) || defined(WIN64)
    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <renderer/platform/win32/opengl_parameters.h>
    #else
    // TODO: Implement static assert for C using negative indices array.
    #endif
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

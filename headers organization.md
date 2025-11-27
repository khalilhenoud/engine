Headers organization scheme
===========================
Headers should follow this specific grouping, all in alphabetical order:
1. System specific headers, ie: *windows.h*, *timeapi.h* etc...
2. Standard library (clib or std) headers, ie: *stdio.h*, *stdlib.h* etc...
3. Current package headers. If you are working in mesh package, mesh headers
come first.
4. Any other headers, sorted alphabetically by package.
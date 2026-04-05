Project organization
====================
HAS_STANDALONE_PREDECESSOR
simple vs complex project structure
structure of the cmakelist.txt file
no project can depend on anything other than math and library, except
  > game: the engine is required to include the dlls for now, until we have a
  working build system
  > assets: any package that depend on the assets package is by necessity a
  hierarchical system (most assets depend on texture asset for example).

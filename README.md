# engine

A simple engine to facilitates further development and experimentation.

## Description

This project is intended as a base for future work/study endeavors. It is
comprised of different modules that can be developed separately and later
integrated with the main project. 

## Getting Started

### Dependencies

* libpng, zlib libraries are used to enable png file loading, the source code
has been included as is in the loaders repo.

* This has been compiled strictly on Windows 11, I did not take the time to test
this on any other platform.

### Executing program

Before starting create a 'media' folder and add your ase files there. Then modify
app.bat to point to the parent folder (basically where the 'media' folder is
located).

```
cmake -B build
cmake --build build
app
```
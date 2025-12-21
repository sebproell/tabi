The obligatory "Hello World" example.

1. bootstrap tabi in this folder

```
mkdir build
cd build
cc -o bootstrap ../build.tabi.c && ./bootstrap ../ . && rm bootstrap
```

This compiles tabi itself, executes it to generate the build.ninja file from build.tabi, and removes the bootstrap binary again. 
The build.ninja file is now present in the build/ folder and set up to regenerate itself when build.tabi.c changes.

2. now use ninja normally

```
ninja 
```

# SDL3 base

## Setup

```
git clone --recursive git@github.com:minchopaskal/sdl3base.git sdl3newrepo

cd sdl3newrepo

rm -rf .git

git init
```

Rename `PROJECT_NAME` in `CMakeFiles.txt`, and the `PROJECT_NAME` define in `src/defines.h`. Optionally, rename `PROJECT_NAME_CAPS` in `CMakeFiles.txt`.

```
cmake -G Ninja -B build -S .
cmake --build build

cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

and finally

```
cd ../
mv sdl3newrepo <actualname>
```
## Run

```
./build/prj res/config.cfg
```

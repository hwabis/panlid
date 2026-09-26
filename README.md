# panlid

C++ transcription of [osu!framework 2026.901.0](https://github.com/ppy/osu-framework/tree/afc88e863a38564c4539774aadad932f3bb8c844).

## Building

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build
cd build
ctest
# todo visual tests
```

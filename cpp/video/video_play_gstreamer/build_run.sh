rm -rf build/
mkdir build && cd build
cmake ..
make -j4
./player /home/moyuping/work/tmp/output_fixed.mp4
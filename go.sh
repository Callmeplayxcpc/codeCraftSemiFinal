#! /bin/bash
## cmake complie
rm -r build
mkdir build
cd build
cmake ..
make
#back to main directory
cd ..

# run test case
# python3 run.py ./interactor ./data/sample.in ./code_craft
python3 run.py ./interactor ./data/sample_practice.in ./code_craft
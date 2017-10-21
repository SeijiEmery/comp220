Assignment 08 (work in progress)

Build instructions:
    cd <some-temp-dir>
    git clone https://github.com/SeijiEmery/comp220
    cd comp220/assignment_08/
    mkdir build; cd build
    unzip ../../data/dvc-schedule.txt.zip
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make dvc_test; time ./dvc_test

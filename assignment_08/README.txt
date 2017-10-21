Assignment 08 (work in progress)

Build instructions:
    cd <some-temp-dir>
    git clone https://github.com/SeijiEmery/comp220
    cd comp220/assignment_08
    unzip ../../assignment_04/data/dvc-schedule.zip
    mkdir build; cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make dvc_test; time ./dvc_test

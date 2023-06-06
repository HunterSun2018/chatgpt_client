rm build -rf
cmake . -Bbuild -GNinja
cd build && ninja
# ChatGPT Client
A ChatGPT client implemented by c++ 20 coroutine and Beast of Boost

## Requirements
1. sudo apt install cmake ninja-build libgtest-dev libreadline-dev libfmt-dev
2. Download boost 1.8.1 to folder $HOME 

## Build
cmake . -Bbuild -GNinja && cd build  
ninja

## Run
exprot CHATGPT_KEY=YOUR_API_KEY  
./chat

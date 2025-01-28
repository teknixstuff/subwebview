How to build this thing:
1. open cmd in this folder
2. md build
3. cd build
4. cmake -G "Visual Studio 17" -A x64 ..
5. open build\cef.sln in Visual Studio 2022 and select Build > Build Solution.
6. copy build\release\* into xpi\plugins\
7. zip the contents of the xpi folder
8. change the extension of your zip to .xpi

windows only
set /p ndkpath=<ndkpath.txt
%ndkpath%\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android-addr2line.exe -e .\obj\local\arm64-v8a\%1 %2
@setlocal

@if "%1"=="" (set config=Debug) else (set config=%1)
@if "%~2"=="" (set cmake_args=) else (set cmake_args=-- /logger:%2)

@set PreferredToolArchitecture=x64
@set BOOST_ROOT=%CD%\IPC\packages\boost.1.71.0.0\lib\native\include
@set BOOST_LIBRARYDIR=%CD%\IPC\packages\boost.1.71.0.0
@set BOND_GBC_PATH=%CD%\IPC\packages\Bond.Compiler.8.2.0\tools

@mkdir bond\build\target\%config%
@pushd bond\build

@cmake -G "Visual Studio 16 2019" -A x64 -DBOND_LIBRARIES_ONLY=ON -DBOND_ENABLE_GRPC=FALSE -DCMAKE_INSTALL_PREFIX=%CD%\target\%config% ..
@cmake --build . --config %config% --target %cmake_args%
@cmake --build . --config %config% --target INSTALL %cmake_args%

@popd
@endlocal

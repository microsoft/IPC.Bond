@setlocal

@set PreferredToolArchitecture=x64
@set BOOST_ROOT=%CD%\IPC\packages\boost.1.63.0.0\lib\native\include
@set BOOST_LIBRARYDIR=%CD%\IPC\packages\boost.1.63.0.0
@set BOND_GBC_PATH=%CD%\IPC\packages\Bond.Compiler.6.0.0\tools

@mkdir bond\build
@mkdir bond\build\target

@pushd bond\build

@cmake -G "Visual Studio 14 2015 Win64" -DBOND_LIBRARIES_ONLY=ON -DBOND_ENABLE_COMM=FALSE -DBOND_ENABLE_GRPC=FALSE -DCMAKE_INSTALL_PREFIX=%CD%\target ..
@cmake --build . --target
@cmake --build . --target INSTALL

@popd
@endlocal

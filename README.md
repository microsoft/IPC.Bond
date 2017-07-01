# IPC.Bond

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/Microsoft/IPC/blob/master/LICENSE)

IPC.Bond is an extension of [IPC](https://github.com/Microsoft/IPC) library that provides inter-process communication using shared memory on Windows with [Bond](https://github.com/Microsoft/bond) serialization.<br/>

# Build

The library is developed and maintained with [Visual Studio 2015](https://msdn.microsoft.com/en-us/library/dd831853.aspx).
In order to build the library you will need to do the following:
  1. Restore NuGet packages for [IPC.Bond.sln](https://github.com/Microsoft/IPC.Bond/blob/master/IPC.Bond.sln).
  2. Build the [Bond](https://github.com/Microsoft/bond) (only core C++) submodule using helper [bond.cmd](https://github.com/Microsoft/IPC.Bond/blob/master/bond.cmd) script.
  3. Build the [IPC](https://github.com/Microsoft/IPC) submodule using [IPC.sln](https://github.com/Microsoft/IPC/blob/master/IPC.sln).
  4. Build the [IPC.Bond.sln](https://github.com/Microsoft/IPC.Bond/blob/master/IPC.Bond.sln).

# Getting Started

Start with [C++](https://github.com/Microsoft/IPC.Bond/blob/master/UnitTests/TransportTests.cpp) and [C#](https://github.com/Microsoft/IPC.Bond/blob/master/UnitTestsManaged/TransportTests.cs) tests.

# Contributing

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# License

Copyright (c) Microsoft Corporation. All rights reserved.

Licensed under the [MIT](https://github.com/Microsoft/IPC.Bond/blob/master/LICENSE) License.

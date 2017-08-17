Mr. Plotter Application
=======================
This is an adaptation of the Multi-Resolution Plotter to a desktop application. It is used to view data in BTrDB.

To build the application, you will need to install the gRPC C++ runtime. This has been tested with gRPC version 1.4.2. Follow the instructions at https://github.com/grpc/grpc to build an install.

You will also need to generate the gRPC and protobuf code for the BTrDB C++ bindings. Execute `make proto` inside the `btrdb/btrdb-cpp` directory.

Then, build the application using Qt Creator. The build and application have been tested with Qt 5.9.1.

License
-------
This application is licensed under the GNU General Public License version 3.

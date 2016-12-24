# FTP
Partial Implementation of RFC959

Build:
  Please use `cmake` to compile this project. Refer to https://cmake.org/cmake/help/v3.7/manual/cmake.1.html
  
Usage:
- Client
    ``` bash
    ./Client ip_address
    ```
    type `?` to view all supported commands.
- Server
    ``` bash
    ./Server
    ```
    Server will listen on port `5021` and print log message to stdout and stderr.

# 3310-lab2
## Simple single-client single-server remote backup system
* Works under Linux Env, not working under Windows CMD (missing header files).
* only upload, remove, ls, on/off function is implemented.
* only one client is supported simultaneously.
* for different server constraints, buffer size for TCP connection should be modified.
  * For my own server on Tencent Cloud, buffer size = 3000 will fail, so I implement the 1000-size buffer.
---
Currently, something wrong with the `htons()` translation.

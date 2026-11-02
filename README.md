# Mosaic

My own little attempt at a vulkan library. 
Current version does not support fully featured applications yet, but it is still in development.

## Building 

This is built using make.

Calling `make` in project root should be enough to build.

Dependencies:
- Vulkan
- (GLFW)[https://www.glfw.org/]

Dependencies are generally found using `pkg-config`, so make sure they can be found through that. Usually a `make install` should be good enough when self compiling.

# Portfolio
Thank you for visiting my GitHub  portfolio.   

This is portfolio containing some of my personal projects with a focus on C++ and computer graphics.

I can be contacted at RonanQuill96@gmail.com to discuss any opportunities that you have available.  

# Portfolio Breakdown



### How to build a project
1) Clone this repository
2) Each folder contains a ".sln" file which can be opened in Microsoft Visual Studio 2019.
3) Compile with C++17 enabled.

### Final Year Project
A real time 3D rendering engine created as part of undergraduate degree, using C++ and the Vulkan graphics API.
This folder contains the code, an executable version of the project and my thesis document for the project. Due to GitHub file size limits I had to remove the asset folder required to run the project. If you would like to run the project please contact me and I will provide this asset folder.

### SIMD Math Library
A basic SIMD math library which contains implementations of a 4D Vector, 4x4 Matrix and associated methods.
This library also contains a basic test suite to ensure correct functionality.


### Thread Pool 
A lightweight header only C++ thread pool to allow easy use of thread pools in a project.


### Software Based Ray Tracer 
A software based ray tracer created with C++ which renders individual frames of a provided scene. This is a basic ray tracer developed to familiarise myself with the techniques used when created ray tracing software.
The ray tracer is multithreaded through the use of my C++ thread pool library and also makes use of the previously mentioned SIMD library to improve performance.
##### Examples of rendered images.
###### Example 1: Dimensions: 600 x 600. Samples Per Pixel: 10,000
![Cornell Box](CornellBox.png)

###### Example 2: Dimensions: 1920 x 1080. Samples Per Pixel: 100
![Random Scene](RandomScene.png)


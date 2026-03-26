# BVH Visualizer

An interactive BVH (Bounding Volume Hierarchy) visualizer applying the Surface Area Heuristic (SAH) or Midpoint methodologies to accelerate ray-triangle intersections. Used to display traversal test volumes and candidate bounding boxes. 

## Requirements
Arch Linux standard dependencies:
```bash
sudo pacman -S cmake gcc glfw glm mesa glu
```

## Build Instructions
1. Clone or CD into the repository
2. Run CMake and build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
3. Execute the visualizer
```bash
./bvh_visualizer
```

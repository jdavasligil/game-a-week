# Week 1 (Strategy Game)
Date: 3/23/25
Theme: Geometry

## Ideas
- Non-euclidean
- Tower Defense
- Fractal Geometry
- Aliens?

## Design
Florbles: Alien Invasion is a spherical planetary tower defense game.

Defend the pentagon against the wacky dancing aliens!


## Log
### Day 1
Found a fantastic resource on [icospheres](https://www.songho.ca/opengl/gl_sphere.html). I modified the code to simply generate an icosphere with 8 subdivisions and serialized the data to binary. Now, it should be possible to quickly deserialize at runtime. Once I have the data imported, I need to figure out how to render the sphere with SDL3's new graphics API. This is the biggest hurdle. Afterwards, I can customize the UV texture and start working on the actual game which shouldn't be too bad.

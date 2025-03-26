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

### Day 2
Did some research on gpu programming. The big challenge is pushing 3D operations onto the GPU with the SDL3 GPU API. I now have the model deserialized and stored into a struct. Now, how to draw the thing with UV mapped textures? We have arrays for the indices, vertices, normals, and uvs. We have a texture. The challenge is performing the proper transformations and drawing with the GPU. This might take up the rest of the week!

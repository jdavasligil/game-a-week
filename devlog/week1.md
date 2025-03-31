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

### Day 3
Followed an [ODIN tutorial](https://www.youtube.com/watch?v=tfc3vschDVw) on getting a basic triangle rendered. Also followed some [SDL3 GPU examples](https://github.com/TheSpydog/SDL_gpu_examples) for reference. Not as bad as expected, but still pretty tricky. Burnt some midnight oil on this one, especially fiddling with shader compiling and cmake. Anyways, we are on track to rendering the world! Next time, we will get model / view / perspective transformations working and push the sphere and world texture to the gpu.

### Day 4
Got the triangle working! Otherwise, not much else.

### Day 5
Starting to understand glsl. Got cglm working with model / view / projection transformations. Created a 'Transform' model for handling homogenous coordinate affine transformations, and got quaternion interpolation working!

### Day 6
Tried to get the sphere rendering on the GPU, but got bogged down learning glsl and SDL3 GPU intrinsics. Got stuck for a couple hours because I forgot to submit the command buffer. Had a party to attend, so only got the buffer transfer setup.

### Day 7
Final Push! We got textures working on the GPU with uv mapping. One step away from rendering the world of Florbles... and it's time to wrap it up!

### Retrospective
This project was a real eye opener. GPU programming is no joke, and getting even a simple model on screen has been a real challenge. I knew going for 3D on the first week would be extremely difficult and unlikely to produce a playable game, but it was worthwhile nonetheless.

I've learned an incredible amount of stuff in just one week: compiling shaders, pushing data from disk to cpu to gpu, rotational interpolation with quaternions, etc. Even though the game was not ultimately finished, this is a project I can look back to for reference in the future. Maybe I will come back and finish this up one day. Until then, on to the next one!

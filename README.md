Version 1 (3/23/26):

When tested with the following system:

HP Victus 16 R0037CL
Max fans
Hybrid Graphics
Intel I7-13700HX (with Intel UHD 770 Graphics)
NVIDIA GeForce RTX 4060 Laptop GPU (120 watt power limit, connected through a PCIe 4.0x8 interface)
32GB (2x16gb) ddr5-4800 CL40 SODIMM
HP Smart 230W AC adapter PLUGGED IN

And following render test:

2,000,000 triangles
Approximately 11664 pixels per triangle
No textures, just colors
All opaque
Tested at FHD 16:9 1080p (1920x1080)

On the integrated graphics (Intel UHD 770):

The performance is around 39-40 FPS with the occlusion culling.
The performance slows to a crawl without occlusion culling and is around 10-11 FPS.

On the dedicated graphics (NVIDIA GeForce RTX 4060 Laptop GPU, 120 watt power limit, PCIe 4.0x8 interface):

The performance with occlusion culling is around 400-5000 FPS.
The performance is around 275-400 FPS without occlusion culling.

On other computer (core ultra 7 255hx, intel arc xe-lpg 64eu integrated graphics, ddr5-5600 2x16gb):

2,000,000 triangles
Approximately 11664 pixels per triangle
No textures, just colors
All opaque
Tested at FHD 16:9 1080p (1920x1080)

22 fps without occlusion culling
70 fps with occlusion culling

The process I used:

Instead of using 4 buffers of previously culled images, temporal data or bounding box checking, I instead made 2 sets of vertices: One with the normal positions, colors and values of each triangle, and the other with the normal positions, but having the colors act as the triangle's ID. Then I rendered the final framebuffer at 1:4 resolution, 270p (480x270), and whichever triangles didn't appear on the 1:4 mip would be culled.

Version 1.1 (3/24/26):

BUG FIXES:

- Fixed small triangles being incorrectly culled

Added a cull buffer. I noticed small triangles were not being rendered because they didn't appear in the 270p mip (because they were subpixel after being scaled down) leading to them being culled, when they should have been fully visible in the final 1080p image output. I added a cull buffer with a CPU loop to tell the vertex shader to not cull subpixel triangles in the mip.

So, this is a "large triangle" occlusion culler (8x8px triangles or bigger), if further tests show stability with smaller triangles, I will push the update. For small triangles, a different occlusion culler is needed (I might add later).

Version 1.2 (3/27/2026):

Removed excess libraries and left stack allocation up to Windows.

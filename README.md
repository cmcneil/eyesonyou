# eyesonyou

# Running

```bash
# From project root
> python -m SimpleHTTPServer
```

Then, visit http://localhost:8000/web.

## On Coordinates:
This project uses a lot of coordinate transform. Standards:
each virtual camera is at 0.5 meters from the Three.js coordinate system it is in,
respectively. The eyeball display client expects to be told where an object is, in its
own coordinate system. The central vision server needs to keep track of the transformations between (real) camera
coordinate systems, and the various Three.js coordinate systems of the display clients.

If a display client is displaying only one eyeball, that eyeball will be at (0, 0, 0) in the 
central coordinate system. The central vision server's primary global coordinate system is
relative to CAMERA 0, the primary (real) camera.

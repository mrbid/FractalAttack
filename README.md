# FractalAttack
A little game where you can crash into the planet or the asteroids.

#### Snapcraft
https://snapcraft.io/fractalattack

#### WebGL
- https://mrbid.github.io/eris2/
- https://mrbid.github.io/eris/
- https://mrbid.github.io/erisrot/

#### [OLD] Fractal Comet Versions
- https://github.com/mrbid/FractalAttack/tree/FractalComets2
- https://github.com/mrbid/FractalAttack/tree/FractalComets

#### YouTube
- https://www.youtube.com/watch?v=pbpDG7VXTsw
- https://www.youtube.com/watch?v=eJ0qqsFmLEA

#### Misc
If this version does not run smoothly you will have to try the [Initial Release here](https://github.com/mrbid/FractalAttack/tree/InitialRelease) which has no comets. There is also a web version of the initial release [here](https://mrbid.github.io/eris/).

I have unlocked Y axis rotations to allow full 360+ and sample the Y axis in the view matrix to detect an inversion and flip the controls once an inversion is detected, although obviously gimbal lock is apparent when Y and Z align so I lock rotations in the X axis *(around Z)* between a threshold to avoid that clunky and tactally confusing transition. Also Y axis transversals are now relative to the view direction and not the global axis.

### Attributions
https://www.solarsystemscope.com/textures/
- https://www.solarsystemscope.com/textures/download/4k_eris_fictional.jpg
- https://www.solarsystemscope.com/textures/download/8k_venus_surface.jpg

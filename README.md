# FractalAttack
A little game where you must stop asteroids destroying a planet.

At the moment you can crash into the planet or the asteroids.

**Snapcraft;** https://snapcraft.io/fractalattack<br>
**WebGL;** https://mrbid.github.io/eris2/
**[OLD] Fractal Comets Version;** https://github.com/mrbid/FractalAttack/tree/FractalComets

If this version does not run smoothly you will have to try the [Initial Release here](https://github.com/mrbid/FractalAttack/tree/InitialRelease) which has no comets. There is also a web version of the initial release [here](https://mrbid.github.io/eris/).

I have unlocked Y axis rotations to allow full 360+ and sample the Y axis in the view matrix to detect an inversion and flip the controls once an inversion is detected, although obviously gimbal lock is apparent when Y and Z align so I lock rotations in the X axis *(around Z)* between a threshold to avoid that clunky and tactally confusing transition. Also Y axis transversals are now relative to the view direction and not the global axis.

### Attributions
https://www.solarsystemscope.com/textures/

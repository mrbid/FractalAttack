# FractalAttack
A little game where you must stop fractal asteroids destroying a planet.

I'm not going to finish this, this project is abandoned, you can crash into the planet or just watch the Menger Sponge comets hit it.

If this version does not run smoothly you will have to try the [Initial Release here](https://github.com/mrbid/FractalAttack/tree/InitialRelease) which has no comets. There is also a web version of the initial release [here](https://mrbid.github.io/eris/).

By default I lock Y axis rotations to 180 degrees, to avoid Y axis inversion. Although in [main_360_euler.c](main_360_euler.c) I have unlocked Y axis rotations to allow full 360+ and sample the Y axis in the view matrix to detect an inversion and flip the controls once an inversion is detected, although obviously gimbal lock is apparent when Y and Z align so I lock rotations in the X axis *(around Z)* between a threshold to avoid that clunky and tactally confusing transition. Also Y axis transversals are now relative to the view direction and not the global axis.

### Attributions
https://www.solarsystemscope.com/textures/

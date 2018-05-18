# CarND-Controls-PID
Self-Driving Car Engineer Nanodegree Program

---

## Project Goal

The goal of the project is to model a self driving using a Proportional-Integral-Derivative (PID) controller. The model is tested on a simulated car in the Udacity simulator. The following are the expectations of the project:

- The car should stay within the lane and not veer too far off the center.
- Fine tune the parameters to stabilize the controller for steering (optionally a controller for throttle)

### Reflections

A demo of the project can be seen [here](https://www.youtube.com/watch?v=rmgkuRVAnF4).

<div align="center">
  <a href="https://www.youtube.com/watch?v=rmgkuRVAnF4"><img src="https://img.youtube.com/vi/rmgkuRVAnF4/0.jpg" alt="IMAGE ALT TEXT"></a>
</div> 

### Parameters

The PID is modeled as follows:
$$
\alpha = -\tau_p CTE -\tau_i \sum CTE - \tau_d \frac{d}{dt}CTE
$$


Here, $\alpha$ is the steering angle. CTE is the cross track error received from the simulator. $\tau_p$, $\tau_i$ and $\tau_d$ are the parameters for each of the PID terms of the controller. I use the twiddle algorithm to find the best set of parameters for the project. It can be found in the code [here](src/main.cpp#L257). It should be noted that I ran twiddle a couple times, each time with initializing it with the parameters I got from the previous runs. The final set of parameters I have used are $\tau_{p,i,d} = \{0.15, 0, 3.31\}$.

#### Effects of parameters

The following graph shows the effect of each P and D parameters when they are set to 0(while keeping other parameters constant) vs that of a finely tuned PID. As can be seen, for the properly tuned controller, the CTE remains fairly constant, around 0. When $\tau_p$ is set 0, the CTE doesn't converge to 0. On the other hand, when $\tau_d$ is set to 0, the CTE oscillates. This is due to the linear effect of a non-zero $\tau_p$ on the steering angle.

![Plot](plot.png)

## Dependencies

* cmake >= 3.5
 * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1(mac, linux), 3.81(Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools]((https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)
* [uWebSockets](https://github.com/uWebSockets/uWebSockets)
  * Run either `./install-mac.sh` or `./install-ubuntu.sh`.
  * If you install from source, checkout to commit `e94b6e1`, i.e.
    ```
    git clone https://github.com/uWebSockets/uWebSockets 
    cd uWebSockets
    git checkout e94b6e1
    ```
    Some function signatures have changed in v0.14.x. See [this PR](https://github.com/udacity/CarND-MPC-Project/pull/3) for more details.
* Simulator. You can download these from the [project intro page](https://github.com/udacity/self-driving-car-sim/releases) in the classroom.

There's an experimental patch for windows in this [PR](https://github.com/udacity/CarND-PID-Control-Project/pull/3)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./pid`. 

Tips for setting up your environment can be found [here](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/0949fca6-b379-42af-a919-ee50aa304e6a/lessons/f758c44c-5e40-4e01-93b5-1a82aa4e044f/concepts/23d376c7-0195-4276-bdf0-e02f1f3c665d)

## Editor Settings

We've purposefully kept editor configuration files out of this repo in order to
keep it as simple and environment agnostic as possible. However, we recommend
using the following settings:

* indent using spaces
* set tab width to 2 spaces (keeps the matrices in source code aligned)

## Code Style

Please (do your best to) stick to [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html).

## Project Instructions and Rubric

Note: regardless of the changes you make, your project must be buildable using
cmake and make!

More information is only accessible by people who are already enrolled in Term 2
of CarND. If you are enrolled, see [the project page](https://classroom.udacity.com/nanodegrees/nd013/parts/40f38239-66b6-46ec-ae68-03afd8a601c8/modules/f1820894-8322-4bb3-81aa-b26b3c6dcbaf/lessons/e8235395-22dd-4b87-88e0-d108c5e5bbf4/concepts/6a4d8d42-6a04-4aa6-b284-1697c0fd6562)
for instructions and the project rubric.

## Hints!

* You don't have to follow this directory structure, but if you do, your work
  will span all of the .cpp files here. Keep an eye out for TODOs.

## Call for IDE Profiles Pull Requests

Help your fellow students!

We decided to create Makefiles with cmake to keep this project as platform
agnostic as possible. Similarly, we omitted IDE profiles in order to we ensure
that students don't feel pressured to use one IDE or another.

However! I'd love to help people get up and running with their IDEs of choice.
If you've created a profile for an IDE that you think other students would
appreciate, we'd love to have you add the requisite profile files and
instructions to ide_profiles/. For example if you wanted to add a VS Code
profile, you'd add:

* /ide_profiles/vscode/.vscode
* /ide_profiles/vscode/README.md

The README should explain what the profile does, how to take advantage of it,
and how to install it.

Frankly, I've never been involved in a project with multiple IDE profiles
before. I believe the best way to handle this would be to keep them out of the
repo root to avoid clutter. My expectation is that most profiles will include
instructions to copy files to a new location to get picked up by the IDE, but
that's just a guess.

One last note here: regardless of the IDE used, every submitted project must
still be compilable with cmake and make./

## How to write a README
A well written README file can enhance your project and portfolio.  Develop your abilities to create professional README files by completing [this free course](https://www.udacity.com/course/writing-readmes--ud777).


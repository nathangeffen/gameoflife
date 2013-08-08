# Conway Game of Life (sequential and threaded)

This program is written in C and has been tested on GNU/Linux using the GCC and clang compilers. It's flaky and obviously comes with no warranty. It was written in a hurry to learn parallelism so that I could teach it to a computer science class. It uses pthreads and since it was my first threaded program, it has awful style. But it works, more or less.

To use it you have to compile it. You have two options:

- If you want to run a GUI version, you need to install the gtk3 development libraries. (I use Synaptic under Mint to install libgtk-3-dev and libgtk-3-0.) To compile it, run *make release*. Then at the command prompt, run *./gameoflife*.

- If you are happy to have a plain text interface (a bit more robust), run *make release_no_gui*. Then at the command prompt, run *./gameoflife*.

It's quite easy to break the program, especially the GUI interface. The game executes on a 10 million cell grid, but obviously only a small fraction of this is visible. There's no out-of-bounds checking, so presumably if a non-stable initial setup is allowed to run for long enough (probably hours on an i3 or i5), it'll eventually core dump.

The main point of the program is to demo threads. The main code for the single-thread version is iterate_generations(). The main code for the multi-threaded version is iterate_generations_t().

In the GUI version, if you set the initial pattern to Gosper and the step size to 3000, then click step, running the code with one thread will take about twice the time as running it with four threads on a quadcore machine. On my old quadcore i3, if I load Gosper and set the number of iterations to 3,000, then click *Step* it takes 14 seconds with one thread and eight seconds with 4.

The license is contained in the *LICENSE* file.

Enjoy!

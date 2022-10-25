<strong>Welcome to Iancu's Mandelbrot Project!</strong>

# What is the Mandelbrot set?
In its most formal, Wikipedia definition:
<blockquote>The Mandelbrot set is the set of complex numbers $c$ for which the function $f_c(z)=z^2+c$ does not diverge to infinity when iterated from $z=0$, i.e., for which the sequence $f_c(0)$, $f_c(f_c(0))$, etc., remains bounded in absolute value.</blockquote>
In less formal terms, the Mandelbrot Set is the set of all the complex numbers that hold a certain property. A complex number consists of a real and imaginary part, which can be represented on the XY axis, that is why in most representations, you can see a central black bulb (corresponding to the complex numbers inside the set). The points on the XY axis which hold this property are usually assigned the color black, on a white background (white - points that do not belong to the set). The property held by the points in the set is that we can repeatedly apply the function $f_c(z) = z^2+c$ to the complex number $c$, with $z=0$ initially so that $z$ remains bounded, that it doesn't diverge. To color the Mandelbrot with something other than black and white, we can check *when* $z$ exits the orbit, and assign colors according to the iteration count (the number of times we applied the function $f$).

I am not going to go into more depth of what the Mandelbrot set is, since there are tons of resources online with probably better explanations, but I recommend the [Numberphile](https://www.youtube.com/c/numberphile) YouTube videos on the Mandelbrot set to give intuition and an overall idea of what is going on.

# Motivation
In the pursuit of creating software that utilizes the different computer's components efficiently, I came across the Mandelbrot Set, which also stunned me by its mathematical beauty. There were already many programs on the Internet which rendered fractals, but I wanted to create one of my own, so I could explore the different approaches to coding the set, and also plot the fractal in unique ways. I've written two C++ programs, one relying on the CPU (central processing unit) for doing the hard work, and the other on the GPU (graphics processing unit). I've compared the two variants and documented the methods I've used. Moreover, I've made it easy for other users who may want to tinker with these programs by providing a config file which determines the way in which the set is drawn to the screen (the GPU version is recommended due to performance concerns). The current features of the program are:
- Exploring the Mandelbrot Set in a "Google Maps" manner (zoom/pan)
- Setting custom gradients used for coloring the set
- Rendering at a higher resolution (supersampling)
- Performance benchmark (automatic zoom)
- Taking screenshots of the current region and saving it to the local directory
- Altering other parameters related to rendering the set in the config file

# Coding the set
**Note**: this project (and documentation) has been written in C++, though the code could be replicated in other programming languages.

The general algorithm is pretty straight-forward, but the optimization are, in my opinion, the real challenge. Let's have a look at the following pseudocode:

```c++
int x = 0, y = 0;
int iteration = 0;
while (x * x + y * y <= 4 && iteration < max_iteration) {
  int x_aux = x * x - y * y + x0;
  y = 2 * x * y + y0;
  x = x_aux;
}
colorPixel(iteration);
```

As you have probably seen already, we are denoting each complex number in the coordinate system c(x0, y0) a pixel on the screen and assign a color to that pixel according to the number of iterations it takes it to either diverge or reach the *max_iteration* upper bound (since we cannot continue iterating forever). As we are treating every pixel individually, we might be able to implement some optimizations, such as *parallel computing*, to give a little insight on what we are about to discuss. Moreover, we can reduce the number of multiplications we make (as they are more time-consuming than additions) and also skip some points because we know for sure are in the set, or because their iteration count can be deduced by others around it (a technique known as *border tracing*).

## CPU oriented
### Multithreading
The most general optimization we can implement is multithreading. Since computing the number of iterations of each point is independent from the others, we can spread the processing among the multiple threads of our CPU. An initial idea of how we are going to go about this is simply to take the whole screen and divide it into, let's say, 4 equal horizontal tiles (assuming our CPU has 4 threads). Now we can assign each portion of the screen to each thread in our CPU. However, depending on the region of the Mandelbrot that we are rendering, some threads might be able to finish their job faster and then simply do nothing, waiting for the others to complete their tasks, thus impacting performance. In this case, each frame would finish rendering only when the last tile is done.

However, a workaround fortunately exists. We are going to divide the screen into more than 4 tiles, for instance, 16 of them, and create a ***thread pool***. What this does is it assigns each tile to a thread and, whenever a thread finishes its job and becomes available, it automatically reassigns it the next unprocessed tile. Thus, we were able to noticeably increase performance! Certainly, performance may vary depending on the scenario, but still, it is a major improvement. You might be asking yourself how we are going to achieve this. Thankfully, the library we are going to use, OpenMP, has already taken care of this, so we wouldn't have to worry about changing our code too much. Assuming we built a function ***drawTile(tileIndex)*** that takes the number of the tile and draws it to the screen, we would proceed in the following way:

```c++
#pragma omp parallel for
for (int i = 0; i < tile_count; i++)
  drawTile(i);
```

The highlighted pragma directive essentially tells the compiler to split the *for* loop up between multiple threads.

### SIMD
We are able to further parallelize our algorithm by making use of SIMD (Single instruction, multiple data). This technology allows us to perform the same operation (for instance: addition, subtraction, multiplication) on more than one variable at the same time. For instance, this could be useful when adding up two arrays simultaneously, as it would be much faster than adding up each of the arrays' elements individually. 

Modern CPU's often offer support for AVX (Advanced Vector Extensions) which are additional SIMD instructions sets. At the time this article has been written, the most widespread versions of AVX are AVX2 and AVX512 among both Intel and AMD CPU's. My own CPU (Intel Core i5-6600K) only supports AVX2, so I wrote the program with that in mind, though the code could be easily modified to take advantage of the more recent and improved AVX512 technology.

AVX2 provides a 256-bit long registers which can be filled with multiple types of data (for example: 8x32bit integers or 4*64bit doubles). Since we require higher precision for zooming deeper into the set, we are going to store the complex numbers (the coordinates) using doubles. In order to utilize AVX2 in C++ we must include the intrinsics library "immintrin.h", which provides wrappers for Assembly-like functions. 

**Note:** These functions are rather hard to understand, but I will try my best to describe the general idea behind the code. I am going to post the code here, but I strongly advise you to read the explanations below and search these functions on https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html and try to grasp what they actually do.

**Note:** This implementation is the optimized version of the original algorithm. Refer to this page for more information: https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set#Optimized_escape_time_algorithms

```c++
_zr = _mm256_setzero_pd();
_zi = _mm256_setzero_pd();
_cr = _mm256_set_pd(cr[0], cr[1], cr[2], cr[3]);
_ci = _mm256_set_pd(ci[0], ci[1], ci[2], ci[3]);
_max_iterations = _mm256_set1_epi64x(max_it);
_iteration = _mm256_set_epi64x(0, sz >= 2 ? 0 : 1e9, sz >= 3 ? 0 : 1e9, sz == 4 ? 0 : 1e9); _two = _mm256_set1_pd(2.0);
_four = _mm256_set1_pd(4.0);

repeat:

  _zr2 = _mm256_mul_pd(_zr, _zr);
  _zi2 = _mm256_mul_pd(_zi, _zi);
  _a = _mm256_sub_pd(_zr2, _zi2);
  _a = _mm256_add_pd(_a, _cr);
  _b = _mm256_mul_pd(_zr, _zi);
  _b = _mm256_fmadd_pd(_b, _two, _ci);

  _zr = _a;
  _zi = _b;

  _a = _mm256_add_pd(_zr2, _zi2);

  _mask1 = _mm256_cmp_pd(_a, _four, _CMP_LT_OQ);
  _mask2 = _mm256_cmpgt_epi64(_max_iterations, _iteration);
  _mask2 = _mm256_and_si256(_mm256_castpd_si256(_mask1), _mask2);
  _mask2 = _mm256_and_si256(_mm256_set1_epi64x(1LL), _mask2);

  _iteration = _mm256_add_epi64(_mask2, _iteration);

  if (_mm256_testz_si256(_mask2, _mask2) != 1) goto repeat;
```

First, we are going to create registers <code>_cr</code> and <code>_ci</code> each comprised of 4 doubles. These represent the real and imaginary part of 4 points that we are going to process (essentially determine escape time of) at the same time. Also, we are going to need <code>_zr</code> and <code>_zi</code> initially filled with zeros, for computing the next iteration. Another important aspect of using intrinsics is that we also have to create registers for constant values, for instance <code>_two</code> and <code>_four</code> which are crucial for establishing our escape radius. The other registers' meaning can be easily deduced: <code>_zr2</code> and <code>_zi2</code> are the squared counterparts of the aforementioned <code>_zr</code> and <code>_zi</code> <code>_max_iterations</code> is the maximum number of iterations and <code>_iterations</code> is the current iteration count.

Since we are processing four points at the same time, we want to stop either when all of them have escaped the radius or when the number of maximum iterations has been reached. Since checking each element in the register would defeat the whole purpose of using them in the first place and decrease performance, we are going to create some "mask" registers which are in fact **flags** indicating whether any of the points is done processing. To be more specific, when a point escapes the radius the sequence of bits in the <code>_mask1</code> register corresponding to that point (one of the four points) is set to 0. This also happens to <code>_mask2</code> when the maximum number of iterations is reached. 

**Recap**: As of now, if any 64bit sequence in the <code>_mask1</code> register is set to 0 (try to think as the 256 register as broken into four 64bit buckets, denoting the four points), it means that the point corresponding to that sequence (either the first, second, third, or forth) has escaped the radius. This also applies to <code>_mask2</code> which is set to zero if the maximum number of iterations has been exceeded. We now perform a bitwise AND on <code>_mask1</code> and <code>_mask2</code> and store the result in <code>_mask2</code>. We can now assert that if every bit in <code>_mask2</code> is set to zero we have to stop iterating and break out of the loop.

**Note:** Because we are using intrinsics the traditional "while" loop has been replaced by an <code>if</code>/<code>goto</code> combo.

### Border tracing
This optimization won't take advantage of the newer technologies present in modern CPU's, but rather of a mathematical property of the Mandelbrot set. This property states that the Mandelbrot set is connected, meaning that if we are able to trace the border of a closed shape with the same color on its contour, we can simply fill it in after with that color. This means that all the pixels which lie inside that shape are basically "skipped", thus pruning our computation. Also, recall that we color each point (corresponding to a pixel on the screen) according to the number of iterations it took it to escape (or <code>max_iterations</code> if it doesn't).

Tracing these borders is done by running an algorithm similar to BFS (breadth-first search). The method implies maintaining 2 queues:
;scan_queue
: This queue contains the "scanned" pixels, which have already been processed (their iteration count has been calculated)
;calc_queue
: This queue contains the "to-be" processed pixels, of which we know nothing about. These pixels are pushed to the <code>scan_queue</code> afterwards. The reason this queue is required is because the pixels are processed in a somewhat "offline" manner (recall that we compute the iteration count of 4 pixels at a time using SIMD).

**Note:** Flushing the <code>calc_queue</code> means processing every pixel in it (by using buffers of up to 4 pixels). At the end of this operation, the <code>calc_queue</code> is empty. 

Initially, all the pixels on the edges of the screen (think of this as the "border" of the rectangular display) are added to the <code>scan_queue</code> which is then flushed to the <code>calc_queue</code> and the iteration count of each of these pixels (points) is computed. The algorithm runs until the <code>scan_queue</code> becomes empty. At each iteration, the front of the <code>scan_queue</code> is removed and all of its "neighbors" (N/W/S/E pixels) are added to the <code>calc_queue</code>, which is then flushed. Now we know the iteration count of all the neighboring pixels and we further add them to the <code>scan_queue</code> if their iteration count **differs** from the iteration count of the current pixel (the front of the <code>scan_queue</code>). The reason behind this decision is that we want to only process as few pixels as possible, by only tracing the border, which, by its definition, is the area where the color changes (we do not want to process pixels inside shapes). Consider the border between a black and white area - at first, for example, only a white pixel on the border will be in the queue, then it will be removed and a black pixel will appear in the queue, which will push another white pixel to the queue and so on and so forth. This "back and forth" operation ensures that only the pixels on the border are being processed.

After the algorithm is done, we are left with some untouched pixels. We can now simply iterate over the array of pixels (or the matrix) and, for every unprocessed pixel, assign the color of the pixel just before it, as simply as:
```c++
pixel_color[i] = pixel_color[i - 1];
```
considering we mapped the 2d array of pixels to a 1d array in [row-major order](https://en.wikipedia.org/wiki/Row-_and_column-major_order).

This technique is similar to the "paint bucket tool" operation in MSPaint. 

For implementation details, look for the <code>[BorderTracer](https://github.com/iancupopp/mandel/blob/main/main.cpp#L121)</code> class in the source code.  


**Note:** I've chosen to use the <code>std::deque</code> instead of <code>std::queue</code> over performance concerns.

## GPU oriented
Previously, we have seen that parallelism significantly improves performance, so a natural question comes to mind: what if we were able to execute **more** operations at the same time? It turns out that the GPU (graphics processing unit) is specialized in performing rather simple operations in parallel, which comes in handy for our implementation of the Mandelbrot set. In order to run code on the GPU and not on the CPU I used the OpenGL API, through a couple of C++ libraries (GLFW + GLAD). In fact, the actual C++ code (mainly boilerplate code for creating a window, etc.) was executed on the CPU, but the GLAD library made it possible to write a **shader** (a little program that lies on the GPU) in the *OpenGL shading language* (GLSL) which is executed on the **GPU**. In this documentation I will include the GLSL code responsible for plotting the Mandelbrot set. For more information about OpenGL, I strongly recommend the [LearnOpenGL](https://learnopengl.com/). website, which made this project possible.

### Brief overview of the shader code
The project includes two types of shaders: the **vertex** and **fragment** shader. The **vertex** shader is responsible for setting up the area of the screen that is being rendered to (achieved by drawing **two** triangles composing a rectangle covering the whole screen). The **fragment** shader takes as input every pixel and outputs its color, which, in this case, is based on the iteration count of the point corresponding to that pixel's XY coordinates in the screen space.
.
For a practical introduction to shaders, visit [LearnOpenGL-Shaders](https://learnopengl.com/Getting-started/Shaders).

The vertex shader code is pretty much boilerplate (standard), as it simply sets up the drawing area (a full-screen quad). Thus, it is going to need some *vertex data* which comprises of the vertices of the 2 triangles. This part is covered in the previous link.

### Fragment shader
However, the fragment shader is the one the allows us to draw the Mandelbrot set and not just a blank screen. The code might look a little odd at first, so please read the comments and the explanations below.

```c++
#version 330 core

out vec4 FragColor;

uniform vec2 lowerLeft;
uniform vec2 upperRight;
uniform vec2 viewportDimensions;
uniform int maxIterations;
uniform vec4 palette[64];
uniform float paletteOffset;
uniform int numColors;
uniform bool applySmooth;

vec4 getColor(float x) {
  int i = 0;
  while (x > palette[i + 1].x)
    ++i;
  return vec4(mix(palette[i].yzw, palette[i + 1].yzw, (x - palette[i].x) / (palette[i + 1].x - palette[i].x)), 1);
}

void main() {
  FragColor = vec4(0, 0, 0, 0);
  vec2 c = mix(lowerLeft, upperRight, gl_FragCoord.xy / viewportDimensions.xy);
  float x0, y0, x, y, x2, y2;
  x = y = x2 = y2 = 0;
  x0 = c.x, y0 = c.y;

  //cardioid checking - check if point is inside the biggest 2 bulbs (which are in the set)
  float q = (x0 - 0.25) * (x0 - 0.25) + y0 * y0;
  if (q * (q + (x0 - 0.25)) <= 0.25 * y0 * y0) {
    FragColor += vec4(0, 0, 0, 1);
    return;
  }

  //this is an optimized algorithm for computing iterations,
  //minimizing the number of multiplications made,
  //sacrificing clarity for speed

  //this slightly modified version of the original algorithm
  //uses a larger bailout radius, namely 2^8
  float iteration = 0;
  while (x2 + y2 <= (1 << 16) && iteration < maxIterations) {
    y = (x + x) * y + y0;
    x = x2 - y2 + x0;
    x2 = x * x;
    y2 = y * y;
    ++iteration;
  }
  if (iteration < maxIterations) {
    //the coloring function used here is explained over here https://www.math.univ-toulouse.fr/~cheritat/wiki-draw/index.php/Mandelbrot_set,
    //though the following implementation (inspired by Wikipedia) differs a bit from the link above, while the
    //mentioned link provides intuition for the formula
    if (applySmooth)
      iteration -= (log(log(x2 + y2) / 2.0) - log(log(exp2(8)))) / log(2.0);
    FragColor = getColor(fract((iteration + paletteOffset) / numColors));
  } else {
    FragColor = vec4(0, 0, 0, 1);
  }
}
```

The fragment shader takes as input the *uniforms* (which are shared variables between the CPU/GPU) and is called for every pixel (*fragment*) producing a color for each of them. Note that there are quite a few changes compared to the CPU version, particularly:
* Cardioid checking - a simple check of whether the current point lies within the main cardioid or not (i.e. the central circular shape of point that we know for a fact belong to the set), thus skipping many useless iterations.
* A new color function
* Smooth coloring algorithm

#### Color function
You may have noticed that the array <code>palette</code> is being fed into the fragment shader. This array contains a maximum of 64 gradient points in a custom format. The first value <code>x</code> of the <code>vec4</code> represents the "position" of the gradient point in the [0,1] range. Then, the last three values (<code>y</code>, <code>z</code>, <code>w</code>) correspond to the color of the point in the RGB format. Credits: https://cssgradient.io/ (note that the positions here are expressed in percentages).


**Note:** These gradient points are provided in the config file (see **Configuration**).


### Supersampling
To obtain a higher quality image, we are able to render the Mandelbrot at a higher resolution (i.e. 4x the width/height) and then downscale the image to the window's resolution. We are going to achieve by rendering the high-res image to and off-screen **framebuffer** (<code>FBO</code>) and

## Benchmark

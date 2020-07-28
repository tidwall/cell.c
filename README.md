# `cell`

A library for C that encodes and decodes multidimensional z-ordered cells. A cell is an integer value that interleaves the coordinates of a point, which is useful for range and spatial based operations.

## Using

There are three data types `cell_xy`, `cell_xyz`, and `cell_xyzm` which represents two, three, and four dimensions respectively.

The input for the `encode` function and output for the `decode` function are floating points within the range `[0.0,1.0)`.

The 2 dimensional `cell_xy_encode` function results in a uint64.
The 3/4 dimensional `cell_xyz_encode` and `cell_xym_encode` functions result in a 128-bit integer that is represented by a struct with `hi` and `lo` uint64s.

## Examples

Encode/Decode a 2D point.

```c
cell_xy cell = cell_xy_encode(0.331, 0.587);
printf("%" PRIu64 "\n", cell);

double x, y;
cell_xy_decode(cell, &x, &y0);
printf("%f %f\n", x, y);

// output:
// 7148508595364657900
// 0.331000 0.587000
```

Encode/Decode a Lat/Lon point.

```c
double lat = 33.1129;
double lon = -112.5631;
cell_xy = cell_xy_encode((lon+180)/360, (lat+90)/180);
printf("%" PRIu64 "\n", cell);

double x, y;
cell_xy_decode(cell, &x, &y0);
printf("%f %f\n", x, y);

lat = y*180-90;
lon = x*360-180;
printf("%f %f\n", lat, lon);

// output:
// 5548341696901379915
// 0.187325 0.683961
// 33.112900 -112.563100
```

## Performance

```
$ cc -DCELL_TEST -O3 cell.c && BENCH=1 ./a.out
xy encode      1,000,000 ops in 0.008 secs, 8 ns/op, 121,036,069 op/sec
xy decode      1,000,000 ops in 0.002 secs, 2 ns/op, 455,996,352 op/sec
xyz encode     1,000,000 ops in 0.018 secs, 18 ns/op, 54,478,100 op/sec
xyz decode     1,000,000 ops in 0.014 secs, 14 ns/op, 69,003,588 op/sec
xyzm encode    1,000,000 ops in 0.021 secs, 21 ns/op, 47,897,308 op/sec
xyzm decode    1,000,000 ops in 0.016 secs, 16 ns/op, 61,031,431 op/sec
```

## Contact

Josh Baker [@tidwall](http://twitter.com/tidwall)

## License

`cell` source code is available under the MIT [License](/LICENSE).
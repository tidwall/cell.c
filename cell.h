#ifndef CELL_H
#define CELL_H

#include <stdint.h>

typedef uint64_t cell_xy;

cell_xy cell_xy_encode(double x, double y);
void cell_xy_decode(cell_xy cell, double *x, double *y);
const char *cell_xy_string(cell_xy cell, char *str);
cell_xy cell_xy_from_string(const char *str);
int cell_xy_compare(cell_xy a, cell_xy b);

typedef struct { uint64_t hi, lo; } cell_xyz;

cell_xyz cell_xyz_encode(double x, double y, double z);
void cell_xyz_decode(cell_xyz cell, double *x, double *y, double *z);
const char *cell_xyz_string(cell_xyz cell, char *str);
cell_xyz cell_xyz_from_string(const char *str);
int cell_xyz_compare(cell_xyz a, cell_xyz b);

typedef struct { uint64_t hi, lo; } cell_xyzm;

cell_xyzm cell_xyzm_encode(double x, double y, double z, double m);
void cell_xyzm_decode(cell_xyzm cell, double *x, double *y, double *z, double *m);
const char *cell_xyzm_string(cell_xyzm cell, char *str);
cell_xyzm cell_xyzm_from_string(const char *str);
int cell_xyzm_compare(cell_xyzm a, cell_xyzm b);

#endif

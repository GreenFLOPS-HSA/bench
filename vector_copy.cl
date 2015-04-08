__kernel void vcopy(__global float *a, __global float *b) {
  int id = get_global_id(0);
  b[id] = a[id];
}

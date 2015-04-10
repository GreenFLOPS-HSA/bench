__kernel void vcopy(__global float *a, __global float *b) {
  int id = get_global_id(0);
  b[id] = a[id];
}

__kernel void flops_3(__global float *in, __global float *out) {
  int x = get_global_id(0);
  float a = in[x];
  float b = 3.9f * a * (1.0f-a);
  out[x] = b;
}


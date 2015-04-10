__kernel void float_3(__global float *a, __global float *b) {
  int x = get_global_id(0);
  float a = in[x];
  float b = 3.9f * a * (1.0f-a);
  b[id] = b;
}

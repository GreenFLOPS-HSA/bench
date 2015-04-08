__kernel void flops_3(__global float *a, __global float *b) {
  int id = get_global_id(0);
  float c = a[id];           	// Load
  float d = 3.9f * c * (1.0f-c);// Three floating point ops
  b[id] = d;                	// Store
}

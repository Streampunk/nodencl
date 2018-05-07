__kernel void square(
    __global unsigned char* input,
    __global unsigned char* output,
    const unsigned int count) {

    int i = get_global_id(0);
    if (i < count)
        output[i] = input[i] - i % 7;
}

__device__ void swap(int& a, int& b) {
    int t = a;
    a = b;
    b = t;
}
 
__global__ void sort(int* a, int flag_j, int flag_i, int count)
{
    unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int tid_comp = tid ^ flag_j;
 
    if (tid > count)
        return;
 
 
    if (tid_comp > tid) {
        if ((tid & flag_i) == 0) { //ascending
            if (a[tid] > a[tid_comp]) {
                swap(a[tid], a[tid_comp]);
            }
        }
        else { //desending
            if (a[tid] < a[tid_comp]) {
                swap(a[tid], a[tid_comp]);
            }
        }
    }
 
}
 
int main() 
{
 
 
    int count = 800;
    std::default_random_engine e;
 
    int* a, * b;
    cudaMallocHost((void**)&a, sizeof(int) * count);
    cudaMallocHost((void**)&b, sizeof(int) * count);
 
    for (int i = 0; i < count; i++)
    {
        a[i] = e() % 100000;
        //printf("a[%d], %d\n", i, a[i]);
    }
    int* d_a, * d_b;
    cudaMallocManaged((void**)&d_a, sizeof(int) * count);
    cudaMallocManaged((void**)&d_b, sizeof(int) * count);
    cudaMemcpy(d_a, a, sizeof(int) * count, cudaMemcpyHostToDevice);
 
 
    int thread = 512;
    int block = (count + thread) / thread;
 
    int _len = 1;
    while (_len < count)
    {
        _len <<= 1;
        printf("%d \n ", _len);
    }
 
 
 
    for (unsigned int i = 2; i <= _len; i <<= 1) {
        for (unsigned int j = i >> 1; j > 0; j >>= 1) {
 
            sort << <block, thread >> > (d_a, j, i, count);
            cudaDeviceSynchronize();
        }
    }
 
    cudaMemcpy(a, d_a, sizeof(int) * count, cudaMemcpyDeviceToHost);
 
 
    printf("sorted\n");
    for (int i = 0; i < count; i++)
    {
        printf("a[%d], %d\n", i, a[i]);
    }

}
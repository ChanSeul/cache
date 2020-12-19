#include <stdio.h>
#include <stdlib.h>
#include<algorithm>
#include <cuda.h>
using namespace std;

//INSERT CODE HERE---------------------------------
__global__ void make_count_table(int* src, int* count,int size){
	__shared__ int dataShared[101];
	for(int i =0;i<101;i++){
		dataShared[i]=0;
	}
	int gx = blockIdx.x * blockDim.x + threadIdx.x,
		 tx = threadIdx.x;
	if(gx<size){
		atomicAdd(&dataShared[src[gx]],1);
	}
	__syncthreads();
	if(tx<101)
		atomicAdd(&count[tx],dataShared[tx]);
}
__global__ void make_offset_table(int* count){
	__shared__ int src[101],dst[101];
	int tx = threadIdx.x,
	    temp;
	src[tx] = count[tx];
	for(int stride=1;stride<101;stride*=2){
		__syncthreads();
		if(tx-stride>=0)
			dst[tx] = src[tx] + src[tx-stride];
		else
			dst[tx] = src[tx];
		temp=dst[tx];
                dst[tx]=src[tx];
                src[tx]=temp;
	}
	__syncthreads();
	count[tx] = src[tx];

}
__global__ void sort(int* src,int* dst,int* offset,int size){
	//__shared__ int OffsetShared[101];
	int gx = blockIdx.x * blockDim.x + threadIdx.x;
	int n;
	/*if(tx<101)
		OffsetShared[tx]=offset[tx];
	__syncthreads();
*/
	if(gx<size){
		n=atomicSub(&offset[src[gx]],1);
		n=n-1;
		//printf("dst[%d] = src[%d]\n",n,gx);
		dst[n]=src[gx];
	}
}

void verify(int* src, int*result, int input_size){
	sort(src, src+input_size);
	long long match_cnt=0;
	for(int i=0; i<input_size;i++)
	{
		if(src[i]==result[i])
			match_cnt++;
	}

	if(match_cnt==input_size)
		printf("TEST PASSED\n\n");
	else
		printf("TEST FAILED\n\n");

}

void genData(int* ptr, unsigned int size) {
	while (size--) {
		*ptr++ = (int)(rand() % 101);
	}
}

int main(int argc, char* argv[]) {
	int* pSource = NULL;
	int* pResult = NULL;
	int* pCount = NULL;
	int input_size=0;

	cudaEvent_t start, stop;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);

	if (argc == 2)
		input_size=atoi(argv[1]);
	else
	{
    		printf("\n    Invalid input parameters!"
	   		"\n    Usage: ./sort <input_size>"
           		"\n");
        	exit(0);
	}

	//allocate host memory
	pSource=(int*)malloc(input_size*sizeof(int));
	pResult=(int*)malloc(input_size*sizeof(int));
	pCount=(int*)malloc(101*sizeof(int));
	// generate source data
	genData(pSource, input_size);
/*	for(int i=0;i<input_size;i++)
		printf("pSource[%d] = %d\n",i,pSource[i]);*/
	//allocate device memory
	int* pSourceDev = NULL;
	int* pResultDev = NULL;
	int* pCountDev = NULL;
	cudaMalloc((void**)&pSourceDev,input_size*sizeof(int));
	cudaMalloc((void**)&pResultDev,input_size*sizeof(int));
	cudaMalloc((void**)&pCountDev,101*sizeof(int));	
	// start timer
	cudaEventRecord(start, 0);
	cudaMemcpy(pSourceDev, pSource, input_size * sizeof(int), cudaMemcpyHostToDevice);

	dim3 dimGrid(ceil((float)input_size/(float)1024),1,1);
	dim3 dimBlock(1024,1,1);
	make_count_table<<<dimGrid,dimBlock>>>(pSourceDev,pCountDev,input_size);
	cudaMemcpy(pCount,pCountDev,101*sizeof(int),cudaMemcpyDeviceToHost);
	/*for(int i=0;i<101;i++){
                printf("Count[%d] = %d\n",i,pCount[i]);
        }*/


	dim3 dimGrid2(1,1,1);
	dim3 dimBlock2(101,1,1);
	make_offset_table<<<dimGrid2,dimBlock2>>>(pCountDev);
	cudaMemcpy(pCount,pCountDev,101*sizeof(int),cudaMemcpyDeviceToHost);
	/*for(int i=0;i<101;i++){
		printf("Offset[%d] = %d\n",i,pCount[i]);
	}*/
	//cudaMemcpy(pResult, pResultDev, input_size * sizeof(int), cudaMemcpyDeviceToHost);
	dim3 dimGrid3(ceil((float)input_size/(float)1024),1,1);
        dim3 dimBlock3(1024,1,1);
	sort<<<dimGrid3,dimBlock3>>>(pSourceDev,pResultDev,pCountDev,input_size);
	cudaMemcpy(pResult, pResultDev, input_size * sizeof(int), cudaMemcpyDeviceToHost);
	/*for(int i=0;i<input_size;i++)
		printf("Reslut[%d]= %d\n",i,pResult[i]);
*/
	//INSERT CODE HERE--------------------
	





	// end timer
	float time;
	cudaEventRecord(stop, 0);
	cudaEventSynchronize(stop);
	cudaEventElapsedTime(&time, start, stop);
	printf("elapsed time = %f msec\n", time);
	cudaEventDestroy(start);
	cudaEventDestroy(stop);
	
    	printf("Verifying results..."); fflush(stdout);
	verify(pSource, pResult, input_size);
	fflush(stdout);
	/*for( int i=0;i<input_size;i++){
                printf("index %d : %d %d \n",i,pSource[i],pResult[i]);
        }*/

}



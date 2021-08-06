/*     include librarys     */
#include <math.h>
#include <stdlib.h>
#include "mex.h"

void DAScomNtNr128fh(short *pt, float *xele, float *x, float *y, int *rf)
{
	unsigned short Nt = 2560; 		  //每个探头单张图接收到的数据长度
	unsigned char  N  = 253;  		  //图像在平行探头方向的数据点个数
	unsigned short M  = 1559; 		  //图像在垂直探头方向的数据点个数
	unsigned char  Nx = 64;           //探头个数
	unsigned char  Nf = 21;           //一次成像使用到的单张图像的数量
	unsigned short cs = 1540;         //声速
	unsigned int   fs = 31250000;     //采样频率
	float alpha[21] = {-0.17453292,   //声波发射时的倾斜角 向0号探头倾斜为负 向63好探头倾斜为正
					    -0.15707964,
					    -0.13962634,
					    -0.12217305,
					    -0.10471976,
					    -0.087266460,
					    -0.069813170,
					    -0.052359879,
					    -0.034906585,
					    -0.017453292,
					     0,
					     0.017453292,
					     0.034906585,
					     0.052359879,
					     0.069813170,
					     0.087266460,
					     0.10471976,
					     0.12217305,
					     0.13962634,
					     0.15707964,
					     0.17453292};

	/*   Compute the delay   */
	float *sa, *ca;
	sa = (float *)malloc(sizeof(float)*Nf);
	ca = (float *)malloc(sizeof(float)*Nf);
	for (unsigned char nf = 0; nf < Nf; nf++) {
		sa[nf] = (float)sin(alpha[nf]);
		ca[nf] = (float)cos(alpha[nf]);
	}
	
	unsigned short ***Delay;
	Delay = (unsigned short ***)malloc(sizeof(unsigned short **)*Nx);

	/*   Delay time of backward propagation   */
	float dist;
	unsigned short dt;
	for (unsigned char i = 0; i < Nx; i++) {
		Delay[i] = (unsigned short **)malloc(sizeof(unsigned short *)*N);
		for (unsigned char j = 0; j < N; j++) {
			Delay[i][j] = (unsigned short *)malloc(sizeof(unsigned short)*M);
			for (unsigned short k = 0; k < M; k++) {
				dist = (float)sqrt((x[j] - xele[i])*(x[j] - xele[i]) + y[k] * y[k]);  											   //图像点与对应探头的物理距离
				dt = (unsigned short)(dist*fs / cs);                                  											   //图像点与对应探头的采样距离
				Delay[i][j][k] = dt;
			}
		}
	}
	
	int valsum1;
	unsigned short delay;
	for (unsigned char nf = 0; nf < Nf; nf++) {                                       											   //21张图像叠加 
		for (unsigned char nx = 0; nx < N ; nx++) {                                   											   //遍历深度方向图像点
			for (unsigned short ny = 0; ny < M; ny++) {                               											   //遍历宽度方向图像点
				valsum1 = 0;
				for (unsigned char n = 0; n < Nx; n++) {                              											   //每个探头的单张图叠加
					if ( (xele[n] - x[nx])/y[ny] > 1 || (xele[n] - x[nx])/y[ny] < -1  )
						continue;
					if (alpha[nf] >= 0 ){
						delay = (unsigned short)(Delay[n][nx][ny] + (y[ny]*ca[nf] + x[nx]*sa[nf])*fs / cs + 0.5);                  //加算倾斜角为正时探头发射延迟
					}
					else{
						delay = (unsigned short)(Delay[n][nx][ny] + (y[ny]*ca[nf] + (x[nx] - xele[Nx-1])*sa[nf])*fs / cs + 0.5);   //加算倾斜角为负时探头发射延迟
					}
					if (delay >= 0){
						if (delay <= Nt-1){
							valsum1 = valsum1 + pt[ n*Nt*Nf + delay + nf*Nt];
						}
					}
				}
				rf[nx*M + ny] = rf[nx*M + ny] + (int)valsum1;
			}
		}
	}
	
	for (unsigned char i = 0; i < Nx; i++) {
		for (unsigned short j = 0; j < N; j++) {
			free(Delay[i][j]);
		}
		free(Delay[i]);
	}
	free(Delay);
	free(sa);
	free(ca);
	
	return;
}

/* [rf] = DAScompound64(short pt, float xele, float x, float y) */
void mexFunction(int nlhs, mxArray *plhs[],
	int nrhs, const mxArray*prhs[])
{
/* define the hyper-parameters */
int N  = 253;
int M  = 1559;

/* input setting */
int *rf;
short *pt;
float *x, *y, *xele;

/* check for proper number of arguments */
if (nrhs != 4) {
	mexErrMsgIdAndTxt("MATLAB:DASBF:invalidNumInputs",
		"Four input arguments required.");
}
else if (nlhs > 1) {
	mexErrMsgIdAndTxt("MATLAB:DASBF:maxlhs",
		"Too many output arguments.");
}

/* create matrices for the return argument */
plhs[0] = mxCreateNumericMatrix(M, N, mxINT32_CLASS, mxREAL);

/* assign pointrs to the input and output parameters */
pt = (short *)mxGetPr(prhs[0]);
xele = (float *)mxGetPr(prhs[1]);
x = (float *)mxGetPr(prhs[2]);
y = (float *)mxGetPr(prhs[3]);
rf = (int *)mxGetPr(plhs[0]);

/* Do the actual computations in a subroutine */
DAScomNtNr128fh(pt, xele, x, y, rf);

return;
}

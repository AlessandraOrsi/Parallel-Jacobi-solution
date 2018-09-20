#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <math.h>



int main(int argc, char *argv[]) {

	int rank,size;
	int max_iter=100;
	char hostname[50];
	int length;
	int tag=0;
	int n=14000; 
	int i,j;
	int numIterazioni;
	double tolleranza2=0.0;
	double start, finish;
  	srand(13);
  	
 	double *x_processi;
	double *x_aggiornata;
	double *final_matrix;
	
  	gethostname(hostname, length);
	MPI_Init(&argc, &argv);
	start= MPI_Wtime();
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Status status;
	
  	
  	int works[size];
  	int resto=n%size;
  	int l;
  	int disp[size];
  	int dis=0;
  	
  	for(i=0;i<size;i++){
  		l = n/size;
  		if(resto>0){
  			works[i]=(l+1)*n;
  			resto--;
  		}
  		else{ 
  			works[i]=l*n;
  		}
  		disp[i]=dis;
  		dis+=works[i];
  	}
  	
  	l = works[rank]/n;
  	l = l + 2;
  	
   	
  	x_processi = (double *)malloc(sizeof(double) * l*n);
  	x_aggiornata = (double *)malloc(sizeof(double) * l*n);
	final_matrix = (double *)malloc(sizeof(double) * n*n);
  
  	
	////////////////////////creo le sottomatrici per ogni processo/////////////////////
	for(i=0; i < l ;i++){
		for(j=0; j < n; j++){
			
			if ((rank == 0 && i == 1) || (rank == (size-1) && i == (l-2)))
			{
				x_processi[(i*n)+j]= -1;
				//printf("%.2f ",x_processi[(i*n)+j]);
				//fflush(stdout);
			}
			else if(i==0 || i==l-1){
				x_processi[(i*n)+j]=0;
				//printf("%.2f ",x_processi[(i*n)+j]);
				//fflush(stdout);
			}else
			{
				//x_processi[i][j]= rand()%10;
				x_processi[(i*n)+j]= rank;
				//printf("%.2f ",x_processi[(i*n)+j]);
				//fflush(stdout);
			}
			
		}
		//printf("\n");
		//fflush(stdout);
	}
	////////////////////////////////// Comunicazione /////////////////////////////////////////////////
	if(rank==size-1) l=l-1;
	numIterazioni=0;
	do{
		if(rank<size-1){
			MPI_Send(&x_processi[(l-2)*n],n,MPI_DOUBLE,rank+1,tag,MPI_COMM_WORLD);
		}
		if(rank>0){
			MPI_Recv(&x_processi[0],n,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&status);
		}
		if(rank>0){
			MPI_Send(&x_processi[1*n],n,MPI_DOUBLE,rank-1,1,MPI_COMM_WORLD);
		}
		if(rank<size-1){
			MPI_Recv(&x_processi[((l-1)*n)],n,MPI_DOUBLE,rank+1,1,MPI_COMM_WORLD,&status);
		}
	
	/////////////////////////////////aggiorno i valori interi/////////////////////////////
	double tolleranza=0.0;
	i=1;
	if(rank==0) i=2;
	
	
	for(; i < l - 1; i++){
			
		for(j = 1; j < n-1; j++){
			x_aggiornata[(i*n)+j]=(x_processi[((i+1)*n)+j]+x_processi[((i-1)*n)+j]+x_processi[(i*n)+(j-1)]+x_processi[(i*n)+(j+1)])/4.0;
			tolleranza+=(x_aggiornata[(i*n)+j]-x_processi[(i*n)+j])*(x_aggiornata[(i*n)+j]-x_processi[(i*n)+j]);
			
		}
							
	}
	////////////////////////////////////////////////////////////////////////////////////
	
	///////////////////////trasferisco solo i valori interni///////////////////////////		
	i=1;
	if(rank==0) i=2;	
	for(;i<l-1;i++){
		for(j=1;j<n-1;j++){
			x_processi[(i*n)+j]=x_aggiornata[(i*n)+j];
		}
	}
	//////////////////////////////////////////////////////////////////////////////////
	numIterazioni++;
	
	MPI_Allreduce(&tolleranza,&tolleranza2,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
	tolleranza2=sqrt(tolleranza2);
	/*if(numIterazioni==1){
		printf("%d iterazione \n",numIterazioni);
	for(i=0;i<l;i++){
	 	for(j=0;j<n;j++){
	 		printf(" %.2f\t",x_processi[(i*n)+j]);
	 	}
	 	printf("\n");
	 }
	}*/
	} while(numIterazioni<max_iter && tolleranza2>1.0e-2);
	
	//printf("#############\n");
	//printf("iterazione : %d\n",numIterazioni);
	//printf("tolleranza : %e\n",tolleranza2);
 
	 /*for(i=0;i<l;i++){
	 	for(j=0;j<n;j++){
	 		printf(" %.2f\t",x_processi[(i*n)+j]);
	 	}
	 	printf("\n");
	 }*/
 
 	
	MPI_Gatherv(&x_processi[1*n], works[rank], MPI_DOUBLE, final_matrix, works, disp, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	//printf("#########\n");
	
	  if(rank==0){
		/*for(i=0;i<size;i++){
			printf("works[%d] %d\n",i,works[i]);
			printf("displ[%d] %d\n",i,disp[i]);
		}*/

		////// Stampa della matrice finale //////
		/*for(i=0; i < n; i++){
			for(j=0; j< n ;j++){
				
				printf("%.2f\t ",final_matrix[(i*n)+j]);
				fflush(stdout);
			}
			 
			printf("\n");
			fflush(stdout);
		}*/	
		/////////////////////////////////////////

		finish= MPI_Wtime();
		printf("Tempo: %f\n", finish-start);
	}
	
	fflush(stdout);
	MPI_Finalize();
	return 0;
	
}

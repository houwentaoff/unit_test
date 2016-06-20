#include <time.h>
#include <stdio.h>
#include <unistd.h>

int main(){

	char strbuf[1000][100];
	int i,j,k;

	clock_t before, after;
	double duration;

	before = clock();

	for(i=0;i<1000;++i){
		FILE *f = fopen("/sys/kernel/gyro-data/gyro", "r");
		read(fileno(f), strbuf[i], sizeof(strbuf[i]));
		fclose(f);
	}

	after = clock();
	duration = (double)(after - before) / CLOCKS_PER_SEC;

	for(j=0;j<100;j++){
		for(k=0;k<10;k++){
			printf("%s\b",strbuf[j*10+k]);
		}
		printf("\n");
	}
	printf( "1000 samples in %f seconds\n", duration );

	return 0;
}

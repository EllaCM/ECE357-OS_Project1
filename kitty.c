#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define EXIT_FAIL -1

bool binary_check(char buf[], int size){
	for(int i=0; i<size; i++){
		if(!(isprint(buf[i])||isspace(buf[i]))){
			return true;
		}
	}
	return false;
}

//argv--file name
void ReadWrite(int fd_in, int fd_out, int r_cnt, int w_cnt, char *buffer, int *total_transfer, bool *binary){
	int write_o = 0;
	int read_i=read(fd_in, buffer, BUFFER_SIZE);
	while(read_i>0){
		r_cnt++;	
		//check binary
		if(!(*binary))	*binary=binary_check(buffer,read_i);
		int bytes_wrote = 0;
		int total_bytes = 0;
		//unload bytes for writing, also prevent partial writing
		while(write_o<read_i){
			buffer+=bytes_wrote;
			read_i-=bytes_wrote;
			write_o = write(fd_out, buffer, read_i);
			if(write_o<0){
				fprintf(stderr, "ERROR: CANNOT write in the output file: %s\n", strerror(errno));
				exit(EXIT_FAIL);
			}
			total_bytes+=write_o;
			w_cnt++;
		}
		*total_transfer+=total_bytes;
	}
	fprintf(stdout, "make sure readwrite has been processed--read:%d,write%d\n",r_cnt,w_cnt);
	if(read_i<0){
		fprintf(stderr, "ERROR: CANNOT read from input file: %s\n", strerror(errno));
		exit(EXIT_FAIL);
	}
}


int main(int argc, char* argv[]){
	bool binary = false;
	char buff[BUFFER_SIZE];
	char* outfile = NULL;
	int rCNT = 0;
	int wCNT = 0;
	int out = 0;
	int total_transfer = 0;
	int fd_in;
	//check valid i/o file names
	while((out=getopt(argc, argv, "o:"))!=-1){
		switch(out){
			case 'o':
				outfile = optarg;
				break;
		}
		
	}
	//fprintf(stderr,"ERROR: no '-o' flag detected\n");
	//exit(EXIT_FAIL);

	int fd_out = STDOUT_FILENO;
	if(outfile==NULL){
		fd_out = 0;
	}else{
		fd_out = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if(fd_out<0){
			fprintf(stderr,"ERROR: CANNOT open the output file:%s\n",strerror(errno));
			exit(EXIT_FAIL);
		}	
	}
	
	for(int i=optind; i<argc; i++){
		fprintf(stdout, "processing argument: %s\n",argv[optind]);
		if(strcmp(argv[optind],"-")==0){
			fd_in = STDIN_FILENO;
			argv[optind] = "STDIN_FILENO";
		}else if((fd_in = open(argv[optind],O_RDONLY))<0){
			fprintf(stderr, "ERROR: CANNOT open the file for reading: %s\n",strerror(errno));
			exit(EXIT_FAIL);
		}		
		binary = false;
		ReadWrite(fd_in, fd_out, rCNT, wCNT, buff, &total_transfer, &binary);
		
		if(fd_in!=STDIN_FILENO && close(fd_in)<0){
			fprintf(stderr, "ERROR: FAILED to close input file: %s\n", strerror(errno));
			exit(EXIT_FAIL);
		}
		
	}

	for(int j=optind+1; j<argc; j++){
		strcat(argv[optind],argv[j]);
	}
	
	fprintf(stderr,"The file content is copied from %s to %s. Total bytes transfered:%d. Count in read request:%d; count in write request:%d\n", argv[optind], outfile, total_transfer, rCNT, wCNT);
	
	if(fd_out!=STDOUT_FILENO && close(fd_out)<0){
		fprintf(stderr, "ERROR: FAILED to close output file: %s\n", strerror(errno));
		exit(EXIT_FAIL);
	}
	return (EXIT_SUCCESS);
		
}

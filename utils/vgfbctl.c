#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <vg.h>

int fbm = -1;

void run_set(char** args, size_t n){
	if(n<2){
		fprintf(stderr,"Usage: set <param> <values>\n");
		return;
	}

	if(!strcmp("mode",args[0])){
		if(!strcmp("none",args[1])){
			if( ioctl( fbm, IOCTL_VG_SET_MODE, VGFB_MODE_NONE ) == -1 )
				perror("IOCTL_VG_SET_MODE failed");
		}else{
			fprintf(stderr,"Unknown mode \"%s\"\n",args[1]);
		}
	}else if(!strcmp("resolution",args[0])){
		if( n != 5 ){
			fprintf(stderr,"Usage: set resolution <width px> <height px> <width mm> <height mm>\n");
			return;
		}
		unsigned long resolution[4];
		resolution[0] = atol(args[1]);
		resolution[1] = atol(args[2]);
		resolution[2] = atol(args[3]);
		resolution[3] = atol(args[4]);
		if( ioctl( fbm, IOCTL_VG_SET_RESOLUTION, resolution ) == -1 )
			perror("IOCTL_VG_SET_RESOLUTION failed");
	}else{
		fprintf(stderr,"Unknown parameter \"%s\"\n",args[0]);
	}
}

void run(char** args, size_t n){
	if(!n) return;

	if(!strcmp("set",args[0])){
		run_set(args+1,n-1);
	}else{
		fprintf(stderr,"Unknown command \"%s\"\n",args[0]);
	}
}

int main(){
	fbm = open("/dev/vgfbmx",0);
	if( fbm == -1 ){
		perror("Failed to open /dev/vgfbmx");
		return 1;
	}
	char buf[1024] = {0};
	while( fgets(buf,sizeof(buf),stdin) ){
		const char* del = " \n";
		char* tok = strtok(buf,del);
		char* args[32];
		size_t n = 0;
		while( tok && n != sizeof(args)/sizeof(*args) ){
			args[n++] = tok;
			tok = strtok(0,del);
		}
		run(args,n);
	}
	close(fbm);
	return 0;
}

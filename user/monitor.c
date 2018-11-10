#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sched_monitor.h>

int
main(int     argc,
     char ** argv)
{
	int fd, status, i, j;
	int num = atoi(argv[1]);
	int ctr = 1;
	size_t copied;
//	preemption_info_t data[sizeof(preemption_info_t) * num];
	preemption_info_t *data = malloc(sizeof(preemption_info_t) * num);

	if(argc != 2){

		printf("usage: %s <the number of preemptions to track>\n", argv[0]);
		exit(0);
	}


    fd = open(DEV_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Could not open %s: %s\n", DEV_NAME, strerror(errno));
        return -1;
    }

    status = enable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not enable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1;
    }

	for(j = 0; j < 10000000; j++){

		ctr *= 2;
	}

	for(i = 0; i < num; i++){

		copied = read(fd, (data + i), sizeof(preemption_info_t));
		if(copied == 0){
			printf("on_core_time:%llu\n", data[i].on_core_time);
			printf("wait_time:%llu\n", data[i].wait_time);
			printf("which_cpu:%d\n", data[i].on_which_core);
			printf("name:%s\n", data[i].name);
			printf("\n");
		}
		else if(copied == -1){

			printf("failed to read\n");
		}
	}


    status = disable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not disable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    printf("Monitor ran to completion!\n");

    return 0;
}

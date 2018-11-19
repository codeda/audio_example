#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <signal.h>

#include "popen2.h"
#include "log.h"
#include <fcntl.h>

const int audio_freq = 48000;
const char *audio_out = "ffmpeg|-vn|-f|s16le|-acodec|pcm_s16le|-ar|%d|-ac|2|-i|-|-an|-i|%s|-c:v|copy|-c:a|aac|-b:a|128k|-shortest|-f|mp4|-y|%s";
const char *audio_in = "ffmpeg|-i|%s|-vn|-f|s16le|-acodec|pcm_s16le|-ar|%d|-ac|2|-";

int video_pid=0;
int output_pid=0;
int video_fd_in=0;
int audio_fd_out=0;

typedef struct overlay{
	char* filename;
	int ms;
	char* cmd;
	int fd;
	int pid;
} _overlay;

struct overlay* overlays=NULL;

struct overlay* current_overlay=NULL;

int main(int argc, char** argv) {
	char *video_in = (char*)malloc(MAX_PATH);
	snprintf(video_in, MAX_PATH-1, audio_in, argv[1], audio_freq);
	video_pid = popen2(video_in, NULL, &video_fd_in, NULL);

	char *out = (char*)malloc(MAX_PATH);
	snprintf(audio_out, MAX_PATH-1, audio_out, audio_freq, argv[1], argv[2]);
	output_pid = popen2(out, &audio_fd_out, NULL, NULL);

	int overlay_no=(argc/2)-1;
	overlays = (struct overlay*)malloc(sizeof(struct overlay)*overlay_no);
	for (int i=0;i<overlay_no;i++) {
		struct overlay *o = overlays+i;
		o->filename = (char*)malloc(MAX_PATH);
		memcpy(o->filename, argv[2+i*2], strlen(argv[2+i*2])+1);
		o->ms = atoi(argv[3+i*2]);
		o->cmd = (char*)malloc(MAX_PATH);
		snprintf(o->cmd, MAX_PATH-1, audio_in, o->filename, audio_freq);
		o->pid = popen2(o->cmd, NULL, &o->fd, NULL);
	}

	int ms=0;
	int chunk_size = audio_freq/1000*2*2;
	uint8_t *audio_data = (uint8_t*)malloc(chunk_size);
	uint8_t *overlay_data = (uint8_t*)malloc(chunk_size);

	while(true) {
		for (int i=0;i<overlay_no;i++) {
			if (overlays[i].ms == ms) {
				if (current_overlay!=NULL) {
					pclose2(current_overlay->pid);
				}
				current_overlay = overlays+i;
			}
		}		
		int sread = read(video_fd_in, audio_data, chunk_size);
		if (sread != chunk_size) 
			return 0;
		if (current_overlay != NULL) {
			sread = read(current_overlay->fd, overlay_data, chunk_size);
			if (sread != chunk_size) {
				pclose2(current_overlay->pid);
				current_overlay = NULL;
			}
		}
		if (current_overlay!=NULL) {
			write(audio_fd_out, overlay_data, chunk_size);
		} else {
			write(audio_fd_out, audio_data, chunk_size);
		}
	}
	return 0;
}

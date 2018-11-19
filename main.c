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
const char *audio_out = "/usr/local/bin/ffmpeg|-vn|-f|s16le|-acodec|pcm_s16le|-ar|%d|-ac|2|-i|-|-an|-i|%s|-c:v|copy|-c:a|libmp3lame|-b:a|128k|-shortest|-y|%s";
const char *audio_in = "/usr/local/bin/ffmpeg|-i|%s|-vn|-f|s16le|-acodec|pcm_s16le|-ar|%d|-ac|2|-";

int video_pid=0;
int output_pid=0;
int video_fd_in=0;
int video_fd_1=0;
int video_fd_2=0;
int audio_fd_out=0;
int audio_fd_1=0;
int audio_fd_2=0;

typedef struct overlay{
	char* filename;
	int ms;
	char* cmd;
	int fd;
	int pid;
	int fd_in;
	int fd_err;
	pthread_t vt;
} _overlay;

struct overlay* overlays=NULL;

struct overlay* current_overlay=NULL;

void *read_stderr(void* ptr) {
         int c=0;
         ssize_t max = 4096;
         char *s=(char*)malloc(max);
         memset(s, 0, max);
         int fd_err=(int)ptr; 
	 int flags = fcntl(fd_err, F_GETFL, 0);
         fcntl(fd_err, F_SETFL, flags | O_NONBLOCK);
         while (true) {
                 int n = 0;
                 fd_set fds;
                 FD_ZERO(&fds);
                 FD_SET(fd_err, &fds);
                 struct timeval tv;
                 tv.tv_sec = 1;
                 tv.tv_usec = 0;
                 if (select(fd_err+1, &fds, 0, 0, &tv) == 1) {
                         if (FD_ISSET(fd_err, &fds)) {
                                 ssize_t count = read(fd_err, s+c, 1);
                                 if (count!=1) {
                                         int e = errno;
                                         Log("Can't read stream of output_ffmpeg_stderr, errno %d\n", e);
                                         break;
                                 }
                         } else continue;
                 } else continue;
                 c++;
                 if (c == max) {
                         Log("stderr: next 4096 bytes read\n");
                         c=0;
                         continue;
                 }
                 if (s[c-1] == 10) { // 0x0A
			s[c-1]=0;
			Log("stderr %d printed: >%s<\n", fd_err, s);
			c=0;
		}
	}
	return NULL;
}

int main(int argc, char** argv) {
	char *video_in = (char*)malloc(MAX_PATH);
	snprintf(video_in, MAX_PATH-1, audio_in, argv[1], audio_freq);
	Log("Starting %s\n", video_in);
	pthread_t vt_video_in;
	video_pid = popen2(video_in, &video_fd_1, &video_fd_in, &video_fd_2);
        int te = pthread_create(&vt_video_in, NULL, &read_stderr, (void*)video_fd_2);
        if (te) {
             Log("Can't create thread: %d\n", te);
             exit(-1);
        }
	Log("Video PID is %d, fd %d, fd_err %d\n", video_pid, video_fd_in, video_fd_2);

	char *out = (char*)malloc(MAX_PATH);
	snprintf(out, MAX_PATH-1, audio_out, audio_freq, argv[1], argv[2]);
	Log("Starting %s\n", out);
	output_pid = popen2(out, &audio_fd_out, &audio_fd_1, &audio_fd_2);
	pthread_t vt_video_out;
        te = pthread_create(&vt_video_out, NULL, &read_stderr, (void*)audio_fd_2);
	if (te) {
             Log("Can't create thread: %d\n", te);
             exit(-1);
        }	
	Log("Output PID is %d, fd %d, fd_err %d\n", output_pid, audio_fd_out, audio_fd_2);

	int overlay_no=(argc/2)-1;
	Log("Number of overlays: %d\n", overlay_no);
	overlays = (struct overlay*)malloc(sizeof(struct overlay)*overlay_no);
	for (int i=0;i<overlay_no;i++) {
		struct overlay *o = overlays+i;
		o->filename = (char*)malloc(MAX_PATH);
		memcpy(o->filename, argv[3+i*2], strlen(argv[3+i*2])+1);
		o->ms = atoi(argv[4+i*2]);
		o->cmd = (char*)malloc(MAX_PATH);
		snprintf(o->cmd, MAX_PATH-1, audio_in, o->filename, audio_freq);
		o->pid = popen2(o->cmd, &o->fd_in, &o->fd, &o->fd_err);
		Log("Added overlay %d: filename=%s, ms=%d, cmd=%s, pid=%d, fd=%d, fd_in=%d, fd_err=%d\n", 
				i, o->filename, o->ms, o->cmd, o->pid, o->fd, o->fd_in, o->fd_err);
		int te = pthread_create(&o->vt, NULL, &read_stderr, (void*)o->fd_err);
        	if (te) {
            	     Log("Can't create thread: %d\n", te);
          	     exit(-1);
       		}
	}

	int ms=0;
	int chunk_size = audio_freq/1000*2*2;
	uint8_t *audio_data = (uint8_t*)malloc(chunk_size);
	uint8_t *overlay_data = (uint8_t*)malloc(chunk_size);
	while(true) {
		if (ms%1000==0) {
			Log("ms=%d\n", ms);
		}
		for (int i=0;i<overlay_no;i++) {
			if (overlays[i].ms == ms) {
				if (current_overlay!=NULL) {
					Log("Closing %s as a new overlay opens\n", current_overlay->filename);
					pclose2(current_overlay->pid);
				}
				current_overlay = overlays+i;
				Log("Current overlay is now %s\n", current_overlay->filename);
			}
		}	
		errno=0;	
		int bytes_to_read = chunk_size;
		while (bytes_to_read>0) {
			int sread = read(video_fd_in, audio_data+chunk_size-bytes_to_read, bytes_to_read);
			if (errno || sread==0) {
			        int e = errno;
                        	Log("Can't read video input at %d, read: %d, errno: %d\n", ms, sread, e);
                    		return 0;
			}
			bytes_to_read-=sread;
		}
		if (current_overlay != NULL) {
			int bytes_to_read = chunk_size;
			while (bytes_to_read>0) {
				int sread = read(current_overlay->fd, overlay_data+chunk_size-bytes_to_read, bytes_to_read);
				if (errno || sread==0) {
					Log("Can't read overlay %s input at %d\n", current_overlay->filename, ms);
					pclose2(current_overlay->pid);
					current_overlay = NULL;
					break;
				}
				bytes_to_read-=sread;
			}
		}
		if (current_overlay!=NULL) {
			write(audio_fd_out, overlay_data, chunk_size);
		} else {
			write(audio_fd_out, audio_data, chunk_size);
		}
		ms++;
	}
	return 0;
}

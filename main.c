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

const char *audio_out = "ffmpeg|-vn|-f|s16le|-acodec|pcm_s16le|-ar|%d|-ac|2|-i|-|-an|-i|%s|-c:v|copy|-c:a|aac|-b:a|128k|-shortest|-f|mp4|%s";
const char *audio_in = "ffmpeg|-i|%s|-vn|-f|s16le|-acodec|pcm_s16le|-ar|%d|-ac|2|-";

int main(int argc, char** argv) {
	return 0;
}

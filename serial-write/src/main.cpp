
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define BUFFER_SIZE 128
#define BAUDRATE    B9600

//float data[] = {
//    -12.1297f,-8.7258f,-3.00238f,4.07658f,8.77581f,10.8543f,12.722f,15.6138f,18.8972f,
//    20.0118f,20.4636f,21.9698f,24.5604f,28.5367f,41.0077f,71.0707f,118.876f,163.097f,
//    185.087f,184.214f,171.743f,147.433f,118.063f,91.5244f,71.1008f,48.6891f,23.2049f,
//    4.25732f,-6.97865f,-12.4611f,-12.5515f,-8.69568f,-2.67103f,3.53437f,6.78768f,
//    8.65532f,11.3965f,14.5294f,16.9995f,17.7525f,18.5358f,21.6083f,24.5002f,34.2601f,
//    50.376f,86.9758f,134.45f,166.712f,177.496f,171.984f,155.687f,131.378f,103.092f,
//    81.162f,60.9493f,36.2784f,12.6316f,-2.49029f,-10.8646f,-13.3045f,-10.985f,-5.62311f,
//    1.36549f,6.66719f,8.68544f,11.0049f,14.4089f,17.8128f,18.8972f,19.1683f,20.5239f,
//    23.717f,27.5125f,39.0798f,66.8535f,113.816f,159.633f,182.346f,182.045f,169.755f,
//    147.734f,118.967f,91.8257f,71.4623f,49.2012f,24.1086f,4.28745f,-7.61124f,-13.0033f,
//    -13.0334f,
//};
// constexpr uint32_t N = sizeof(data) / sizeof(data[0]);

// Different data waves, to be concatenated together and sent to serial device. The choice
// of wave is random so that there will be some variability in the signal
std::vector<std::vector<float>> data = {
    // 15 timesteps
    {
        48.401f,62.001f,79.395f,86.285f,84.739f,79.717f,71.706f,65.821f,63.056f,61.394f,
        59.115f,56.269f,52.730f,50.581f,49.026f,
    },
    // 25
    {
        48.401f,53.300f,66.014f,76.205f,83.002f,86.234f,86.104f,84.454f,81.824f,78.293f,
        73.453f,68.415f,65.821f,64.023f,62.866f,61.849f,60.744f,59.412f,57.580f,56.035f,
        53.634f,52.521f,51.075f,49.456f,49.026f,
    },
    // 34 (original)
    {
        48.401f,50.352f,58.353f,68.323f,75.528f,80.795f,84.755f,86.266f,86.289f,85.360f,
        83.844f,81.824f,79.199f,76.314f,72.341f,68.733f,66.627f,65.177f,63.854f,62.987f,
        62.333f,61.559f,60.744f,59.819f,58.466f,57.330f,56.184f,54.403f,52.974f,52.477f,
        51.383f,50.132f,49.057f,49.026f,
    },
    // 40
    {
        48.401f,49.900f,55.795f,63.853f,71.186f,76.778f,81.135f,84.613f,86.168f,86.425f,
        85.947f,84.914f,83.586f,81.824f,79.571f,77.434f,74.353f,70.631f,68.145f,66.503f,
        65.276f,64.166f,63.172f,62.749f,62.095f,61.440f,60.744f,59.968f,58.932f,57.791f,
        56.917f,55.890f,54.254f,53.075f,52.559f,51.842f,50.809f,49.794f,49.026f,49.026f,
    },
    // 50
    {
        48.401f,49.594f,53.080f,58.561f,65.441f,71.058f,75.749f,79.395f,82.507f,84.867f,
        86.104f,86.372f,86.229f,85.715f,84.739f,83.673f,82.439f,80.594f,78.903f,77.057f,
        74.605f,71.706f,69.200f,67.486f,66.364f,65.387f,64.517f,63.605f,63.056f,62.696f,
        62.175f,61.654f,61.120f,60.556f,59.918f,59.115f,58.095f,57.412f,56.670f,55.823f,
        54.521f,53.486f,52.730f,52.491f,51.821f,51.004f,50.158f,49.426f,49.026f,49.026f,
    },
};

//
int main(int argc, char** argv) {

    int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (fd == -1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return 1;
    }

    grantpt(fd);
    unlockpt(fd);

    const char *pts_name = ptsname(fd);
    fprintf(stdout, "ptsname: %s\n", pts_name);
    // fprintf(stdout, "data size: %d\n", N);


    // serial port parameters
    struct termios newtio;
    memset(&newtio, 0, sizeof(newtio));
    struct termios oldtio;
    tcgetattr(fd, &oldtio);

    newtio = oldtio;
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = 0;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush(fd, TCIFLUSH);

    cfsetospeed(&newtio, BAUDRATE);
    tcsetattr(fd, TCSANOW, &newtio);

    // loop waves
    float f;
    size_t n_datasets = data.size();
    while (1)
    {
        size_t current_dataset = rand() % n_datasets;
        // size_t current_dataset = rand() % 3;
        size_t n = 0;
        size_t N = data[current_dataset].size();
        // printf("dataset %zu selected (N=%zu)\n", current_dataset, N);

        while (n < N)
        {
            f = data[current_dataset][n++ % N];
            write(fd, &f, sizeof(float));

            usleep(20000);  // ~50 Hz
        }
    }

    close(fd);
    return 0;

}

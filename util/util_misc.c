#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <assert.h>

#include <inttypes.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#include "util_misc.h"

// -----------------  LOGMSG  --------------------------------------------

void logmsg(char *lvl, const char *func, char *fmt, ...) 
{
    va_list ap;
    char    msg[1000];
    int     len;
    char    time_str[MAX_TIME_STR];
    bool    print_prefix;

    // if fmt begins with '#' then do not print the prefix
    print_prefix = (fmt[0] != '#');
    if (print_prefix == false) {
        fmt++;
    }

    // construct msg
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    len = strlen(msg);
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // log to stderr 
    if (print_prefix) {
        fprintf(stderr, "%s %s %s: %s\n",
            time2str(time_str, get_real_time_us(), false, true, true),
            lvl, func, msg);
    } else {
        fprintf(stderr, "%s\n", msg);
    }
}

// -----------------  TIME UTILS  -----------------------------------------

uint64_t microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
}

uint64_t get_real_time_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME,&ts);
    return ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
}

char * time2str(char * str, int64_t us, bool gmt, bool display_ms, bool display_date) 
{
    struct tm tm;
    time_t secs;
    int32_t cnt;
    char * s = str;

    secs = us / 1000000;

    if (gmt) {
        gmtime_r(&secs, &tm);
    } else {
        localtime_r(&secs, &tm);
    }

    if (display_date) {
        cnt = sprintf(s, "%2.2d/%2.2d/%2.2d ",
                         tm.tm_mon+1, tm.tm_mday, tm.tm_year%100);
        s += cnt;
    }

    cnt = sprintf(s, "%2.2d:%2.2d:%2.2d",
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
    s += cnt;

    if (display_ms) {
        cnt = sprintf(s, ".%3.3"PRId64, (us % 1000000) / 1000);
        s += cnt;
    }

    if (gmt) {
        strcpy(s, " GMT");
    }

    return str;
}

// -----------------  CONFIG READ / WRITE  -------------------------------

int config_read(char * config_path, config_t * config, int config_version)
{
    FILE * fp;
    int    i, version=0;
    char * name;
    char * value;
    char * saveptr;
    char   s[100] = "";

    // open config_file and verify version, 
    // if this fails then write the config file with default values
    if ((fp = fopen(config_path, "re")) == NULL ||
        fgets(s, sizeof(s), fp) == NULL ||
        sscanf(s, "VERSION %d", &version) != 1 ||
        version != config_version)
    {
        if (fp != NULL) {
            fclose(fp);
        }
        INFO("creating default config file %s, version=%d\n", config_path, config_version);
        return config_write(config_path, config, config_version);
    }

    // read config entries
    while (fgets(s, sizeof(s), fp) != NULL) {
        name = strtok_r(s, " \n", &saveptr);
        if (name == NULL || name[0] == '#') {
            continue;
        }

        value = strtok_r(NULL, " \n", &saveptr);
        if (value == NULL) {
            value = "";
        }

        for (i = 0; config[i].name[0]; i++) {
            if (strcmp(name, config[i].name) == 0) {
                strcpy(config[i].value, value);
                break;
            }
        }
    }

    // close
    fclose(fp);
    return 0;
}

int config_write(char * config_path, config_t * config, int config_version)
{
    FILE * fp;
    int    i;

    // open
    fp = fopen(config_path, "we");  // mode: truncate-or-create, close-on-exec
    if (fp == NULL) {
        ERROR("failed to write config file %s, %s\n", config_path, strerror(errno));
        return -1;
    }

    // write version
    fprintf(fp, "VERSION %d\n", config_version);

    // write name/value pairs
    for (i = 0; config[i].name[0]; i++) {
        fprintf(fp, "%-20s %s\n", config[i].name, config[i].value);
    }

    // close
    fclose(fp);
    return 0;
}

// -----------------  NETWORKING  ----------------------------------------

int getsockaddr(char * node, int port, struct sockaddr_in * ret_addr)
{
    struct addrinfo   hints;
    struct addrinfo * result;
    char              port_str[20];
    int               ret;

    sprintf(port_str, "%d", port);

    bzero(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags    = AI_NUMERICSERV;

    ret = getaddrinfo(node, port_str, &hints, &result);
    if (ret != 0) {
        ERROR("failed to get address of %s, %s\n", node, gai_strerror(ret));
        return -1;
    }
    if (result->ai_addrlen != sizeof(*ret_addr)) {
        ERROR("getaddrinfo result addrlen=%d, expected=%d\n",
            (int)result->ai_addrlen, (int)sizeof(*ret_addr));
        return -1;
    }

    *ret_addr = *(struct sockaddr_in*)result->ai_addr;
    freeaddrinfo(result);
    return 0;
}

char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
{
    char addr_str[100];
    int port;

    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET,
                  &((struct sockaddr_in*)addr)->sin_addr,
                  addr_str, sizeof(addr_str));
        port = ((struct sockaddr_in*)addr)->sin_port;
    } else if (addr->sa_family == AF_INET6) {
        inet_ntop(AF_INET6,
                  &((struct sockaddr_in6*)addr)->sin6_addr,
                 addr_str, sizeof(addr_str));
        port = ((struct sockaddr_in6*)addr)->sin6_port;
    } else {
        snprintf(s,slen,"Invalid AddrFamily %d", addr->sa_family);
        return s;
    }

    snprintf(s,slen,"%s:%d",addr_str,htons(port));
    return s;
}

int do_recv(int sockfd, void * recv_buff, size_t len)
{
    int ret;
    size_t len_remaining = len;

    while (len_remaining) {
        ret = recv(sockfd, recv_buff, len_remaining, MSG_WAITALL);
        if (ret <= 0) {
            if (ret == 0) {
                errno = ENODATA;
            }
            return -1;
        }

        len_remaining -= ret;
        recv_buff += ret;
    }

    return len;
}

int do_send(int sockfd, void * send_buff, size_t len)
{
    int ret;
    size_t len_remaining = len;

    while (len_remaining) {
        ret = send(sockfd, send_buff, len_remaining, MSG_NOSIGNAL);
        if (ret <= 0) {
            if (ret == 0) {
                errno = ENODATA;
            }
            return -1;
        }

        len_remaining -= ret;
        send_buff += ret;
    }

    return len;
}

// -----------------  RANDOM NUMBERS  ------------------------------------

// return uniformly distributed random numbers in range min to max inclusive
double random_range(double min, double max)
{
    return ((double)random() / RAND_MAX) * (max - min) + min;
}

// return triangular distributed random numbers in range min to max inclusive
// Refer to:
// - http://en.wikipedia.org/wiki/Triangular_distribution
// - http://stackoverflow.com/questions/3510475/generate-random-numbers-according-to-distributions
double random_triangular(double min, double max)
{
    double range = max - min;
    double range_squared_div_2 = range * range / 2;
    double U = (double)random() / RAND_MAX;   // 0 - 1 uniform

    if (U <= 0.5) {
        return min + sqrtf(U * range_squared_div_2);
    } else {
        return max - sqrtf((1.f - U) * range_squared_div_2);
    }
}

// returns a vector whose length equals 'magnitude' and with a random direction
void random_vector(double magnitude, double * x, double * y, double * z)
{
    double x_try, y_try, z_try, hypot, f;

    // compute x/y/z_try within a spherical shell 
    while (true) {
        x_try = random() - (RAND_MAX/2.);
        y_try = random() - (RAND_MAX/2.);
        z_try = random() - (RAND_MAX/2.);
        hypot = hypotenuse(x_try,y_try,z_try);
        if (hypot >= (RAND_MAX/10.) && hypot <= (RAND_MAX/2.)) {
            break;
        }
    }

    // scale the random vector to the caller's specified magnitude
    f = magnitude / hypot;
    *x = x_try * f;
    *y = y_try * f;
    *z = z_try * f;

#if 0
    // verification
    double magnitude_check = hypotenuse(*x, *y, *z);
    if (fabsf(magnitude_check-magnitude) > 2) {
        FATAL("magnitude=%f magnitude_check=%f, xyz=%f %f %f\n",
              magnitude, magnitude_check, *x, *y, *z);
    }
#endif
}

// -----------------  MISC MATH  -----------------------------------------

bool solve_quadratic_equation(double a, double b, double c, double *x1, double *x2)
{
    double discriminant, temp;

    discriminant = b*b - 4*a*c;
    if (discriminant < 0) {
        return false;
    }
    temp = sqrt(discriminant);
    *x1 = (-b + temp) / (2*a);
    *x2 = (-b - temp) / (2*a);
    return true;
}

double hypotenuse(double x, double y, double z)
{
    return sqrtf(x*x + y*y + z*z);
}

// -----------------  SMOOTHING  -----------------------------------------

void basic_exponential_smoothing(double x, double *s, double alpha)
{
    double s_last = *s;
    *s = alpha * x + (1 - alpha) * s_last;
}

void double_exponential_smoothing(double x, double *s, double *b, double alpha, double beta, bool init)
{
    if (init) {
        *s = x;
        *b = 0;
    } else {
        double s_last = *s;
        double b_last = *b;
        *s = alpha * x + (1 - alpha) * (s_last + b_last);
        *b = beta * (*s - s_last) + (1 - beta) * b_last;
    }
}

// -----------------  MOVING AVERAGE  ------------------------------------

double moving_average(double val, ma_t *ma)
{
    int64_t idx;

    idx = (ma->count % ma->max_values);
    ma->sum += (val - ma->values[idx]);
    ma->values[idx] = val;
    ma->count++;
    ma->current = ma->sum / (ma->count <= ma->max_values ? ma->count : ma->max_values);
    return ma->current;
}

double moving_average_query(ma_t *ma)
{
    return ma->current;
}

ma_t * moving_average_alloc(int32_t max_values)
{
    ma_t * ma;
    size_t size = sizeof(ma_t) + max_values * sizeof(ma->values[0]);

    ma = malloc(size);
    assert(ma);

    ma->max_values = max_values;

    moving_average_reset(ma);

    return ma;
}

void moving_average_free(ma_t * ma) 
{
    free(ma);
}

void moving_average_reset(ma_t * ma)
{
    ma->sum = 0;
    ma->count = 0;
    ma->current = NAN;
    memset(ma->values,0,ma->max_values*sizeof(ma->values[0]));
}

// - - - - - - - - - - - - - - - - - - - - 

double timed_moving_average(double val, double time_arg, tma_t *tma)
{
    int64_t idx, i;
    double maq;

    idx = time_arg * (tma->max_bins / tma->time_span);

    if (idx == tma->last_idx || tma->first_call) {
        tma->sum += val;
        tma->count++;
        tma->last_idx = idx;
        tma->first_call = false;
    } else if (idx > tma->last_idx) {
        for (i = tma->last_idx; i < idx; i++) {
            double v;
            if (i == idx-1 && tma->count > 0) {
                v = tma->sum / tma->count;
            } else {
                v = 0;
            }
            moving_average(v,tma->ma);
        }
        tma->sum = val;
        tma->count = 1;
        tma->last_idx = idx;
    } else {
        // time can't go backwards
        assert(0);
    }

    maq = moving_average_query(tma->ma);
    tma->current = (isnan(maq) ? tma->sum/tma->count : maq);

    return tma->current;
}

double timed_moving_average_query(tma_t *tma)
{
    return tma->current;
}

tma_t * timed_moving_average_alloc(double time_span, int64_t max_bins)
{
    tma_t * tma;

    tma = malloc(sizeof(tma_t));
    assert(tma);

    tma->time_span = time_span;
    tma->max_bins  = max_bins;
    tma->ma        = moving_average_alloc(max_bins);

    timed_moving_average_reset(tma);

    return tma;
}

void timed_moving_average_free(tma_t * tma)
{
    if (tma == NULL) {
        return;
    }

    free(tma->ma);
    free(tma);
}

void timed_moving_average_reset(tma_t * tma)
{
    tma->first_call = true;
    tma->last_idx   = 0;
    tma->sum        = 0;
    tma->count      = 0;
    tma->current    = NAN;

    moving_average_reset(tma->ma);
}


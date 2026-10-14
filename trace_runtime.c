#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define DEFAULT_MAX_EVENTS 1000000ULL
#define FLUSH_INTERVAL 4096ULL

static FILE *trace_file = NULL;
static uint64_t events_written = 0;
static uint64_t max_events = DEFAULT_MAX_EVENTS;
static int disabled = 0;
static int initialized = 0;

static void trace_close(void)
{
    if (trace_file != NULL) {
        fflush(trace_file);
        fclose(trace_file);
        trace_file = NULL;
    }
}

static void trace_init(void)
{
    if (initialized)
        return;

    initialized = 1;

    const char *path = getenv("TRACE_FILE");
    if (path == NULL || path[0] == '\0')
        path = "trace.bin";

    const char *limit = getenv("TRACE_MAX_EVENTS");
    if (limit != NULL && limit[0] != '\0') {
        errno = 0;
        char *end = NULL;
        unsigned long long parsed = strtoull(limit, &end, 10);

        if (errno == 0 && end != limit && *end == '\0')
            max_events = (uint64_t)parsed;
    }

    trace_file = fopen(path, "wb");
    if (trace_file == NULL) {
        perror("trace_runtime: fopen");
        disabled = 1;
        return;
    }

    atexit(trace_close);
}

void trace_use(uint64_t user_id, uint64_t operand_id)
{
    if (disabled)
        return;

    if (!initialized)
        trace_init();

    if (disabled || trace_file == NULL)
        return;

    if (events_written >= max_events) {
        disabled = 1;
        trace_close();
        return;
    }

    /* Каждое событие занимает 16 байт: два uint64_t. */
    if (fwrite(&user_id, sizeof(user_id), 1, trace_file) != 1 ||
        fwrite(&operand_id, sizeof(operand_id), 1, trace_file) != 1) {
        perror("trace_runtime: fwrite");
        disabled = 1;
        trace_close();
        return;
    }

    events_written++;

    if (events_written % FLUSH_INTERVAL == 0) {
        if (fflush(trace_file) != 0) {
            perror("trace_runtime: fflush");
            disabled = 1;
            trace_close();
        }
    }

    if (events_written >= max_events) {
        disabled = 1;
        trace_close();
    }
}

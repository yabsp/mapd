#include "analyzer.h"

#define SOCKET_PATH "/tmp/mapd_socket"

static int client_counter = 0;
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
AnalyzerOptions* analyzer_options = NULL;

/**
 * server_socket_thread:
 *
 * Listens for incoming client connections via UNIX domain socket.Creates a new detached thread to handle communication
 * for each client and runs indefinitely.
 *
 * @param arg Unused
 * @return NULL when thread exits
 */
static void* server_socket_thread(void* arg)
{
    (void)arg;

    int server_fd;
    struct sockaddr_un addr;

    // Create UNIX domain socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket"); exit(EXIT_FAILURE);
    }

    // Remove any previous socket file
    unlink(SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Bind socket to the filesystem path
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind"); exit(EXIT_FAILURE);
    }

    listen(server_fd, 10);
    printf("[Analyzer] Listening on %s\n", SOCKET_PATH);

    // Main accept loop
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        // Allocate context for new client
        ClientContext* ctx = malloc(sizeof(ClientContext));
        if (!ctx) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        ctx->client_fd = client_fd;

        // Assign unique client number (thread-safe)
        pthread_mutex_lock(&counter_lock);
        ctx->client_number = ++client_counter;
        pthread_mutex_unlock(&counter_lock);

        // Spawn detached thread to handle client communication
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, ctx) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(ctx);
        } else {
            ctx->thread_id = tid;
            pthread_detach(tid);
        }
    }
}

/**
 * analyzer_init - Starts all analyzer background threads.
 *
 * Initializes both the fragmentation monitoring and the main server socket thread that handles client connections.
 *
 * @param options Pointer to options of Analyzer
 */
void analyzer_init(AnalyzerOptions* options)
{
    analyzer_options = options;

    pthread_t frag_thread;
    pthread_create(&frag_thread, NULL, fragmentation_thread, NULL);
    pthread_detach(frag_thread);

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_socket_thread, NULL);
    pthread_detach(server_thread);
}

/**
 * handle_client:
 *
 * Handles communication with a single connected client. Receives JSON messages from the client, parses them, and
 * enqueues them for processing.
 *
 * @param arg: Pointer to ClientContext containing connection information.
 * @return: NULL when thread exits.
 */
void* handle_client(void* arg) {
    ClientContext* ctx = (ClientContext*)arg;
    char buffer[1024];
    ssize_t bytes_read;

    printf("[Analyzer] New connection: Client #%d (fd = %d)\n",
        ctx->client_number, ctx->client_fd);
    create_connection_message(ctx->client_number, "connected");

    // Main receive loop
    while ((bytes_read = read(ctx->client_fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';

        // Process each line
        char* line = strtok(buffer, "\n");
        while (line != NULL)
        {
            if (strlen(line) == 0 || line[0] != '{')
            {
                line = strtok(NULL, "\n");
                continue;
            }

            // Parse JSON message and enqueue for processing
            Message msg = parse_json_to_message(line, ctx->client_number);

            if (analyzer_options != NULL && analyzer_options->info_logs_enabled == 0)
            {
                if (strcmp(msg.type, "malloc") == 0 ||
                    strcmp(msg.type, "free") == 0 ||
                    strcmp(msg.type, "realloc") == 0 ||
                    strlen(msg.type) == 0)
                {
                    // Suppress message entirely
                    line = strtok(NULL, "\n");
                    continue;
                }
            }

            enqueue_message(&msg);
            line = strtok(NULL, "\n");
        }
    }

    printf("[Analyzer] Client #%d disconnected.\n", ctx->client_number);
    create_connection_message(ctx->client_number, "disconnected");

    // Clean
    close(ctx->client_fd);
    free(ctx);
    return NULL;
}

/**
 * gui_consumer_thread:
 *
 * Dequeues messages from the global message queue and prints them to the console. Prints depending on message type.
 *
 * @param arg: Unused
 * @return NULL when thread exits
 */
void* gui_consumer_thread(void* arg)
{
    (void)arg;

    while (1)
    {
        Message msg = dequeue_message();

        // Handle connection messages
        if (strcmp(msg.type, "connection") == 0)
        {
            printf("[GUI] %s\n", msg.description);
            continue;
        }

        // Handle fragmentation messages
        if (strcmp(msg.type, "fragmentation") == 0)
        {
            printf("[GUI] FRAG | %s\n", msg.description);
            continue;
        }

        // Format time into human readable string
        char time_buf[32];
        struct tm *tm_info = localtime(&msg.timestamp);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

        // Always print remaining messages
        printf("[GUI] Client %d | %-12s | Addr: %-12s | Size: %-5zu | Thread: %lu | Time: %s\n",
            msg.client_id, msg.type, msg.addr, msg.size, msg.thread, time_buf);
    }
    return NULL;
}
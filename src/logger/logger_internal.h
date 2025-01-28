
#define MAX_LOG_MESSAGE_SIZE 120
#define MAX_TAG_SIZE 10

typedef struct {
    int log_timestamp_s;
    int log_timestamp_ns;
    int log_level;
    int pid;
    char log_tag[MAX_TAG_SIZE + 1];
    char log_message[MAX_LOG_MESSAGE_SIZE + 1];
} LogMessage;

typedef struct {
    int write;
    int read;
} Header;

void allocate();

void write_log(LogMessage* log_message);

LogMessage* read_log();

void deallocate_client();

void deallocate_server();
#include "logger.h"

#include<stdio.h>
#include<stdlib.h>

void handle_sigint(int singal) {
    close_logger();
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);
    init_logger();

    while (1) {
        consume_log_message();
    }
    close_logger();
    return 0;
}
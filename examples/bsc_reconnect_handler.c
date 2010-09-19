/**
 * =====================================================================================
 * @file   bsc_reconnect_handler.c
 * @brief  
 * @date   09/19/2010 05:37:25 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <beanstalkclient.h>

char spawn_cmd[200], kill_cmd[200], *host = "localhost", *port = "16666", errorstr[BSC_ERRSTR_LEN];
static bsc_error_t bsc_error;
static int reconnect_attempts = 5, finished = 0, put_finished;

void put_cb(bsc *client, struct bsc_put_info *info)
{
    put_finished = true;
}

int client_poll(bsc *client, fd_set *readset, fd_set *writeset)
{
    FD_SET(client->fd, readset);
    FD_SET(client->fd, writeset);
    if (AQ_EMPTY(client->outq)) {
        if ( select(client->fd+1, readset, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "critical error: select()");
            return EXIT_FAILURE;
        }
        if (FD_ISSET(client->fd, readset))
            bsc_read(client);
    }
    else {
        if ( select(client->fd+1, readset, writeset, NULL, NULL) < 0) {
            fprintf(stderr, "critical error: select()");
            return EXIT_FAILURE;
        }
        if (FD_ISSET(client->fd, readset))
            bsc_read(client);
        if (FD_ISSET(client->fd, writeset))
            bsc_write(client);
    }
}

void spawn_put_client()
{
    fd_set readset, writeset;

    bsc *put_client;
    put_client = bsc_new(host, port, "baba", NULL, 16, 12, 4, errorstr);
    if(!put_client) {
        fprintf(stderr, "bsc_new: %s", errorstr);
        exit(EXIT_FAILURE);
    }

    bsc_error = bsc_put(put_client, put_cb, NULL, 1, 0, 10, strlen("baba"), "baba", false);

    if (bsc_error != BSC_ERROR_NONE) {
        fprintf(stderr, "bsc_put failed (%d)", bsc_error );
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!put_finished)
        client_poll(put_client, &readset, &writeset);
}

void ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    system(kill_cmd);
}

void reserve_cb(bsc *client, struct bsc_reserve_info *info)
{
    printf("%s\n", "got reserve cb");
    finished = true;
}

static void reconnect(bsc *client, bsc_error_t error)
{
    char errorstr[BSC_ERRSTR_LEN];
    int i;

    system(spawn_cmd);
    spawn_put_client();

    switch (error) {
        case BSC_ERROR_INTERNAL:
            fprintf(stderr, "critical error: recieved BSC_ERROR_INTERNAL, quitting\n");
            break;
        case BSC_ERROR_MEMORY:
            fprintf(stderr, "critical error: recieved BSC_ERROR_MEMORY, quitting\n");
            break;
        case BSC_ERROR_SOCKET:
            fprintf(stderr, "error: recieved BSC_ERROR_SOCKET, attempting to reconnect ...\n");
            for (i = 0; i < reconnect_attempts; ++i) {
                if ( bsc_reconnect(client, errorstr) ) {
                    printf("reconnect successful\n");
                    return;
                }
                else
                    fprintf(stderr, "error: reconnect attempt %d/%d - %s", i+1, reconnect_attempts, errorstr);
            }
            fprintf(stderr, "critical error: maxed out reconnect attempts, quitting\n");
            break;
        default:
            fprintf(stderr, "critical error: got unknown error (%d)\n", error);
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    bsc *client;
    fd_set readset, writeset;

    sprintf(spawn_cmd, "beanstalkd -p %s -d", port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd |grep '%s'| gawk '!/grep/ {print $2}'|xargs kill", port);

    system(kill_cmd);
    system(spawn_cmd);

    client = bsc_new(host, port, "baba", reconnect, 16, 12, 4, errorstr);
    if(!client) {
        fprintf(stderr, "bsc_new: %s", errorstr);
        return EXIT_FAILURE;
    }

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    if (bsc_error != BSC_ERROR_NONE)
        fprintf(stderr, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_reserve(client, reserve_cb, NULL, BSC_RESERVE_NO_TIMEOUT);
    if (bsc_error != BSC_ERROR_NONE)
        fprintf(stderr, "bsc_reserve failed (%d)", bsc_error );

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!finished) {
        if (client_poll(client, &readset, &writeset) == EXIT_FAILURE)
            return EXIT_FAILURE;
    }

    system(kill_cmd);
    bsc_free(client);
}

#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params, char *cookie, char *jwt)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (jwt != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt);
        compute_message(message, line);
    }

    if (cookie != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }

    compute_message(message, "");
    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params, char *cookie, char *jwt)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (jwt != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt);
        compute_message(message, line);
    }

    if (cookie != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }

    compute_message(message, "");
    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char *content_type, char **body_data, int body_data_fields_count, char **cookies, int cookies_count, char *jwt) {

    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));
    for (int i = 0; i < body_data_fields_count; i++) {
        strcat(body_data_buffer, *(body_data + i));
    }

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    if (jwt != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt);
        compute_message(message, line);
    }
    while (cookies_count) {
        if (*cookies != NULL) {
            memset(line, 0, LINELEN);
            sprintf(line, "Cookie: %s", *cookies);
            compute_message(message, line);
        }
    }

    compute_message(message, "");
    compute_message(message, body_data_buffer);
    compute_message(message, "");

    free(body_data_buffer);
    free(line);
    return message;
}

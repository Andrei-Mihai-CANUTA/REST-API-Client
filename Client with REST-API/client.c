#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

//function to remove end of line
char *removeEOL(char *line) {
    line = strtok(line, "\n");
    return line;
}

//check if the server returns timeout
char *checkTimeout (char *response) {
    char *aux = strstr(response, "Too many requests");
    return aux;
}

//check if the server returns errors and prints them
//it asumes that the server returns a json
int checkError(char *response) {
    char *aux = basic_extract_json_response(response);
    if(aux) {

        JSON_Value *value = json_parse_string(aux);
        JSON_Object *object = json_value_get_object(value);

        if(json_object_get_string(object, "error")){
            printf("%s\n\n", json_object_get_string(object, "error"));
            return 1;
        } else {
            return 0;
        }

    } else {
        return 0;
    }
}

//register
void post_register(char *username, char *password) {
    
    //create a json obj
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);

    //populate the json with username and password
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);

    //make the json as payload
    char *payload = json_serialize_to_string(value);

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return;
    }
    
    //send message to server
    char *message = compute_post_request(HOST, URL"/auth/register", "application/json", &payload, 1, NULL, 0, NULL);
    send_to_server(sockfd, message);

    //recieve from server;
    char *response = receive_from_server(sockfd);
    
    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return;
    }

    //check errors
    if(checkError(response)) {
        return;
    } 

    //if no err
    printf("Register succesful! Now you can login on %s\n\n", username);

    //close the connection
    close_connection(sockfd);
}

//login
char *post_login(char *username, char *password) {

    //create a json obj
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);

    //populate the json with username and password
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);

    //make the json as payload
    char *payload = json_serialize_to_string(value);

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return 0;
    }
    
    //send message to server
    char *message = compute_post_request(HOST, URL"/auth/login", "application/json", &payload, 1, NULL, 0, NULL);
    send_to_server(sockfd, message);

    //recieve from server;
    char *response = receive_from_server(sockfd);
    
    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return 0;
    }

    //check errors
    if(checkError(response)) {
        return 0;
    }

    //get the autentification cookie
    char *cookie = strstr(response, "connect.sid=");
    cookie = strtok(cookie, ";");


    //if no err
    printf("Login succesful! Welcome, %s!\n\n", username);

    //close the connection
    close_connection(sockfd);
    return cookie;
}
//enter_library
char *get_enter_library(char *cookie) {

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return NULL;
    }

    //send a message containing the login cookie to server
    char *message = compute_get_request(HOST, URL"/library/access", NULL, cookie, NULL);
    send_to_server(sockfd, message);
    
    //get the response
    char *response = receive_from_server(sockfd);

    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return NULL;
    }

    //check errors
    if(checkError(response)) {
        return NULL;
    }

    //if no err or timeout, get the token 
    JSON_Value *value = json_parse_string(basic_extract_json_response(response));
    JSON_Object *object = json_value_get_object(value);
    char *jwt = (char *)json_object_get_string(object, "token");

    //close conection and return the jws token
    printf("Library access gained!\n\n");
    close_connection(sockfd);
    return jwt;
}

//get_books
void get_books(char *cookie, char *jwt) {

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return ;
    }

    //send a message containing the login cookie and the jwt token to the server
    char *message = compute_get_request(HOST, URL"/library/books", NULL, cookie, jwt);
    send_to_server(sockfd, message);
    
    //get the response
    char *response = receive_from_server(sockfd);

    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return;
    }

    //check errors
    if(checkError(response)) {
        return;
    }

    //check for books
    char *noBook = strstr(response, "[]");
    if(noBook) {
        printf("No books here!\n\n");
        return;
    }

    //get the returning json containing the books,
    //pretty print it and close the connection
    JSON_Value *value = json_parse_string(strstr(response, "["));
    printf("%s\n\n", json_serialize_to_string_pretty(value));
    close_connection(sockfd);
}

//get_book
void get_book(char *cookie, char *jwt, char *bookId) {

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return ;
    }
    
    //create the URL
    char bookURL[100] = "";
    strcat(bookURL, "/api/v1/tema/library/books/");
    strcat(bookURL, bookId);

    //send a message containing the login cookie and the jwt token to the server
    char *message = compute_get_request(HOST, bookURL, NULL, cookie, jwt);
    send_to_server(sockfd, message);
    
    //get the response
    char *response = receive_from_server(sockfd);

    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return;
    }

    //check errors
    if(checkError(response)) {
        return;
    }

    //get teh json containing the book with the given id,
    //pretty print it and close connection
    JSON_Value *value = json_parse_string(basic_extract_json_response(response));
    printf("%s\n\n", json_serialize_to_string_pretty(value));
    close_connection(sockfd);
}

//add_book
void post_add_book(char *cookie, char *jwt, char *title, char *author, char *genre, char *page_count, char *publisher) {

    //create a json obj
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);

    //populate the json with the book info
    json_object_set_string(object, "title", title);
    json_object_set_string(object, "author", author);
    json_object_set_string(object, "genre", genre);
    json_object_set_string(object, "page_count", page_count);
    json_object_set_string(object, "publisher", publisher);

    //make the json as payload
    char *payload = json_serialize_to_string(value);

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return;
    }
    
    //send a message containing the login cookie and the jwt token to the server
    char *message = compute_post_request(HOST, URL"/library/books", "application/json", &payload, 1, &cookie, 0, jwt);
    send_to_server(sockfd, message);

    //recieve from server;
    char *response = receive_from_server(sockfd);
    
    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return;
    }

    //check errors
    if(checkError(response)) {
        return;

    }

    //print a success mesage and close connection
    printf("Book added to library!\n\n");
    close_connection(sockfd);
}

//delete_book
void delete_book(char *cookie, char *jwt, char *bookId) {

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return ;
    }
    
    //create the URL
    char bookURL[100] = "";
    strcat(bookURL, "/api/v1/tema/library/books/");
    strcat(bookURL, bookId);

    //send a message containing the login cookie and the jwt token to the server
    char *message = compute_delete_request(HOST, bookURL, NULL, cookie, jwt);
    send_to_server(sockfd, message);
    
    //get the response
    char *response = receive_from_server(sockfd);

    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return;
    }

    //check errors
    if(checkError(response)) {
        return;
    }

    //print a success message and close connection
    printf("Book deleted from library!\n\n");
    close_connection(sockfd);
}

//logout
int get_logout(char *cookie, char *jwt) {

    //open connection
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    //check if the connection was succesful
    if(sockfd < 0) {
        return 0;
    }

    //send a message containing the login cookie and the jwt token to the server
    char *message = compute_get_request(HOST, URL"/auth/logout", NULL, cookie, jwt);
    send_to_server(sockfd, message);
    
    //get the response
    char *response = receive_from_server(sockfd);

    //check timeout
    char *to = checkTimeout(response);
    if(to) {
        printf("%s\n\n", to);
        return 0;
    }

    //check errors
    if(checkError(response)) {
        return 0;
    }

    //print a success message and close connection
    printf("Logout succesful! Goodbye!\n\n");
    close_connection(sockfd);
    return 1;
}

int main(int argc, char *argv[]){
    
    char comand[50];
    char *cookie = NULL;
    char *jwt = NULL;

    printf("Welcome to the client interface!\nPlease, enter your command!\n\n");
    
    while(1) {
        
        //get command
        fgets(comand, 50, stdin);
        removeEOL(comand);

        //check command
        if(strncmp(comand, "login", 5) == 0) {
            
            //if already loged in
            if(cookie != NULL) {
                printf("You are already logged in!\n\n");
            } else {

                
                char username[50];
                char password[50];

                //get user info
                printf("username=");
                fgets(username, 50, stdin);
                removeEOL(username);
                printf("password=");
                fgets(password, 50, stdin);
                removeEOL(password);

                //get login cookie
                cookie = post_login(username, password);
            }

        }
        else if(strncmp(comand, "register", 8) == 0) {

            char username[50];
            char password[50];

            //get user info
            printf("username=");
            fgets(username, 50, stdin);
            removeEOL(username);
            printf("password=");
            fgets(password, 50, stdin);
            removeEOL(password);

            post_register(username, password);

        }

        else if(strncmp(comand, "logout", 6) == 0) {
            if(get_logout(cookie, jwt)) {
                //flush cookie and token
                cookie = NULL;
                jwt = NULL;
            }
        }

        else if(strncmp(comand, "enter_library", 13) == 0) {
            jwt = get_enter_library(cookie);
        }

        else if(strncmp(comand, "get_books", 9) == 0) {
            get_books(cookie, jwt);
        }

        else if(strncmp(comand, "get_book", 8) == 0) {
            char bookId[20];
            //get book id
            printf("id=");
            fgets(bookId, 50, stdin);
            removeEOL(bookId);
            get_book(cookie, jwt, bookId);
        }

        else if(strncmp(comand, "add_book", 8) == 0) {
            
            char title[50];
            char author[100];
            char genre[100];
            char publisher[100];
            char page_count[100];
            
            //get book info
            printf("title=");
            fgets(title, 100, stdin);
            removeEOL(title);
            printf("author=");
            fgets(author, 100, stdin);
            removeEOL(author);
            printf("genre=");
            fgets(genre, 100, stdin);
            removeEOL(genre);
            printf("publisher=");
            fgets(publisher, 100, stdin);
            removeEOL(publisher);
            printf("page_count=");
            fgets(page_count, 100, stdin);
            removeEOL(page_count);

            //check if page_count can be an int
            int aux = 1;
            for(int i = 0; i < strlen(page_count); i++) {
                if(page_count[i] < '0' || page_count[i] > '9') {
                    aux = 0;
                    break;
                }
            }

            if(aux) {
                post_add_book(cookie, jwt, title, author, genre, page_count, publisher);
            } else {
                printf("Wrong input, page_count should be int!!\n\n");
            }

        }

        else if(strncmp(comand, "delete_book", 11) == 0) {
            char bookId[20];
            //get book id
            printf("id=");
            fgets(bookId, 50, stdin);
            removeEOL(bookId);
            delete_book(cookie, jwt, bookId);
        }

        else if(strncmp(comand, "exit", 4) == 0) {
            break;
        }     

        else {
            printf("Invalid (%s) comand!\n\n", comand);
        }
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 20000
#define BUF_SIZE 100

void encrypt_text(char *buffer, char *key) {
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (isalpha(buffer[i])) {
            if (isupper(buffer[i])) {
                buffer[i] = key[buffer[i] - 'A'];
            } else {
                buffer[i] = tolower(key[buffer[i] - 'a']);
            }
        }
    }
}

int main() {
    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[BUF_SIZE];
    char key[27];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.127.127.1");
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    listen(sockfd, 5);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) { // Child process
            close(sockfd);
            char client_filename[50];
            snprintf(client_filename, 50, "%s.%d.txt", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

            recv(newsockfd, key, 27, 0);

            FILE *temp = fopen(client_filename, "w");
            if (!temp) {
                perror("File creation failed");
                close(newsockfd);
                exit(1);
            }

            while (1) {
                int bytes_received = recv(newsockfd, buffer, BUF_SIZE - 1, 0);
                if (bytes_received <= 0) break;
                buffer[bytes_received] = '\0'; // Null-terminate the buffer
                fputs(buffer, temp);
            }
            fclose(temp);

            FILE *input = fopen(client_filename, "r");
            char enc_filename[60];
            snprintf(enc_filename, 60, "%s.enc", client_filename);
            FILE *output = fopen(enc_filename, "w");

            while (fgets(buffer, BUF_SIZE, input)) {
                encrypt_text(buffer, key);
                fputs(buffer, output);
            }
            fclose(input);
            fclose(output);

            FILE *enc_file = fopen(enc_filename, "r");
            while (fgets(buffer, BUF_SIZE, enc_file)) {
                send(newsockfd, buffer, strlen(buffer), 0);
            }
            fclose(enc_file);

            close(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }
    return 0;
}

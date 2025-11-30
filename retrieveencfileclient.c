#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 20000
#define BUF_SIZE 100

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE], key[27], filename[100], enc_filename[100];
    FILE *file;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    inet_aton("127.127.127.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Cannot connect to server");
        exit(1);
    }

    while (1) {
        printf("Enter filename: ");
        scanf("%s", filename);

        file = fopen(filename, "r");
        while (!file) {
            printf("NOTFOUND %s\nEnter a valid filename: ", filename);
            scanf("%s", filename);
            file = fopen(filename, "r");
        }

        printf("Enter encryption key (26 characters): ");
        scanf("%s", key);
        while (strlen(key) < 26) {
            printf("Invalid key. Enter again: ");
            scanf("%s", key);
        }

        send(sockfd, key, 27, 0);

        while (fgets(buffer, BUF_SIZE, file)) {
            send(sockfd, buffer, strlen(buffer), 0);
        }
        fclose(file);
        shutdown(sockfd, SHUT_WR);

        snprintf(enc_filename, sizeof(enc_filename), "%s.enc", filename);

        FILE *enc_file = fopen(enc_filename, "w");

        while (1) {
            int bytes_received = recv(sockfd, buffer, BUF_SIZE - 1, 0);
            if (bytes_received <= 0) break;
            buffer[bytes_received] = '\0'; // Null-terminate the buffer
            fputs(buffer, enc_file);
        }
        fclose(enc_file);

        printf("File encrypted: %s -> %s\n", filename, enc_filename);

        printf("Do you want to encrypt another file? (yes/no): ");
        scanf("%s", buffer);
        if (strcmp(buffer, "no") == 0) {
            break;
        }
        
        // Reconnect for the next file
        close(sockfd);
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Cannot create socket");
            exit(1);
        }
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Cannot connect to server");
            exit(1);
        }
    }
    close(sockfd);
    return 0;
}

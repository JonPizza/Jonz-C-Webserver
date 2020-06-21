#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

#define PORT 6969

char *headers_fmt = "Server: Jon'z Lit Server -- Written with C and pain\n"
		    "Content-Type: %s; charset=UTF-8\n"
		    "Content-Security-Policy: script-src self\n"
		    "Content-Length: %d\r\n\r\n";

char *page_404 = "<!DOCTYPE html>"
		 "<html><head><title>404 Not Found</title></head><body>"
		 "<h1>404 Not Found</h1><hr><b>JON'Z LIT SERVER // v0.0.1</b>"
		 "</body></html>\n";

char *webroot = "/var/www/jonz/";


char *read_file(const char *filename) {
    long int size = 0;
    FILE *file = fopen(filename, "r");

    if(!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    char *result = (char *) malloc(size);
    if(!result) {
        fputs("Memory error.\n", stderr);
        return NULL;
    }

    if(fread(result, 1, size, file) != size) {
        fputs("Read error.\n", stderr);
        return NULL;
    }

    fclose(file);
    return result;
}


int file_exists(char *filename) {
	FILE *file = fopen(filename, "r");

	if(!file) {
		return 1;
	}

	return 0;
}


void *preprocess_filename(char *filename) {
	int i = 0;
	int filename_len = strlen(filename);
	char queue[3];
	char temp;
	
	while (strstr(filename, "../") != NULL) {
		queue[0] = queue[1];	
		queue[1] = queue[2];
		queue[2] = filename[i];
		queue[3] = 0;
			
		if (strcmp(queue, "../") == 0) {
			filename[i - 1] = '/';
			filename[i - 2] = '/';
		}

		i++;

		if (i > filename_len) {
			i = 0;
		}
	}
}


long int get_content_len(char *filename) {
	long int size = 0;
	FILE *file = fopen(filename, "r");

	fseek(file, 0, SEEK_END);
	size = ftell(file);

	return size;
}


char *concat(char *s1, char *s2) {
	char *res;
	res = malloc(strlen(s1) + strlen(s2) + 1);

	if (!res) {
		printf("Malloc failed");
		exit(0);
	}

	strcpy(res, s1);
	strcat(res, s2);
	return res;
}


char *parse_filename_from_request(char *request) {
	char ch;
	int j = 0;
	int i = 0;
	char *buf = (char *)malloc(sizeof(char) * 128);
	
	for (j; j < strlen(webroot); j++) {
		buf[j] = webroot[j];
	}

	while (request[i] != ' ') i++;
	i += 2;

	while (request[i] != ' ' && request[i] != '?') {
		buf[j] = request[i];
		j++;
		i++;
	}
		
	return buf;
}


int ends_with(char *filename, char *ext) {
	if (strlen(filename) >= strlen(ext)) {
		if (strcmp(filename + strlen(filename) - strlen(ext), ext) == 0) {
			return 1;
		}
	}
	return 0;
}	


char *get_mime_type_from_filename(char *filename) {
	if (ends_with(filename, ".html")) {
		return "text/html";
	} else if (ends_with(filename, ".css")) {
		return "text/css";
	} else if (ends_with(filename, ".js")) {
		return "text/javascript";
	} else if (ends_with(filename, ".bmp")) {
		return "image/bmp";
	} else if (ends_with(filename, ".png")) {
                return "image/png";
        } else if (ends_with(filename, ".ico")) {
                return "image/x-icon";
        } else if (ends_with(filename, ".jpg") || ends_with(filename, ".jpeg")) {
                return "image/jpeg";
        } else if (ends_with(filename, ".svg")) {
                return "image/svg+xml";
        } else if (ends_with(filename, ".wav")) {
                return "audio/wave";
        } else if (ends_with(filename, ".xml")) {
                return "text/xml";
        } else {
		return "text/plain";
	}
}


char *get_status_text(int status_code) {
	if (status_code == 200) {
		return "OK";
	} else if (status_code == 302) {
		return "Found";	
	} else if (status_code == 404) {
		return "Not Found";
	} else if (status_code == 418) {
		return "I'm a teapot";
	}
}


char *generate_response(char *filename) {
	int status_code;
	long int size;
	char *contents;

	if (file_exists(filename)) {
		status_code = 404;	
	} else {
		status_code = 200;
	}

	if (status_code == 404) {
		contents = page_404;
	        size = strlen(page_404);
	} else {
		contents = read_file(filename);
		size = get_content_len(filename);
	}

	char *status_text = get_status_text(status_code);

	char line_one_buffer[64];
	snprintf(line_one_buffer, 64, "HTTP/1.1 %d %s\n", status_code, status_text);
	
	char headers_buffer[512];
	snprintf(headers_buffer, 512, headers_fmt, get_mime_type_from_filename(filename), size);

	char *head = concat(line_one_buffer, headers_buffer);
	char *response = concat(head, contents);
	return response;
}


int main(int argc, char *argv[]) { 
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1;
	int addrlen = sizeof(address); 
	char buffer[16384] = {0};

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	bind(server_fd, (struct sockaddr *)&address, sizeof(address));
	listen(server_fd, 3);

	while (1) {	
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		
		valread = read(new_socket, buffer, 16384); 
		printf("%s\n", buffer);
		char *fname = parse_filename_from_request(buffer);
		preprocess_filename(fname);
			
		if (strcmp(fname, webroot) == 0) {
			fname = concat(webroot, "index.html");
		}

		char *response = generate_response(fname);
		send(new_socket, response, strlen(response), 0);
		
		free(response);
		free(fname);

		shutdown(new_socket, 2);
	}

	return 0;
}


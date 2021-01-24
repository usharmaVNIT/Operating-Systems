#include "io_helper.h"
#include "request.h"

#define MAXBUF (8192)

pthread_mutex_t lock; // Lock for adding and removing
pthread_cond_t put = PTHREAD_COND_INITIALIZER; //Condition Variable
pthread_cond_t get = PTHREAD_COND_INITIALIZER; //Condition Variable


//
//	TODO: add code to create and manage the buffer
//

// An object to store request
typedef struct {
  int fd_id;
  int filesize;
  char name[MAXBUF];
}buffer_object;

// A buffer structure
typedef struct {
  int number;
  buffer_object buffer[100]; 
  int front;
  int last;
} BUFFER;

BUFFER final_buffer; // Statically initialized and will further be initialised while entering...
 // Adding in the buffer in FIFO manner
void addinBufferFIFO(BUFFER *buff,int fd,char *filename, int filesize){
  buff->last = buff->last + 1; // Increasing The last Pointer
  int posn = buff->last%buffer_max_size; // This is the position where we have to add
  buff->buffer[posn].fd_id = fd; // Simply adding in the queue
  buff->buffer[posn].filesize = filesize;
  strcpy(buff->buffer[posn].name,filename);
  buff->number+=1; //Incrementing the count
  buffer_size++; // Condition check
  if(buff->number==1){ // If it is the first element then 
    buff->front = buff->last;
  }
  // printf("\n** Added to Buffer FIFO: position = %d and size is %d  **\n\n",buff->last,filesize);
  printf("Request for %s is added to the buffer.\n", filename);

}
//Removing From The Buffer in FIFO Manner
void removefromBuffer(BUFFER *buff,int *fd,int *size,char *name){
  int posn = buff->front%buffer_max_size; // This is the position where we have to remove
  *fd = buff->buffer[posn].fd_id; //Simply storing the values
  *size = buff->buffer[posn].filesize;
  strcpy(name,buff->buffer[posn].name);
  buff->front = buff->front + 1; // Increasing The first Pointer
  buff->number-=1; // Decrementing The count
  buffer_size--;
  // printf("\n** Removed From Buffer: position = %d and size is %d **\n\n",buff->front-1,*size);
  printf("Request for %s is removed from the buffer.\n", name);
  if(buff->number==0){
    buff->last = buff->front = -1;
  }
}

//Adding in the buffer in SSF manner
void addinBufferSSF(BUFFER *buff,int fd,char *filename, int filesize){
  if(buff->number==0){ // If no elements in buffer
    buff->last = buff->front = 0; //Set them to 0 as they might be at different positions
    buff->buffer[0].fd_id = fd; // Setting the values
    buff->buffer[0].filesize = filesize;
    strcpy(buff->buffer[0].name,filename);
    buff->number+=1; // Incrementing the count
    buffer_size++;
    // printf("\n** Added to Buffer SSF: position = %d and size is %d  **\n\n",buff->last,filesize);
    printf("Request for %s is added to the buffer.\n", filename);
    return;
  }
  int ctr = buff->front; // setting the counter to the starting of buffer
  // Finding the index which has filesize > request's filesize
  while(ctr<=buff->last && filesize>buff->buffer[ctr%buffer_max_size].filesize){
    ctr++; // The logic ctr%buffer_max_size keeps the index under bounds
  }
  buff->last+=1; // Incrementing the buffer last pointer
  int nctr = buff->last;
  while(nctr>ctr){
    // Shifting the values ahead 1 index till ctr as  at ctr we have to insert
    buff->buffer[nctr%buffer_max_size].fd_id = buff->buffer[(nctr-1)%buffer_max_size].fd_id;
    buff->buffer[nctr%buffer_max_size].filesize = buff->buffer[(nctr-1)%buffer_max_size].filesize;
    strcpy(buff->buffer[nctr%buffer_max_size].name,buff->buffer[(nctr-1)%buffer_max_size].name);
    nctr--;
  }
  // Inserting at position ctr%buffer_max_size
  buff->buffer[ctr%buffer_max_size].fd_id = fd;
  buff->buffer[ctr%buffer_max_size].filesize = filesize;
  strcpy(buff->buffer[ctr%buffer_max_size].name,filename);
  buff->number+=1; // Incrementing The size
  buffer_size+=1;
  // printf("\n** Added to Buffer SSF: position = %d and size is %d  **\n\n",ctr%buffer_max_size,filesize);
  printf("Request for %s is added to the buffer.\n", filename);
}



//
// Sends out HTTP response in case of errors
//
void request_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXBUF], body[MAXBUF];
    
    // Create the body of error message first (have to know its length for header)
    sprintf(body, ""
	    "<!doctype html>\r\n"
	    "<head>\r\n"
	    "  <title>OSTEP WebServer Error</title>\r\n"
	    "</head>\r\n"
	    "<body>\r\n"
	    "  <h2>%s: %s</h2>\r\n" 
	    "  <p>%s: %s</p>\r\n"
	    "</body>\r\n"
	    "</html>\r\n", errnum, shortmsg, longmsg, cause);
    
    // Write out the header information for this response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Type: text/html\r\n");
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Length: %lu\r\n\r\n", strlen(body));
    write_or_die(fd, buf, strlen(buf));
    
    // Write out the body last
    write_or_die(fd, body, strlen(body));
    
    // close the socket connection
    close_or_die(fd);
}

//
// Reads and discards everything up to an empty text line
//
void request_read_headers(int fd) {
    char buf[MAXBUF];
    
    readline_or_die(fd, buf, MAXBUF);
    while (strcmp(buf, "\r\n")) {
		readline_or_die(fd, buf, MAXBUF);
    }
    return;
}

//
// Return 1 if static, 0 if dynamic content (executable file)
// Calculates filename (and cgiargs, for dynamic) from uri
//
int request_parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;
    
    if (!strstr(uri, "cgi")) { 
	// static
	strcpy(cgiargs, "");
	sprintf(filename, ".%s", uri);
	if (uri[strlen(uri)-1] == '/') {
	    strcat(filename, "index.html");
	}
	return 1;
    } else { 
	// dynamic
	ptr = index(uri, '?');
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	} else {
	    strcpy(cgiargs, "");
	}
	sprintf(filename, ".%s", uri);
	return 0;
    }
}

//
// Fills in the filetype given the filename
//
void request_get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html")) 
		strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif")) 
		strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg")) 
		strcpy(filetype, "image/jpeg");
    else 
		strcpy(filetype, "text/plain");
}

//
// Handles requests for static content
//
void request_serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXBUF], buf[MAXBUF];
    
    request_get_filetype(filename, filetype);
    srcfd = open_or_die(filename, O_RDONLY, 0);
    
    // Rather than call read() to read the file into memory, 
    // which would require that we allocate a buffer, we memory-map the file
    srcp = mmap_or_die(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close_or_die(srcfd);
    
    // put together response
    sprintf(buf, ""
	    "HTTP/1.0 200 OK\r\n"
	    "Server: OSTEP WebServer\r\n"
	    "Content-Length: %d\r\n"
	    "Content-Type: %s\r\n\r\n", 
	    filesize, filetype);
       
    write_or_die(fd, buf, strlen(buf));
    
    //  Writes out to the client socket the memory-mapped file 
    write_or_die(fd, srcp, filesize);
    munmap_or_die(srcp, filesize);
}

//
// Fetches the requests from the buffer and handles them (thread locic)
//
void* thread_request_serve_static(void* arg)
{
	// TODO: write code to actualy respond to HTTP requests

  // Note I have made it as an infinite loop as once these are called the should not stop
  while(1){
    //Aquiring the lock
    pthread_mutex_lock(&lock);
    while(buffer_size==0){ // If the buffer is empty then wait till it fills ...
      pthread_cond_wait(&put, &lock); // Waiting on put as the main thread will notify
      //that it has putted a request in buffer
    }
    buffer_object nbo;
    // Finally removing the request values
    removefromBuffer(&final_buffer,&(nbo.fd_id),&(nbo.filesize),nbo.name);
    pthread_cond_signal(&get); // Signalling that we have removed or got the request
    pthread_mutex_unlock(&lock); // Unlocking ( Releasing the lock )
    request_serve_static(nbo.fd_id,nbo.name,nbo.filesize); // Serving After releasing the lock
    close_or_die(nbo.fd_id); // Closing the request after serving
  }
}
//
// Initial handling of the request
//
void request_handle(int fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
    char filename[MAXBUF], cgiargs[MAXBUF];
    
	// get the request type, file path and HTTP version
    readline_or_die(fd, buf, MAXBUF);
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("method:%s uri:%s version:%s\n", method, uri, version);

	// verify if the request type is GET is not
    if (strcasecmp(method, "GET")) {
		request_error(fd, method, "501", "Not Implemented", "server does not implement this method");
		return;
    }
    request_read_headers(fd);
  

  // check if the url doesnot start with ..
  if(strlen(uri)>=2 && (uri[0]=='.' && uri[1]=='.')){
    request_error(fd, filename, "403", "Forbidden", "Traversing up in filesystem is not allowed");
			return;
  }
  if(strlen(uri)>2 && (uri[0]=='/' && uri[1]=='.' && uri[2]=='.')){
    request_error(fd, filename, "403", "Forbidden", "Traversing up in filesystem is not allowed");
			return;
  }

    
	// check requested content type (static/dynamic)
    is_static = request_parse_uri(uri, filename, cgiargs);
    
	// get some data regarding the requested file, also check if requested file is present on server
    if (stat(filename, &sbuf) < 0) {
		request_error(fd, filename, "404", "Not found", "server could not find this file");
		return;
    }
    
	// verify if requested content is static
    if (is_static) {
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			request_error(fd, filename, "403", "Forbidden", "server could not read this file");
			return;
		}
		
		// TODO: write code to add HTTP requests in the buffer based on the scheduling policy
    pthread_mutex_lock(&lock); // Aquiring the lock
    while(buffer_size==buffer_max_size){ // Waiting when the buffer is full
      pthread_cond_wait(&get, &lock);// Waiting on get as the worker thread will notify
      //that it has removed a request in buffer
    }
    // Now Checking The scheduling Algorithm and adding respectively in the buffer
    if(scheduling_algo==0){
      addinBufferFIFO(&final_buffer,fd,filename,sbuf.st_size);
    }
    else{
      addinBufferSSF(&final_buffer,fd,filename,sbuf.st_size);
    }
    pthread_cond_signal(&put); // Signalling that main thread has putted the request in buffer
    pthread_mutex_unlock(&lock);  // Unlocking ( Releasing the lock )

    } else {
		request_error(fd, filename, "501", "Not Implemented", "server does not serve dynamic content request");
    }
}

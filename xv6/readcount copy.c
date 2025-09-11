#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{
  int initial_count, final_count;
  char buf[100];
  int fd;
  int read_bytes;

  // 1. Get the initial read count
  initial_count = getreadcount();
  printf("Initial read count: %d\n", initial_count);

  // 2. Create and write to a dummy file to ensure it exists
  fd = open("dummy.txt", O_CREATE | O_WRONLY);
  if(fd < 0){
    printf("Error creating dummy file\n");
    exit(1);
  }
  write(fd, "just some text to make sure the file is not empty........................................................................................................", 150);
  close(fd);

  // 3. Open and read 100 bytes from the dummy file
  fd = open("dummy.txt", O_RDONLY);
  if(fd < 0){
    printf("Error opening dummy file for reading\n");
    exit(1);
  }

  read_bytes = read(fd, buf, 100);
  if(read_bytes < 0){
    printf("Error reading from dummy file\n");
    exit(1);
  }
  printf("Read %d bytes from dummy.txt\n", read_bytes);
  close(fd);
  
  // 4. Get the final read count
  final_count = getreadcount();
  printf("Final read count: %d\n", final_count);

  // 5. Verify the increase
  if(final_count >= initial_count + 100) {
    printf("SUCCESS: read count increased by at least 100!\n");
  } else {
    printf("FAILURE: read count did not increase as expected.\n");
  }

  // Clean up the dummy file
  unlink("dummy.txt");

  exit(0);
}
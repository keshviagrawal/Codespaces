#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int initial_count, final_count;
  int fd;
  char buffer[100];
  
  // Get initial read count
  initial_count = getreadcount();
  printf("Initial read count: %d bytes\n", initial_count);
  
  // Create a test file and read from it
  fd = open("README", 0);
  if(fd < 0) {
    printf("Error: Could not open README file\n");
    exit(1);
  }
  
  // Read 100 bytes
  if(read(fd, buffer, 100) != 100) {
    printf("Error: Could not read 100 bytes\n");
    close(fd);
    exit(1);
  }
  
  close(fd);
  
  // Get final read count
  final_count = getreadcount();
  printf("Final read count: %d bytes\n", final_count);
  printf("Bytes read in this test: %d\n", final_count - initial_count);
  
  // Verify the increase
  if(final_count - initial_count == 100) {
    printf("SUCCESS: Read count increased by exactly 100 bytes\n");
  } else {
    printf("ERROR: Expected increase of 100 bytes, got %d\n", final_count - initial_count);
  }
  
  exit(0);
}

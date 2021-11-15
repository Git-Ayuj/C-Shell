#include <iostream>
#include <unistd.h> // notice this! you need it!
#include "functions.h"
using namespace std;
void print_hello(){
   printf("Hello world\n");
   while(true){
      printf("Hello world\n");
      usleep(10000);
   }
}

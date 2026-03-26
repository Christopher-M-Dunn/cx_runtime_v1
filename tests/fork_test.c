#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"
#include "../zoo/addsub/addsub.h"
#include "../zoo/muldiv/muldiv.h"

#include <sys/types.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <unistd.h> 

static int a = 3, b = 5, c = 2, result = 0;
static cx_stctxs_t expected_stctxs = {.sel = {
                                .dc = CX_DIRTY,
                                .R = 0,
                                .state_size = 1,
                                .version = 1
                              }};

/* 
* Ensuring that the fork function works on whichever device
* or emulator this is run on
*/
void test_fork() { 
  pid_t p = fork(); 
  assert(p >= 0);
  if (p == 0) { 
    exit(EXIT_SUCCESS);
  } else {
    wait(NULL);
  }
  return; 
}

/*
* Open and closing in a single process
*/
void test_fork_0() { 

  int result;

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  pid_t pid = fork(); 
  assert(pid >= 0);

  if (pid == 0) { 
  // child process
    int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(cx_sel_C0 > 0);
    cx_error_clear();
    cx_sel(cx_sel_C0);
    result = mac(b, b);
    assert( result == 25 );
    cx_close(cx_sel_C0);

    exit(EXIT_SUCCESS);
  
  } else {
  // parent process

    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }
  return; 
}


/*
* Open and closing in both processes
*/
void test_fork_1() { 

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  pid_t pid = fork(); 
  assert(pid >= 0);

  if (pid == 0) { 
  // child process
    int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(cx_sel_C0 > 0);
    cx_error_clear();
    cx_sel(cx_sel_C0);
    result = mac(b, b);
    assert( result == 25 );
    cx_close(cx_sel_C0);

    exit(EXIT_SUCCESS);
  
  } else {
  // parent process
    int cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(cx_sel_C1 > 0);
    cx_error_clear();
    cx_sel(cx_sel_C1);
    result = mac(c, c);
    assert( result == 4 );
    cx_close(cx_sel_C1);

    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }
  return; 
}

/* 
* Strictly stateless open and close
*/
void test_fork_2() {

  // cx_share_t EX = 0, share_C = 0;
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  pid_t pid = fork();

  // cx_index = cx_csr_read(CX_INDEX);
  if (pid < 0){ 
    perror("fork fail"); 
    exit(1); 
  } else if (pid == 0) {
    int cx_sel_C2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C2 > 0 );

    cx_sel(cx_sel_C2);
    result = add(a, b);
    assert( result == 8 );
    result = add(a, c);
    assert( result == 5 );

    cx_close(cx_sel_C2);

    exit(EXIT_SUCCESS);
  } else {
    int cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    int cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 > 0 );
    assert( cx_sel_C5 > 0 );

    cx_sel(cx_sel_C4);

    result = mul(a, b);
    assert( result == 15 );
    result = mul(c, b);
    assert( result == 10 );
    result = mul(a, c);
    assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b);
    assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);
    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }
}


/*
* Open and closing in both processes, with some more
* complexities.
*/
void test_fork_3() {

  // cx_share_t EX = 0, share_C = 0;
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  pid_t pid = fork();

  if (pid < 0){ 
    perror("fork fail"); 
    exit(1); 
  } else if (pid == 0) {
    cx_sel_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t cx_sel_C2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C1 > 0 );
    assert( cx_sel_C2 > 0 );

    cx_sel(cx_sel_C1);
    result = mac(b, c);
    assert( result == 10 );
    result = mac(b, c);
    assert( result == 20 );
    result = mac(b, c);
    assert( result == 30 );

    cx_sel(cx_sel_C2);
    result = add(a, b);
    assert( result == 8 );
    result = add(a, c);
    assert( result == 5 );

    cx_sel(cx_sel_C1);
    result = mac(a, c);
    assert( result == 36 );

    cx_close(cx_sel_C1);
    cx_close(cx_sel_C2);

    exit(EXIT_SUCCESS);
  } else {
    cx_sel_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_sel_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 > 0 );
    assert( cx_sel_C5 > 0 );

    cx_sel(cx_sel_C4);

    result = mul(a, b);
    assert( result == 15 );
    result = mul(c, b);
    assert( result == 10 );
    result = mul(a, c);
    assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b);
    assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);
    // Parent process
    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }
}

void complex_fork_test() {

  // cx_share_t EX = 0, share_C = 0;
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);

  assert(cx_sel_C0 > 0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(MCX_SELECTOR);
  
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    cx_sel_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t cx_sel_C2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C1 > 0 );
    assert( cx_sel_C2 > 0 );

    cx_sel(cx_sel_C1);

    result = mac(b, c);
    assert( result == 10 );
    result = mac(b, c);
    // sometimes bugs out on the result == 20
    assert( result == 20 );
    result = mac(b, c);
    assert( result == 30 );

    cx_sel(cx_sel_C2);
    result = add(a, b);
    assert( result == 8 );
    result = add(a, c);
    assert( result == 5 );

    cx_sel(cx_sel_C1);
    result = mac(a, c);
    assert( result == 36 );

    cx_close(cx_sel_C0);
    cx_close(cx_sel_C1);
    cx_close(cx_sel_C2);

    exit(EXIT_SUCCESS);
  } else {
    cx_sel_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_sel_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 > 0 );
    assert( cx_sel_C5 > 0 );

    cx_sel(cx_sel_C4);

    result = mul(a, b);
    assert( result == 15 );
    result = mul(c, b);
    assert( result == 10 );
    result = mul(a, c);
    assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b);
    assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);
    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }

  result = mac(b, b);

  cx_close(cx_sel_C0);

  cx_sel(CX_LEGACY);
}



void use_prev_opened_in_child() {

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);

  assert(cx_sel_C0 > 0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(MCX_SELECTOR);
  
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    // child
    cx_sel_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_C1 > 0 );
    
    cx_sel(cx_sel_C0);
    result = mac(b, c);
    assert( result == 35 );
    result = mac(b, c);
    assert( result == 45 );
    result = mac(b, c);
    assert( result == 55 );

    cx_sel(cx_sel_C1);
    result = mac(a, c);
    assert( result == 6 );

    cx_close(cx_sel_C0);
    cx_close(cx_sel_C1);

    exit(EXIT_SUCCESS);
  } else {
    cx_sel_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_sel_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 > 0 );
    assert( cx_sel_C5 > 0 );

    cx_sel(cx_sel_C4);

    result = mul(a, b);
    assert( result == 15 );
    result = mul(c, b);
    assert( result == 10 );
    result = mul(a, c);
    assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b);
    assert(result == 8);

    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);
    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }

  cx_sel(cx_sel_C0);
  result = mac(b, b);
  printf("result: %d\n", result);
  assert (result == 50 );

  cx_close(cx_sel_C0);

  cx_sel(CX_LEGACY);
}

void use_prev_opened_in_parent() {

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);

  assert(cx_sel_C0 > 0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(MCX_SELECTOR);
  
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    cx_sel_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_sel_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 > 0 );
    assert( cx_sel_C5 > 0 );

    cx_sel(cx_sel_C4);

    result = mul(a, b);
    assert( result == 15 );
    result = mul(c, b);
    assert( result == 10 );
    result = mul(a, c);
    assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b);
    assert(result == 8);

    cx_close(cx_sel_C0);
    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);

    exit(EXIT_SUCCESS);
  } else {
    // child
    cx_sel_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_C1 > 0 );
    
    cx_sel(cx_sel_C0);
    result = mac(b, c);
    assert( result == 35 );
    result = mac(b, c);
    assert( result == 45 );
    result = mac(b, c);
    assert( result == 55 );

    cx_sel(cx_sel_C1);
    result = mac(a, c);
    assert( result == 6 );

    cx_close(cx_sel_C1);

    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }

  cx_sel(cx_sel_C0);
  result = mac(b, b);
  assert (result == 80 );

  cx_close(cx_sel_C0);

  cx_sel(CX_LEGACY);
}

void use_prev_opened_in_parent_and_child() {

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);

  assert(cx_sel_C0 > 0);

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(b, b);
  assert( result == 25 );
  uint init_mcx_val = cx_csr_read(MCX_SELECTOR);
  
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    cx_sel_t cx_sel_C4 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_sel_t cx_sel_C5 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( cx_sel_C4 > 0 );
    assert( cx_sel_C5 > 0 );

    cx_sel(cx_sel_C0);
    result = mac(b, b);
    assert(result == 50);
    result = mac(b, b);
    assert(result == 75);
    result = mac(b, b);
    assert(result == 100);
    result = mac(b, b);
    assert(result == 125);

    cx_sel(cx_sel_C4);

    result = mul(a, b);
    assert( result == 15 );
    result = mul(c, b);
    assert( result == 10 );
    result = mul(a, c);
    assert( result == 6 );

    cx_sel(cx_sel_C5);
    result = add(a, b);
    assert(result == 8);

    cx_close(cx_sel_C0);
    cx_close(cx_sel_C4);
    cx_close(cx_sel_C5);

    exit(EXIT_SUCCESS);
  } else {
    // child
    cx_sel_t cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_C1 > 0 );
    
    cx_sel(cx_sel_C0);
    result = mac(b, c);
    assert( result == 35 );
    result = mac(b, c);
    assert( result == 45 );
    result = mac(b, c);
    assert( result == 55 );

    cx_sel(cx_sel_C1);
    result = mac(a, c);
    assert( result == 6 );

    cx_close(cx_sel_C1);

    int status;
    // Wait for child
    waitpid(pid, &status, 0);
    assert(status == 0);
  }

  cx_sel(cx_sel_C0);
  result = mac(b, b);
  assert (result == 80 );

  cx_close(cx_sel_C0);

  cx_sel(CX_LEGACY);
}


void close_unclosed_cx() {

  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index, cx_index_0;

  cx_sel_t cx_sel_C0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
  assert(cx_sel_C0 > 0);

  cx_sel_t cx_sel_C1;

  int *glob_counter;
  glob_counter = mmap(NULL, sizeof *glob_counter, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *glob_counter = 0;

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork fail");
    exit(1);
  } else if (pid == 0) {
    // child
    cx_sel_t cx_sel_C4 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    if (cx_sel_C4 > 0)
      *glob_counter += 1;

    exit(EXIT_SUCCESS);
  } else {
    cx_sel_C1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    if (cx_sel_C1 > 0)
      *glob_counter += 1;

    int status;
    // Wait for child
    wait(NULL);
    assert(status == 0);
    printf("counter: %d\n", *glob_counter);
    assert(*glob_counter == 1);
    munmap(glob_counter, sizeof *glob_counter);
  }
  cx_sel_t cx_sel_C5 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
  assert( cx_sel_C5 < 0 );

  cx_close(cx_sel_C0);
  cx_close(cx_sel_C5);

  cx_sel_t cx_sel_C6 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
  cx_sel_t cx_sel_C7 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);

  assert(cx_sel_C6 > 0);
  assert(cx_sel_C7 > 0);

  cx_close(cx_sel_C6);
  cx_close(cx_sel_C7);

  cx_sel(CX_LEGACY);
}


int main() {
    // for (int i = 0; i < 1000; i++) {
      cx_sel(CX_LEGACY);
      test_fork();
      test_fork_0();
      test_fork_1();
      test_fork_2();
      test_fork_3();
      complex_fork_test();
      use_prev_opened_in_child();
      use_prev_opened_in_parent();
      use_prev_opened_in_parent_and_child();
    //   close_unclosed_cx();
    // }

    printf("Context Copy Test Complete\n");
    return 0;
}

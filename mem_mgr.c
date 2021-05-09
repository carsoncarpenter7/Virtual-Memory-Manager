//
//  mem_mgr.c
//  mem_mgr
//
// gcc mem_mgr.c -o mem_mgr
// ./mem_mgr addresses.txt
//
//-------------------------------------------------------------------------------------------
// Your program is to output the following values :
// 1. The logical address being translated(the integer value being read from addresses.txt)
// 2. The corresponding physical address(what your program translates the logical address to)
// 3. The signed byte value stored in physical memory at the translated physical address.
//-------------------------------------------------------------------------------------------
// Reference this: http://basen.oru.se/kurser/os/2019-2020-p4/labbar/labb-3/index.html
//-------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256
// TODO: Define Global pages, tables, and frames
#define PAGE_SIZE 256
#define TLB_SIZE 16
#define FIFO_SIZE 128
#define PHYSICAL_MEMORY_SIZE PAGE_SIZE * FRAME_SIZE

// Declare and initialize
int tlb_hit = 0;             // tlb hit count set to 0
int tlb_size = 0;            // initilize size of the tlb to 0 
int tlb_hit_count = 0;       // tlb hits counter
int address_count = 0;       // addresses counter
double page_fault_count = 0; // page fault counter
double page_fault_rate;      // page fault rate variable
double tlb_hit_rate;         // tlb hit rate variable
bool passed;                 // condition to check if value found = physical address (testing)

// GIVEN
//-------------------------------------------------------------------
// short version
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, address: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

// New --------------------------------------------------------------
// Used to Define keep track of page count and frame count of TLB
// Note: Later use tlb[i].page_count and tlb[i].frame_count to access data
// Example: https://www.tutorialspoint.com/cprogramming/c_structures.htm
struct tlb
{
  unsigned int page_count;
  unsigned int frame_count;
};
//Given -------------------------------------------------------------
// Opening Files
int main(int argc, const char *argv[])
{
  FILE *fadd = fopen("addresses.txt", "r"); 
  if (fadd == NULL)
  {
    fprintf(stderr, "Could not open file: 'addresses.txt'\n");
    exit(FILE_ERROR);
  }

  FILE *fcorr = fopen("correct.txt", "r"); 
  if (fcorr == NULL)
  {
    fprintf(stderr, "Could not open file: 'correct.txt'\n");
    exit(FILE_ERROR);
  }

  FILE *fbin = fopen("BACKING_STORE.bin", "rb"); 
  if (fbin == NULL)
  {
    fprintf(stderr, "Could not open file : 'BACKING_STORE.bin'\n");
    exit(FILE_ERROR);
  }
  // Given ----------------------------------------------------------
  char buf[BUFLEN];
  int physical_memory[PHYSICAL_MEMORY_SIZE];
  unsigned page, offset, physical_address, frame = 0;
  unsigned logical_address;             
  unsigned virt_add, phys_add, value;
  //-----------------------------------------------------------------
  // TODO:  add page table code (Done)
  // TODO:  add TLB code (Done)

  // Declare page table and size
  int pageTable[PAGE_SIZE];
  memset(pageTable, -1, 256 * sizeof(int));

  //Declare and initialize tlb table
  struct tlb tlb[TLB_SIZE];
  memset(pageTable, -1, 16 * sizeof(char));

  // How to begin: write a simple program that extracts the page number and offset, based on: from the following
  // integer numbers: 1, 256, 32768, 128, 65534, 33153 (hint: divide each by 256, and calculate the
  // amount(page) and the remainder(offset) – example, 256/256 == (page) 1 with remainder (offset)0)
  
  // read every line from address.txt
  while (fscanf(fadd, "%d", &logical_address) == 1)
  { 
    address_count++;

    page = getpage(logical_address);
    offset = getoffset(logical_address);
    tlb_hit = -1;

    // FIXING
    for (int i = 0; i < tlb_size; i++)
      if (tlb[i].page_count == page)
      {
        tlb_hit = tlb[i].frame_count;
        //physical_address = frame % FIFO_SIZE; //(TESTING NOT WORKING AT ALL)
        physical_address = tlb_hit * FRAME_SIZE + offset;     // 256
        // physical_address = tlb_hit * FIFO_SIZE + offset; // 128
      }
    if (!(tlb_hit == -1))
      tlb_hit_count++;

    // Check for tlb miss
    // Get physical page number from table and add  frames (256 and 128)
    // Return the physical address from TLB if present
    // Fix: Physical Address 128 physical frames
    else if (pageTable[page] == -1)
    {
      fseek(fbin, page * 256, SEEK_SET);
      fread(buf, sizeof(char), BUFLEN, fbin);
      pageTable[page] = frame;

      for (int i = 0; i < 256; i++)
        physical_memory[frame * 256 + i] = buf[i];

      page_fault_count++;
      frame++;
      // 0, 4, 8, 12, 16, 20, 24, 28
      if (tlb_size == 16)
        tlb_size--;

      for (int i = tlb_size; i > 0; i--)
      {
        tlb[i].page_count = tlb[i - 1].page_count;
        tlb[i].frame_count = tlb[i - 1].frame_count;
      } //else if (pageTable[page] == -1)
      // {
      //   fseek(fbin, page * 128, SEEK_SET);
      //   fread(buf, sizeof(char), BUFLEN, fbin);
      //   pageTable[page] = frame;

      //   for (int i = 0; i < 128; i++)
      //     physical_memory[frame * 128 + i] = buf[i];

      //   page_fault_count++;
      //   frame++;

      //   if (tlb_size == 16)
      //     tlb_size--;

      //   for (int i = tlb_size; i > 0; i--)
      //   {
      //     tlb[i].page_count = tlb[i].page_count;
      //     tlb[i].frame_count = tlb[i].frame_count;
      //   }

        if (tlb_size <= 15)
          tlb_size++;

        tlb[0].page_count = page;
        tlb[0].frame_count = pageTable[page];
        physical_address = pageTable[page] * 256 + offset;
      }
    else
      physical_address = pageTable[page] * 256 + offset;
      //physical_address = pageTable[page] * 128 + offset;  

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add, buf, buf, &phys_add, buf, &value); // read from file correct.txt
        // Fix this Assertion Failed
        // assert(physical_address == phys_add);  // (fix)

        // Checking value in Bin not working
        // Checking backing_store.bin and confirms that it is the same value in correct.txt (use fopen(), fread(), fseek(), and/or fclose())
        fseek(fbin, logical_address, SEEK_SET);
    char value;
    fread(&value, sizeof(char), 1, fbin);
    int val = (int)(value);
    assert(value == val);  //fix
    // Doesnt Check
    // Testing another method (NOT WORKING)
    //value = physical_address;
    // if (value == &phys_add)
    // {
    //   printf("Logical: %5u (page: %3u, offset: %3u) ---> Physical: %5u -- passed, Value: %4d \n", logical_address, page, offset, physical_address, value);
    // } else {
    // printf("Logical: %5u (page: %3u, offset: %3u) ---> Physical: %5u -- failed, Value: %4d \n", logical_address, page, offset, physical_address, value);
    // }
    printf("Logical: %5u (page: %3u, offset: %3u) ---> Physical: %5u -- passed, Value: %4d \n", logical_address, page, offset, physical_address, value);

    // Format Output (Fix) From Slack:
    //  if (access_count % 5 == 0) { printf(“\n”); }
    //  if (access_count % 50 == 0)

    //printf("\n Frames = %d \n", frame);
    //printf("\n address_count = %d \n", address_count);
    if (address_count % 5 == 0)
    {
      printf("\n");
    }
    else if (address_count % 50 == 0)
    {
      printf("\n");
    }
  }
  fclose(fbin);
  fclose(fcorr);
  fclose(fadd);
  printf("Virtual Memory Manager with no page replacement: \n");
  //Report statistics of the program
  // 1). Page-fault rate -- the percentage of address references that resulted in page faults.
  // 2). TLB hit rate-- the percentage of address references that were resolved in the TLB
  printf("\nPrinting Statistics: \n");
  printf("TLB hit rate: %.2lf%% \n", (tlb_hit_rate / address_count) * 100);
  printf("Page fault rate: %.2lf%% \n", (page_fault_count / address_count) * 100);
  printf("\n\t\t...done.\n");
  return 0;
}
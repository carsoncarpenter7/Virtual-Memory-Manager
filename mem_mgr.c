//
//  memmgr.c
//  memmgr
// ./memmgr addresses.txt
//-------------------------------------------------------------------------------------------
// Your program is to output the following values :
// 1. The logical address being translated(the integer value being read from addresses.txt)
// 2. The corresponding physical address(what your program translates the logical address to)
// 3. The signed byte value stored in physical memory at the translated physical address.
//-------------------------------------------------------------------------------------------
// Reference this: http://basen.oru.se/kurser/os/2019-2020-p4/labbar/labb-3/index.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256
// New
#define PAGE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE PAGE_SIZE *FRAME_SIZE

int address_count = 0; // keeps track of how many addresses have been read
int hit = 0;
int tlb_size = 0;      //size of the tlb
int tlb_hit_count = 0; //used to count how many tlb hits there are
int frame = 0;
int page_fault_count = 0; // counts the amounts of page faults
float page_fault_rate;
float tlb_hit_rate;

// GIVEN
//-------------------------------------------------------------------
// short version
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

// New -----------------------------------------
struct tlb
{
  unsigned int pageNum;
  unsigned int frameNum;
};
//Given -----------------------------------------------

int main(int argc, const char *argv[])
{
  FILE *fadd = fopen("addresses.txt", "r"); // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL)
  {
    fprintf(stderr, "Could not open file: 'addresses.txt'\n");
    exit(FILE_ERROR);
  }

  FILE *fcorr = fopen("correct.txt", "r"); // contains the logical and physical address, and its value
  if (fcorr == NULL)
  {
    fprintf(stderr, "Could not open file: 'correct.txt'\n");
    exit(FILE_ERROR);
  }

  FILE *fbin = fopen("backing_store.bin", "rb"); // Will read the binary storage file
  if (fbin == NULL)
  {
    fprintf(stderr, "Could not open file : 'backing_store.bin'\n");
    exit(FILE_ERROR);
  }
  // Given ---------------------------------------------------------
  char buf[BUFLEN];
  int physical_memory[PHYSICAL_MEMORY_SIZE];
  unsigned page, offset, physical_add, frame = 0;
  unsigned logic_add;                 // read from file address.txt
  unsigned virt_add, phys_add, value; // read from file correct.txt

  printf("READING ALL ENTRIES: \n\n");
  // How to begin: write a simple program that extracts the page number and offset, based on: from the following
  // integer numbers: 1, 256, 32768, 128, 65534, 33153 (hint: divide each by 256, and calculate the
  // amount(page) and the remainder(offset) – example, 256/256 == (page) 1 with remainder (offset)0).

  // TODO:  add page table code
  // TODO:  add TLB code
  // Declare and initialize page table to -1
  int pageTable[PAGE_SIZE];
  memset(pageTable, -1, 256 * sizeof(int));

  //Declare and initialize tlb[] structure to -1
  struct tlb tlb[TLB_SIZE];
  memset(pageTable, -1, 16 * sizeof(char));

  while (fscanf(fadd, "%d", &logic_add) == 1)
  { // will read every line from address.txt
    address_count++;

    page = getpage(logic_add);
    offset = getoffset(logic_add);
    hit = -1;

    // FIXING/UPDATING

    //This will check to see if the page number is already in tlb
    //If it is in tlb, then tlb hit will increase
    for (int i = 0; i < tlb_size; i++)
      if (tlb[i].pageNum == page)
      {
        hit = tlb[i].frameNum;
        physical_add = hit * FRAME_SIZE + offset;
      }
    if (!(hit == -1))
      tlb_hit_count++;

    //Checking for tlb miss
    //Will get physical page number from table
    else if (pageTable[page] == -1)
    {
      fseek(fbin, page * 256, SEEK_SET);
      fread(buf, sizeof(char), BUFLEN, fbin);
      pageTable[page] = frame;

      for (int i = 0; i < 256; i++)
        physical_memory[frame * 256 + i] = buf[i];

      page_fault_count++;
      frame++;

      if (tlb_size == 16)
        tlb_size--;

      for (int i = tlb_size; i > 0; i--)
      {
        tlb[i].pageNum = tlb[i - 1].pageNum;
        tlb[i].frameNum = tlb[i - 1].frameNum;
      }

      if (tlb_size <= 15)
        tlb_size++;

      tlb[0].pageNum = page;
      tlb[0].frameNum = pageTable[page];
      physical_add = pageTable[page] * 256 + offset;
    }
    else
      physical_add = pageTable[page] * 256 + offset;

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add, buf, buf, &phys_add, buf, &value); // read from file correct.txt

    assert(physical_add == phys_add);

    // Reads backing_store.bin and confirms that it is the same value in correct.txt (use fopen(), fread(), fseek(), and/or fclose())
    fseek(fbin, logic_add, SEEK_SET);
    char c;
    fread(&c, sizeof(char), 1, fbin);
    int val = (int)(c);
    assert(val == value);
    //printf("Value from BACKING_STORE.BIN txt is %d whereas value from correct.txt is %d \n", val, value);

    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0)
    {
      printf("\n");
    }

    //Report statistics of the program
    // 1). Page-fault rate -- the percentage of address references that resulted in page faults.
    // 2). TLB hit rate-- the percentage of address references that were resolved in the TLB
    page_fault_count = page_fault_count * 1.0f / address_count;
    tlb_hit_rate = tlb_hit_count * 1.0f / address_count;        
  }
  fclose(fbin);
  fclose(fcorr);
  fclose(fadd);

  printf("\nAddresses: %d \n", address_count);
  printf("Page faults: %d \n", page_fault_count);
  printf("Page fault rate: %d \n", page_fault_count);
  printf("TLB hits: %d \n", tlb_hit_count);
  printf("TLB hit rate: %f \n", tlb_hit_rate);
  printf("\n\t\t...done.\n");
  return 0;
}

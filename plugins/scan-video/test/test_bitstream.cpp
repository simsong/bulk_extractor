#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitstream_reader.h"
#include "bitstream_writer.h"

using namespace std;

int test1()
{
  printf("Starting test 1: single bit reads.\n");

  bitstream_reader* reader = new bitstream_reader();

  static const int N=100;
  char data[N];
  for (int i=0; i<N; i++) {
    data[i] = (char) (random() & 0xff);
  }

  reader->set_data(data, N);

  char buf[N];
  for (int i=0; i<N; i++) {
    int byte = 0;
    for (int j=0; j<8; j++) {
      int bit = reader->read_bit();
      if (bit < 0) {
        printf("Bit is -1 at byte %d bit %d\n", i,j);
        return 1;
      }
      byte <<= 1;
      byte |= (bit & 1);
    }

    buf[i] = byte & (0xff);
  }

  for (int i=0; i<N; i++) {
    if (data[i] != buf[i]) {
      printf("Byte %d differ!\n", i);
      return 1;
    }
  }

  delete reader;
  printf("Test 1 successful.\n");
  return 0;
}

int test2()
{
  printf("Starting test 2: multiple bit reads.\n");

  bitstream_reader* reader = new bitstream_reader();

  static const int N=100;
  char data[N];
  for (int i=0; i<N; i++) {
    data[i] = (char) (random() & 0xff);
  }

  reader->set_data(data, N);

  char buf[N];
  for (int i=0; i<N; i++) {
    int byte = 0;

    int num_bits = (int) (random() % 7) + 1;
    int bits = reader->read_bits(num_bits);
    if (bits < 0) {
      printf("Bits -1 at byte %d, num_bits = %d\n", i, num_bits);
      return 1;
    }
    
    int remaining = reader->read_remaining_byte();
    if (remaining < 0) {
      printf("Remaining byte -1 at byte %d, num_bits = %d\n", i, num_bits);
      return 1;
    }

    byte = bits;
    byte <<= (8-num_bits);
    byte |= remaining;

    buf[i] = byte & (0xff);
  }

  for (int i=0; i<N; i++) {
    if (data[i] != buf[i]) {
      printf("Byte %d differ!\n", i);
      return 1;
    }
  }

  delete reader;
  printf("Test 1 successful.\n");
  return 0;
}

int test3()
{
  printf("Starting test 3: single byte reads.\n");

  bitstream_reader* reader = new bitstream_reader();

  static const int N=100;
  char data[N];
  for (int i=0; i<N; i++) {
    data[i] = (char) (random() & 0xff);
  }

  reader->set_data(data, N);

  char buf[N];
  for (int i=0; i<N; i++) {
    buf[i] = reader->read_byte();
  }

  for (int i=0; i<N; i++) {
    if (data[i] != buf[i]) {
      printf("Byte %d differ!\n", i);
      return 1;
    }
  }

  delete reader;
  printf("Test 3 successful.\n");
  return 0;
}

int test4()
{
  printf("Starting test 4: single bit writes.\n");

  bitstream_writer* writer = new bitstream_writer();

  static const int N=100;
  char data[N];
  for (int i=0; i<N; i++) {
    data[i] = (char) (random() & 0xff);
  }

  char data2[N];
  writer->set_data(data2, N);

  for (int i=0; i<N; i++) {
    int byte = (data[i] & 0xff);
    for (int j=0; j<8; j++) {
      int nb = writer->write_bit((byte >> (7-j)) & 1);
      if (nb < 0) {
        printf("Failed to write byte bit %d at byte %d\n", j,i);
        return 1;
      }
    }
  }

  for (int i=0; i<N; i++) {
    if (data[i] != data2[i]) {
      printf("Byte %d differ!\n", i);
      return 1;
    }
  }

  delete writer;
  printf("Test 4 successful.\n");
  return 0;
}

int test5()
{
  printf("Starting test 5: multiple bit writes.\n");

  bitstream_reader* reader = new bitstream_reader();
  bitstream_writer* writer = new bitstream_writer();

  static const int N=100;
  char data[N];
  for (int i=0; i<N; i++) {
    data[i] = (char) (random() & 0xff);
  }
  reader->set_data(data, N);

  char buf[N];
  writer->set_data(buf, N);

  for (int i=0; i<N; i++) {
    int num_bits = (int) (random() % 7) + 1;
    int bits = reader->read_bits(num_bits);
    if (bits < 0) {
      printf("Failed to read at byte %d, num_bits = %d\n", i, num_bits);
      return 1;
    }

    int nb = writer->write_bits(bits, num_bits);
    if (nb <= 0) {
      printf("Failed to write at byte %d, num_bits = %d\n", i, num_bits);
      return 1;
    }
    
    int remaining = reader->read_remaining_byte();
    if (remaining < 0) {
      printf("Remaining byte -1 at byte %d, num_bits = %d\n", i, num_bits);
      return 1;
    }

    nb = writer->write_bits(remaining, 8-num_bits);
    if (nb <= 0) {
      printf("Failed to write reminaing bits at byte %d, num_bits = %d\n", i, num_bits);
      return 1;
    }
  }

  for (int i=0; i<N; i++) {
    if (data[i] != buf[i]) {
      printf("Byte %d differ!\n", i);
      return 1;
    }
  }

  delete reader;
  delete writer;
  printf("Test 5 successful.\n");
  return 0;
}

int test6()
{
  printf("Starting test 6: single byte writes.\n");

  bitstream_writer* writer = new bitstream_writer();

  static const int N=100;
  char data[N];
  for (int i=0; i<N; i++) {
    data[i] = (char) (random() & 0xff);
  }

  char buf[N];
  writer->set_data(buf, N);

  for (int i=0; i<N; i++) {
    writer->write_byte(data[i]);
  }

  for (int i=0; i<N; i++) {
    if (data[i] != buf[i]) {
      printf("Byte %d differ!\n", i);
      return 1;
    }
  }

  delete writer;
  printf("Test 6 successful.\n");
  return 0;
}

int main(int argc, char** argv) 
{
  if (test1() != 0) {
    printf("Test 1 failed.\n");
  }

  if (test2() != 0) {
    printf("Test 2 failed.\n");
  }

  if (test3() != 0) {
    printf("Test 3 failed.\n");
  }

  if (test4() != 0) {
    printf("Test 4 failed.\n");
  }
  if (test5() != 0) {
    printf("Test 5 failed.\n");
  }
  if (test6() != 0) {
    printf("Test 6 failed.\n");
  }
  return 0;
}


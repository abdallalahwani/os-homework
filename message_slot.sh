#!/bin/bash
sudo -v
set -e

# Build kernel module and user programs
make

# Insert kernel module
sudo insmod message_slot.ko || { echo "Failed to insert module"; exit 1; }

# Create two device files
sudo mknod /dev/slot0 c 235 0 || true
sudo mknod /dev/slot1 c 235 1 || true
sudo chmod 666 /dev/slot*
gcc -O3 -Wall -std=c11 -o message_sender message_sender.c
gcc -O3 -Wall -std=c11 -o message_reader message_reader.c

echo "===== Test valid usage ====="
./message_sender /dev/slot0 123 "HelloSlot0"
./message_sender /dev/slot1 42 "HelloSlot1"
echo -n "Read slot0 channel 123: "
./message_reader /dev/slot0 123
echo -n "Read slot1 channel 42: "
./message_reader /dev/slot1 42
echo "OK"

echo "===== Test invalid channel (0) ====="
if ./message_sender /dev/slot0 0 "BadChannel"; then
  echo "Should have failed, but succeeded"
  exit 1
fi
echo "OK (failed as expected)"

echo "===== Test oversize message (>128) ====="
MSG=$(head -c 129 < /dev/urandom | base64)
if ./message_sender /dev/slot0 123 "$MSG"; then
  echo "Should have failed, but succeeded"
  exit 1
fi
echo "OK (failed as expected)"

echo "===== Test read before any write ====="
sudo rm -f /dev/slot2
sudo mknod /dev/slot2 c 235 2
sudo chmod 666 /dev/slot2
if ./message_reader /dev/slot2 900; then
  echo "Should have failed, but succeeded"
  exit 1
fi
echo "OK (failed as expected)"

echo "All tests passed."

# Cleanup
sudo rmmod message_slot
sudo rm -f /dev/slot* /dev/slot2
make clean

echo 
echo 

echo building module ...
make
echo loading module...
sudo insmod message_slot.ko
echo creating devices...
sudo mknod /dev/slot0 c 235 0
sudo mknod /dev/slot1 c 235 1
sudo chmod 666 /dev/slot0 /dev/slot1
echo building tests...
gcc -O3 -Wall -std=c11 Test.c -o test_program

echo running tests...
./test_program
echo cleaning up...
sudo rm /dev/slot0 /dev/slot1
sudo rmmod message_slot
make clean
sudo rm -r test_program

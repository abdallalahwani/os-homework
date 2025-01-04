
sudo -v
@echo off
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


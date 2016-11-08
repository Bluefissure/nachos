echo "Press any key to continue..............."
read
make clean
make
./nachos -f
echo "Format DISK complete."

echo "Press any key to continue..............."
read 
echo "Continue shell..........................."
./nachos -cp test/empty empty
./nachos -cp test/small small
./nachos -cp test/medium medium
./nachos -cp test/big big
echo "Copy empty, small, medium, big completed."

echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -nap small empty
echo "NAppend small completed."

echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -nap small empty
echo "NAppend small completed."

echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -nap small empty
echo "NAppend small completed."

echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -nap medium empty
echo "NAppend medium completed."

echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -nap small empty
echo "NAppend small completed."

echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -nap big empty
echo "NAppend big completed."

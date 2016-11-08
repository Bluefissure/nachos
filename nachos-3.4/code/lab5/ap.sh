
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
echo "Copy empty completed."
echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -ap test/small empty
echo "Append small completed."
echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -hap test/small empty
echo "Append small completed."
echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -ap test/small empty
echo "Append small completed."
echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -ap test/medium empty
echo "Append medium completed."
echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -ap test/small empty
echo "Append small completed."
echo "Press any key............................"
read 
echo "Continue shell..........................."
./nachos -ap test/big empty
echo "Append big completed."

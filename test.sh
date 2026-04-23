#!/bin/bash

echo "Cleaning and building..."
make clean
make

echo "Starting server..."
./server_app &
SERVER_PID=$!

sleep 1

echo "Running test clients..."

# Test 1: Submit job
echo -e "admin\nadmin123\n1\nls\n4\n" | ./client_app

# Test 2: Another job
echo -e "user\nuser123\n1\npwd\n4\n" | ./client_app

# Test 3: Check status
echo -e "admin\nadmin123\n2\n1\n4\n" | ./client_app

# Test 4: Get result
echo -e "admin\nadmin123\n3\n1\n4\n" | ./client_app

sleep 1

echo "Logs:"
cat logs/system.log

echo "Stopping server..."
kill $SERVER_PID
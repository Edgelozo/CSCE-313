#!/bin/bash
# filepath: /home/hyjacksyn/PA-1 starter code/PA-1/performance-test.sh

echo -e "File Transfer Performance Test\n"

# Colors for output
ORANGE='\033[0;33m'
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test file sizes (in bytes) - Limited to 100 MB max
declare -a sizes=(
    "1048576"      # 1 MB
    "5242880"      # 5 MB
    "10485760"     # 10 MB
    "26214400"     # 25 MB
    "52428800"     # 50 MB
    "104857600"    # 100 MB
)

# Function to format bytes in human readable format
format_bytes() {
    local bytes=$1
    if [ $bytes -lt 1048576 ]; then
        echo "${bytes} bytes ($(echo "scale=1; $bytes/1024" | bc) KB)"
    elif [ $bytes -lt 1073741824 ]; then
        local mb=$(echo "scale=1; $bytes/1048576" | bc)
        local mib=$(echo "scale=1; $bytes/1048576" | bc)
        echo "${bytes} bytes (${mb} MB, ${mib} MiB)"
    else
        local gb=$(echo "scale=2; $bytes/1073741824" | bc)
        local gib=$(echo "scale=2; $bytes/1073741824" | bc)
        echo "${bytes} bytes (${gb} GB, ${gib} GiB)"
    fi
}

# Function to calculate transfer rate
calculate_rate() {
    local bytes=$1
    local time=$2
    local rate=$(echo "scale=1; $bytes / $time / 1000000000" | bc)
    echo "${rate} GB/s"
}

# Make sure programs are compiled
make clean > /dev/null 2>&1
make > /dev/null 2>&1

echo -e "${BLUE}Testing file transfer performance...${NC}\n"

for size in "${sizes[@]}"; do
    echo -e "${ORANGE}Testing $(format_bytes $size)${NC}"
    
    # Create test file
    dd if=/dev/zero of=BIMDC/test.bin bs=$size count=1 > /dev/null 2>&1
    
    # Time the file transfer
    start_time=$(date +%s.%N)
    ./client -f test.bin > /dev/null 2>&1
    end_time=$(date +%s.%N)
    
    # Calculate elapsed time
    elapsed=$(echo "$end_time - $start_time" | bc)
    
    # Check if transfer was successful
    if diff -q BIMDC/test.bin received/test.bin > /dev/null 2>&1; then
        rate=$(calculate_rate $size $elapsed)
        echo -e "  ${GREEN}✓${NC} $(format_bytes $size) copied, ${elapsed} s, ${rate}"
    else
        echo -e "  ${RED}✗${NC} Transfer failed"
    fi
    
    # Cleanup
    rm -f BIMDC/test.bin received/test.bin
    
    echo ""
done

echo -e "\n${BLUE}Testing with new channel (-c flag)...${NC}\n"

# Test with new channel for all file sizes
for size in "${sizes[@]}"; do
    echo -e "${ORANGE}Testing $(format_bytes $size) with new channel${NC}"
    
    # Create test file
    dd if=/dev/zero of=BIMDC/test.bin bs=$size count=1 > /dev/null 2>&1
    
    # Time the file transfer with new channel
    start_time=$(date +%s.%N)
    ./client -c -f test.bin > /dev/null 2>&1
    end_time=$(date +%s.%N)
    
    # Calculate elapsed time
    elapsed=$(echo "$end_time - $start_time" | bc)
    
    # Check if transfer was successful
    if diff -q BIMDC/test.bin received/test.bin > /dev/null 2>&1; then
        rate=$(calculate_rate $size $elapsed)
        echo -e "  ${GREEN}✓${NC} $(format_bytes $size) copied, ${elapsed} s, ${rate} (new channel)"
    else
        echo -e "  ${RED}✗${NC} Transfer failed"
    fi
    
    # Cleanup
    rm -f BIMDC/test.bin received/test.bin
    
    echo ""
done

echo -e "${BLUE}Performance testing complete!${NC}"
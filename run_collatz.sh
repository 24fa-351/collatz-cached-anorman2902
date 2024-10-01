#!/bin/bash

# Check for correct number of arguments
if [ "$#" -ne 5 ]; then
    echo "Usage: ./collatz <N> <MIN> <MAX> <CACHE_POLICY> <CACHE_SIZE>"
    exit 1
fi

# Assigns input arguments to variables
N=$1
MIN=$2
MAX=$3
CACHE_POLICY=$4
CACHE_SIZE=$5

# Creates separate CSV files for LRU and FIFO
if [[ "$CACHE_POLICY" == "LRU" ]]; then
    CSV_FILE="collatz_results_LRU.csv"
elif [[ "$CACHE_POLICY" == "FIFO" ]]; then
    CSV_FILE="collatz_results_FIFO.csv"
else
    echo "Error: Unsupported cache policy! Use LRU or FIFO."
    exit 1
fi

# Adds headers to the CSV file
echo "N,Min,Max,Policy,Cache Size,Random Number,Steps,Cache Hit,Cache Hit Percentage" > "$CSV_FILE"

# Runs the Collatz program and captures the output
OUTPUT=$(./collatz "$N" "$MIN" "$MAX" "$CACHE_POLICY" "$CACHE_SIZE")

# Initializes variables to store results
random_numbers=()
steps=()
cache_hits=()
cache_hit_percentage=0

# Parses the output to extract random numbers, steps, and cache hits
while IFS= read -r line; do
    if [[ "$line" =~ ^[[:space:]]*[0-9]+ ]]; then
        read -r random_number captured_steps captured_cache_hit <<< "$line"
        random_numbers+=("$random_number")
        steps+=("$captured_steps")
        cache_hits+=("$captured_cache_hit")
    elif [[ "$line" =~ Cache\ Hit\ Percentage: ]]; then
        # Capture the cache hit percentage
        cache_hit_percentage=$(echo "$line" | awk '{print $NF}' | sed 's/%//') # Removes the percentage sign
    fi
done <<< "$OUTPUT"

# Append the results to the CSV file
for (( i=0; i<${#random_numbers[@]}; i++ )); do
    cache_hit_status=${cache_hits[$i]}
    if [[ "$cache_hit_status" == "Yes" ]]; then
        cache_hit_status="Yes"
    else
        cache_hit_status="No"
    fi
    echo "$N,$MIN,$MAX,$CACHE_POLICY,$CACHE_SIZE,${random_numbers[$i]},${steps[$i]},$cache_hit_status,$cache_hit_percentage" >> "$CSV_FILE"
done

echo "Results have been saved to $CSV_FILE"

#!/bin/bash

# Define the input and output directories
INPUT_DIR="./inputs"
OUTPUT_DIR="./outputs"

# Create the output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# List of test cases
TESTCASES=(
    "BigSmall.md"
    "Day.md"
    "GentlerHour.md"
    "Hour.md"
    "Input.md"
    "MatchMelfYouCan.md"
    "NiceAndSmooth.md"
    "SpikeyMean.md"
    "SpikeyNefarious.md"
    "TallShort.md"
)

# Compile the project
make simulator && make scheduler

# Iterate through each testcase and run the simulator
for TESTCASE in "${TESTCASES[@]}"; do
    INPUT_FILE="$INPUT_DIR/$TESTCASE"
    OUTPUT_FILE="$OUTPUT_DIR/${TESTCASE%.md}.txt"

    echo "Running $TESTCASE..."
    ./simulator -v 0 "$INPUT_FILE" > "$OUTPUT_FILE"
done

echo "All test cases have been executed. Check the outputs in $OUTPUT_DIR."

# Go into every directory and run the script named test.sh if it exists
# Track exit codes and report at the end
# Count successful and failed tests

#!/bin/bash

success_count=0
failure_count=0

for dir in */ ; do
    if [ -f "$dir/test.sh" ]; then
        (cd "$dir" && bash test.sh)
        exit_code=$?
        if [ $exit_code -eq 0 ]; then
            success_count=$((success_count + 1))
        else
            echo "Test in $dir failed with exit code $exit_code."
            failure_count=$((failure_count + 1))
        fi
    fi
done

echo "Successful tests: $success_count"

if [ $failure_count -eq 0 ]; then
    echo ""
    echo "All tests passed"
    exit 0
else
    echo "Failed tests: $failure_count "
    exit 1
fi

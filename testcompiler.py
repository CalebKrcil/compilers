import os
import subprocess

# Paths
TEST_DIR = "tests"
CATEGORIES = ["errors", "k0", "kotlin"]  # Test categories
COMPILER_EXECUTABLE = "./k0"  # Compiler command

def run_test(file_path, expected_error):
    """Run the compiler on a test file and check if it produces the expected result."""
    try:
        # Execute the compiler and capture stdout + stderr
        result = subprocess.run([COMPILER_EXECUTABLE, file_path], capture_output=True, text=True)

        # Compiler outputs errors to stderr
        has_error = bool(result.stderr.strip())

        # Check if the output matches expectation
        if has_error == expected_error:
            print(f"PASS: {file_path}")
            return True
        else:
            print(f"FAIL: {file_path}")
            print(f"Expected {'Error' if expected_error else 'No Error'}, but got:")
            print(result.stderr if has_error else result.stdout)
            return False

    except Exception as e:
        print(f"ERROR running {file_path}: {e}")
        return False

def run_tests():
    """Runs the compiler on all test files in the test suite."""
    total_tests = 0
    passed_tests = 0

    for category in CATEGORIES:
        expected_error = category != "k0"  # k0 files should not have errors, others should
        category_path = os.path.join(TEST_DIR, category)

        if not os.path.isdir(category_path):
            print(f"Warning: Test folder '{category_path}' not found, skipping...")
            continue

        for file_name in os.listdir(category_path):
            file_path = os.path.join(category_path, file_name)
            if os.path.isfile(file_path):  # Only process files
                total_tests += 1
                if run_test(file_path, expected_error):
                    passed_tests += 1

    # Summary
    print("\n=== TEST SUMMARY ===")
    print(f"Total Tests: {total_tests}")
    print(f"Passed: {passed_tests}")
    print(f"Failed: {total_tests - passed_tests}")

if __name__ == "__main__":
    run_tests()

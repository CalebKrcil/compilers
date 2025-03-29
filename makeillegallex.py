import os

# List of illegal tokens
illegal_tokens = [
    "as", "as?", "class", "!in", "is", "!is", "object", "package",
    "super", "this", "throw", "try", "typealias", "typeof",
    "by", "catch", "constructor", "delegate", "dynamic", "field",
    "file", "finally", "get", "init", "param", "property", "receiver",
    "set", "setparam", "value", "where", "...", "*=", "/=", "%=", "->",
    "=>", "::", ";;", "@", "'", "~", "&=", "|=", "|", "^=", "^",
    ">>=", ">>", "<<=", "<<", "&"
]

# Directory to store the generated test files
output_dir = "lex_tests"

# Ensure the directory exists
os.makedirs(output_dir, exist_ok=True)

# Create test files
for i, token in enumerate(illegal_tokens, start=1):
    file_name = os.path.join(output_dir, f"lex_test{i}.kt")
    with open(file_name, "w") as f:
        f.write(f"// This file contains an illegal token\n{token}\n")

print(f"{len(illegal_tokens)} test files created in '{output_dir}'")
import os
import re
import sys

def replace_comment_block(directory_path):
    # Define the new text to replace the existing comment block
    new_text = '''/*
{
    "ALU" : ["FADD", "FMUL"],
    "MEMPORT"  : ["input", "output"],
    "Constant" : ["const"]
}
*/'''

    # Pattern to match the old comment block
    old_comment_pattern = re.compile(r'/\*.*?\*/', re.DOTALL)

    # Iterate over each file in the directory
    for filename in os.listdir(directory_path):
        # Only process files ending with '_Specific.dot'
        if filename.endswith('_Specific.dot'):
            file_path = os.path.join(directory_path, filename)
            
            # Read the content of the file
            with open(file_path, 'r') as file:
                content = file.read()
            
            # Replace the old comment block with the new text
            updated_content = old_comment_pattern.sub(new_text, content)
            
            # Write the updated content back to the file
            with open(file_path, 'w') as file:
                file.write(updated_content)

    print("Replacement complete for all _Specific.dot files.")

if __name__ == "__main__":
    # Get directory_path from command-line arguments
    if len(sys.argv) != 2:
        print("Usage: python script.py <directory_path>")
    else:
        directory_path = sys.argv[1]
        replace_comment_block(directory_path)


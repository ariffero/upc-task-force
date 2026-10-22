import os
import sys
import argparse

def list_files_recursively(start_path: str, output_file: str, extension: str = None) -> int:
  """
  Walks through the directory tree from start_path, collects each file's full path
  (with ~/ shorthand for anything under your home directory), optionally filtering by
  extension, excluding this script and the output file, writes them into output_file
  in alphabetic order (folder by folder) without a trailing newline at the end.
  Returns the count of files written.
  """
  script_name = os.path.basename(__file__)
  output_name = os.path.basename(output_file)
  home = os.path.expanduser('~')
  paths = []

  # Normalize extension filter
  ext_filter = None
  if extension:
    ext_filter = extension if extension.startswith('.') else f".{extension}"

  for root, dirs, files in os.walk(start_path):
    dirs.sort()    # traverse directories in alphabetic order
    files.sort()   # process files in alphabetic order within each folder
    for file in files:
      if file in (script_name, output_name):
        continue
      if ext_filter and not file.endswith(ext_filter):
        continue
      full_path = os.path.abspath(os.path.join(root, file))
      # if it's under your home directory, replace that part with '~'
      if full_path.startswith(home + os.sep):
        display_path = full_path.replace(home, '~', 1)
      else:
        display_path = full_path
      paths.append(display_path)

  # Write results (or leave file empty if no matches)
  with open(output_file, 'w', encoding='utf-8') as out:
    if paths:
      out.write("\n".join(paths))

  return len(paths)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='List files in a directory tree.')
  parser.add_argument(
    '--target-folder', '-t',
    help='Folder in which to search and save the output file (default: current directory)'
  )
  parser.add_argument(
    '--output-file', '-o',
    default='file_list.txt',
    help='Name of the output file (default: file_list.txt)'
  )
  parser.add_argument(
    '--extension', '-e',
    help='Filter to only include files with this extension (e.g. root for .root files)'
  )
  args = parser.parse_args()

  # Determine the folder to operate in
  target = os.path.abspath(args.target_folder) if args.target_folder else os.getcwd()
  start = target
  output_path = os.path.join(target, args.output_file)

  print(f"Scanning files under: {start}")
  try:
    total = list_files_recursively(start, output_path, args.extension)
    if args.extension and total == 0:
      print(f"No {args.extension} files here!")
    else:
      print(f"Success: wrote {total} file(s) to '{output_path}'")
  except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)

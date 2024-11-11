# CupidFM - file editing

CupidFM is a terminal-based file manager implemented in C, using the `ncurses` library for the user interface. It provides features like directory navigation, file preview, and displaying file information.

## Features

- Navigate directories using arrow keys
- View file details and preview supported file types
- Display MIME types based on file content using `libmagic`
- Command-line interface with basic file operations

## Todo

- [ ] Write custom magic library for in-house MIME type detection
- [X] Implement directory tree preview for directories
  - [ ] Enable scrolling for tree preview in the preview window
- [ ] Add preview support for `.zip` and `.tar` files
- [ ] Implement syntax highlighting for supported file types
- [ ] In text editing buffer implement short cuts like, shift arrow to select shift control arrows to select faster etc.
- [ ] Display symbolic links with correct arrow notation (e.g., `->` showing the target path)
- [ ] Fix being able to enter directory before calculation is done causing a empty directory and a broken file path
- [ ] Add text display on tree preview when user enters a empty dir
- [ ] Add easy select for selecting file names, dir names, and current directory
- [ ] Implement copy and paste short cuts, relating to easy select for file names... etc while also editing a file
- [X] Fix weird crash on different window resize
- [X] Fix text buffer from breaking the preview win border
- [ ] Add dir window scrolling up and down when you hit the bottom or top it brings you to either side
- [X] Fix issue with title banner notif rotating showing char when rotating from left side to right
- [X] Add build ver, and name somewhere
- [ ] Push to main branch
      
## Prerequisites

To build and run CupidFM, you need the following dependencies installed:

- `gcc` (GNU Compiler Collection)
- `make` (build automation tool)
- `ncurses` library for terminal handling
- `libmagic` for MIME type detection based on file content

### Installing Dependencies on Ubuntu

Run the following command to install the necessary packages:

```bash
sudo apt update
sudo apt install build-essential libncurses-dev libmagic-dev
```

## Building the Project

To compile the project, run:

```bash
./dev.sh
```

This script will use `make` with predefined flags to compile the source code and produce an executable named `cupidfm`.

### Compilation Flags

The script uses several compilation flags:

- `-Wall -Wextra -pedantic`: Enable warnings
- `-Wshadow -Werror -Wstrict-overflow`: Additional strictness for code
- `-fsanitize=address -fsanitize=undefined`: Enable sanitization for debugging

## Running the Program

After compilation, you can run the program:

```bash
./cupidfm
```

This will start CupidFM. Error logs will be saved in `log.txt`.

## File Structure

- `src/`: Contains the source code files
- `dev.sh`: Script for compiling the project
- `Makefile`: Used by `make` for the build process
- `LICENSE`, `README.md`: Documentation and license information

## Usage

Use the arrow keys to navigate the directory structure:
- **Up/Down**: Move between files
- **Left/Right**: Navigate to parent/child directories
- **F1**: Exit the application
- **TAB**: Change between preview and directory window
  - **CONTROL+E**: While in the preview window, you can edit within the file
  - **CONTROL+S**: Save files you are editing within the terminal 

## Contributing

Contributions are welcome! Please submit a pull request or open an issue for any changes.

## License

This project is licensed under the GNU General Public License v3.0 terms.

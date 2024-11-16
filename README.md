# CupidFM - file editing

CupidFM is a terminal-based file manager implemented in C. It uses the `ncurses` library for the user interface, providing features like directory navigation, directory tree preview, file preview, file editing, and file information display. 

### Terminal Requirements

For proper emoji display:
- Make sure your terminal emulator supports Unicode and emoji rendering
For proper emoji and icon display:

1. Install a Nerd Font (recommended):
```bash
# On Ubuntu/Debian:
mkdir -p ~/.local/share/fonts
cd ~/.local/share/fonts
curl -fLO https://github.com/ryanoasis/nerd-fonts/releases/download/v3.1.1/JetBrainsMono.zip
unzip JetBrainsMono.zip
fc-cache -fv

# On macOS with Homebrew:
brew tap homebrew/cask-fonts
brew install --cask font-jetbrains-mono-nerd-font

# On Windows:
# Download JetBrainsMono.zip from https://github.com/ryanoasis/nerd-fonts/releases
# Extract and install fonts through the Font Settings
```

2. Configure your terminal:
- Set your terminal font to "JetBrainsMono Nerd Font" (or another Nerd Font)
- Ensure your terminal emulator supports Unicode and emoji rendering
- Set your locale to support UTF-8: `export LANG=en_US.UTF-8`

Alternative fonts:
- Noto Color Emoji (`sudo apt install fonts-noto-color-emoji`)
- Fira Code Nerd Font
- Hack Nerd Font

If emojis aren't displaying correctly:
1. Check your terminal supports Unicode: `echo -e "\xf0\x9f\x93\x81"`
2. Verify locale settings: `locale`
3. Try updating your terminal emulator to a newer version

Note: Some terminal emulators like Alacritty, iTerm2, or Windows Terminal provide better Unicode/emoji support than others.

# Features

- Navigate directories using arrow keys
- View file details and preview supported file types
- Display MIME types based on file content using `libmagic`
- File type indicators with emoji icons:
  - 📄 Text files
  - 📝 C source files
  - 🔣 JSON files
  - 📑 XML files
  - 🐍 Python files
  - 🌐 HTML files
  - 🖼️ Image files
  - 🎞️ Video files
  - 🎵 Audio files
  - 📦 Archive files
  - 💻 Shell scripts
- Command-line interface with basic file operations

## Todo

### High Priority
- [ ] Fix directory list not staying within the border
- [ ] Fix directory preview not scrolling 
- [ ] Write custom magic library for in-house MIME type detection
- [ ] Add text display on tree preview when user enters a empty dir
## Edit Mode Issues
- [ ] Banner notification not rotating correctly when rotating when in edit mode
- [ ] Fix sig winch handling breaking while in edit mode
- [ ] Fix cursor showing up at the bottom of the text editing buffer
- [ ] Fix text buffer not scrolling to the right when typing and hitting the border of the window, it will just go invisible

### Features
- [ ] Enable scrolling for tree preview in the preview window when tabbed over
- [ ] Add preview support for `.zip` and `.tar` files
- [ ] Implement syntax highlighting for supported file types
- [ ] Display symbolic links with correct arrow notation (e.g., `->` showing the target path)
- [ ] In text editing buffer implement shortcuts (shift+arrow to select, ctrl+arrow for faster selection)
- [ ] More text editing shortcuts (ctrl+x, ctrl+c, ctrl+v, eg..)
- [ ] Add a **easy select** feature for selecting file names, dir names, and current directory
- [ ] Implement copy and paste shortcuts with easy select for files/directories

### Completed
- [X] Implement directory tree preview for directories
- [X] Fix weird crash on different window resize
- [X] Fix text buffer from breaking the preview win border
- [X] Fix issue with title banner notif rotating showing char when rotating from left side to right
- [X] Fix inputs being over loaded and taking awhile to execute (input delay when scrolling)
- [X] Add build ver, and name somewhere
- [X] Add cursor highlighting to text editing buffer and make it more precise 
- [X] Add line numbers to text editing buffer
- [X] Not updating preview window on directory enter and leave
- [X] File item list
- [X] Fix directory list being too big so it would be cut off
- [X] Fix crashing when trying to edit a too small of file
- [X] Add support for sig winch handling
- [X] Fix being able to enter directory before calculation is done causing a empty directory and a broken file path
- [X] Add dir window scrolling up and down when hitting bottom/top
- [X] Add tree structure visualization with folder (📁) and file (📄) icons, proper indentation, and smooth scrolling behavior similar to the file preview functionality
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

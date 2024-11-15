# CupidFM - file editing

CupidFM is a terminal-based file manager implemented in C. It uses the `ncurses` library for the user interface, providing features like directory navigation, directory tree preview, file preview, file editing, and file information display. 

## Features

- Navigate directories using arrow keys
- View file details and preview supported file types
- Display MIME types based on file content using `libmagic`
- Command-line interface with basic file operations

## Todo

### High Priority
- [ ] Fix being able to enter directory before calculation is done causing a empty directory and a broken file path
- [ ] Fix directory preview not scrolling 
- [ ] Add text display on tree preview when user enters a empty dir
- [ ] Write custom magic library for in-house MIME type detection
- [ ] Banner notification not rotating correctly when rotating when in edit mode

### Features
- [ ] Enable scrolling for tree preview in the preview window when tabbed over
- [ ] Add preview support for `.zip` and `.tar` files
- [ ] Implement syntax highlighting for supported file types
- [ ] Display symbolic links with correct arrow notation (e.g., `->` showing the target path)
- [ ] Add dir window scrolling up and down when hitting bottom/top
- [ ] Add easy select for selecting file names, dir names, and current directory
- [ ] Implement copy and paste shortcuts with easy select for files/directories
- [ ] In text editing buffer implement shortcuts (shift+arrow to select, ctrl+arrow for faster selection)

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
- [ ] Push to main branch

## Prerequisites

To build and run CupidFM, you need the following dependencies installed:

- `gcc` (GNU Compiler Collection)
- `make` (build automation tool)
- `ncurses` library for terminal handling
- `libmagic` for MIME type detection based on file content

### Installing Dependencies on Ubuntu

Run the following command to install the necessary packages:

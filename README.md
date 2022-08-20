# VED
Terminal based vi-like text editor built on top of GNU ed

# Project goals
I wanted to make a hackable, vi-like zero configuration text editor<br>
for my personal daily use. I started with VIM, it's amazing but bloated.<br>
Then I tried vi - much better but still lot's of redundant features.<br>
Finally GNU ed, the editor ex/vi is based on, got into my scope.<br>
It would've been just perfect if only it had a visual interface,<br>
so I thought what if I do to ed what Bill Joy did to ex?<br>
Here's how ved was born.


# Features
    Ved is a modal text editor, just like vi or vim.
    It has normal, insert/replace and command modes.
    
    NORMAL MODE

    h    move cursor left
    j    move cursor down
    k    move cursor up
    l    move cursor right

    i    go to insert mode
    a    move cursor right, go to insert mode
    A    move cursor to the end of the line, go to insert mode
    o    add empty new line below the cursor
    O    add empty new line above the cursor
    x    delete char under the cursor
    r    go to single char replace mode
    R    go to multi char replace mode

    #    go to the beginning of the line
    $    go to the end of the line

    ESC  return to normal mode


    COMMAND MODE

    All but l,p,n GNU ed commands are valid, type "$ man ved" or "$ man ed" for more details.
    Visual cursor and ed's current address are synced, so you can either manipulate the
    cursor position by typing commands or manipulate the line under the current line.
    While being in normal mode type ':' to enter command mode. Here're some common commands:

    10   go to line number 10
    %    go to first line
    $    go to last line
    /    search pattern forwards
    ?    search pattern backwards
    s    substitute one patern by another

    e    open file for editing, warn if current file has unsaved changes
    E    open file for editing, drop previous file without saving
    w    write current file
    W    append to current file
    r    read file into current buffer

    i    insert line above the current line
    a    insert line below the current line
    c    change current line
    y    copy line(s) to buffer
    d    cut line(s) to buffer
    x    paste line(s) from buffer
    u    undo last change

    !    execute shell command

# Common operations

    COPY/PASTE block of lines:

    1. Put the cursor to the source line
    2. Type: ":.,.5y" - copy 6 lines from cursor and below
    3. Put the cursor to the destination line
    4. Type: "x" - paste 6 lines

    INDENT block / MULTILINE input:

    1. Put the cursor to the source line
    2. Type: ":.,.5s/^/    /" - move 6 lines by 4 spaces to the right 
    3. Replace 4 spaces in the above command with whatever word for multiline input
    4. Type: "x" - paste 6 lines

# Latest release
https://github.com/maksimKorzh/ved/releases/tag/0.0.1

# Build from sources
    git clone https://github.com/maksimKorzh/ved.git
    cd ved
    ./configure
    make
    make install

# Development progress
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/xW4S7-KwOUc/0.jpg)](https://www.youtube.com/watch?v=xW4S7-KwOUc&list=PLLfIBXQeu3aYGP3-x5x-INDUvCDfL570N)

// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/4/Fill.asm

// Runs an infinite loop that listens to the keyboard input. 
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel. When no key is pressed, 
// the screen should be cleared.

//// Replace this comment with your code.

(LOOP)
@KBD
D=M
@DRAW
// If no key is currently being pressed (M[KBD] = 0), a white color will be drawn;
// in this case, D is (0000000000000000)₂
D;JEQ
// Otherwise, a key is being pressed (M[KBD] ≠ 0), and a black color will be drawn;
// D becomes (1111111111111111)₂
D=0
D=!D

(DRAW)
// The value in R0 determines the color to be drawn (white or black),
//  based on the keyboard input check above.
@R0
M=D
// The screen memory begins at address 16384.
@SCREEN
D=A
// The screen contains 8192 words => (2^9 rows)(2^8 columns) * (1 word)/(2^4 pixels) = 2^13 = 8192
@8192
// The screen final address is 16384 + 8192 = 24576.
D=D+A
// R1 holds the address of the last screen word (i.e., 24576).
@R1
M=D

(PIXELS)
@SCREEN
// The drawing process is performed word-by-word, in reverse order from address 24576 down to 16384.
// If D = D - A results in zero, then the starting screen address (16384) has been reached, and the
// drawing is complete.
D=D-A
@LOOP
D;JEQ
// Load the color value to be written to the screen.
@R0
D=M
@R1
// Decrement R1 to point to the next screen word (from 24576 to 16384).
M=M-1
// Set A to the current screen address stored in R1.
A=M // A = M[R1]
// Write the color value into the current screen word.
M=D // M[A] = D => M[M[R1]] = D
// Prepare D with the next screen address for the following iteration.
@R1
D=M
@PIXELS
0;JMP
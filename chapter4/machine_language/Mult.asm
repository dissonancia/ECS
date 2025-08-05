// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/4/Mult.asm

// Multiplies R0 and R1 and stores the result in R2.
// (R0, R1, R2 refer to RAM[0], RAM[1], and RAM[2], respectively.)
// The algorithm is based on repetitive addition.

// The optimal implementation of this multiplication routine would be achievable if the Hack language
// supported left and right shift operations. With such instructions, Ancient Egyptian (binary)
// multiplication could be implemented in O(log n) time. Even without shift-left, multiplication by 2
// can still be performed efficiently via addition (e.g., x + x = 2x). However, the lack of shift-right
// means that dividing a value by two requires a loop based on R1, increasing the algorithm’s complexity
// to O(n).

// In this implementation, R0 represents the value being added repeatedly, and R1 represents the number
// of times the addition occurs. To reduce the number of iterations in the loop, we use a conditional
// optimization that swaps R0 and R1, but only when doing so results in a significant performance gain.
// Specifically, the swap is only performed when R0 < R1 - 15. This ensures that the expected reduction
// in loop iterations is greater than the cost of the additional 15 instructions introduced by the
// comparison and swap logic. The 8 instructions for comparison is the only overhead.

// Given that Hack integers range up to 32,767, the number of input combinations where this optimization
// does not yield a net benefit is negligible compared to the total number of valid pairs. Therefore,
// the added logic is justified in nearly all practical scenarios.

@R1
D=M
@15
D=D-A        // D = R1 - 15
@R0
D=D-M        // D = (R1 - 15) - R0
@SKIP_SWAP
D;JLE        // If R0 >= R1 - 15, skip swap

// swap R0 e R1
@R0
D=M         // D = R0
@R1
D=D+M       // D = R0 + R1
M=D-M       // R1 = D - R1 = R0
@R0
M=D-M       // R0 = D - R1 = R1

(SKIP_SWAP)

@R2
M=0           // result = 0

(LOOP)
@R1
D=M
@END
D;JEQ         // if R1 == 0, end

@R0
D=M
@R2
M=D+M         // result += R0

@R1
M=M-1         // R1--

@LOOP
0;JMP

(END)
@END
0;JMP